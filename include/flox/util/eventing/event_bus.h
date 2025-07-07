/*
 * Flox Engine
 * Developed by FLOX Foundation (https://github.com/FLOX-Foundation)
 *
 * Copyright (c) 2025 FLOX Foundation
 * Licensed under the MIT License. See LICENSE file in the project root for full
 * license information.
 */

#pragma once

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>
#include <unordered_map>

#include "flox/engine/engine_config.h"
#include "flox/engine/subscriber_component.h"
#include "flox/engine/tick_barrier.h"
#include "flox/util/concurrency/spsc_queue.h"
#include "flox/util/eventing/event_bus_component.h"
#include "flox/util/performance/cpu_affinity.h"

namespace flox
{

template <typename Event, typename Policy, size_t QueueSize = config::DEFAULT_EVENTBUS_QUEUE_SIZE>
class EventBus
{
 public:
  using Listener = typename ListenerType<Event>::type;
  using QueueItem = typename Policy::QueueItem;
  using Queue = SPSCQueue<QueueItem, QueueSize>;

  using Trait = traits::EventBusTrait<Event, Queue>;
  using Allocator = PoolAllocator<Trait, 8>;

  EventBus() = default;

  ~EventBus()
  {
    stop();
  }

  EventBus(EventBus&& other) = delete;
  EventBus& operator=(EventBus&&) = delete;

  void subscribe(Listener listener)
  {
    SubscriberId id = listener.id();
    SubscriberMode mode = listener.mode();

    std::lock_guard lock(_mutex);
    _subs.try_emplace(id, Entry(mode, std::move(listener), std::make_unique<Queue>()));
  }

  void start()
  {
    if (_running.exchange(true))
    {
      return;
    }

    std::lock_guard lock(_mutex);
    _active = 0;

    for (auto& [_, e] : _subs)
    {
      if (e.mode == SubscriberMode::PUSH)
      {
        ++_active;
      }
    }

    for (auto& [_, e] : _subs)
    {
      if (e.mode == SubscriberMode::PUSH)
      {
        auto* queue = e.queue.get();
        auto listener = e.listener;

        e.thread = std::make_unique<std::thread>([this, queue, listener = std::move(listener)] mutable
                                                 {
          // Apply enhanced CPU affinity if configured
          if (_coreAssignment.has_value() && _affinityConfig.has_value())
          {
            auto& assignment = _coreAssignment.value();
            auto& config = _affinityConfig.value();
            
            // Select appropriate cores based on component type
            std::vector<int> targetCores;
            std::string componentName;
            
            switch (config.componentType)
            {
              case ComponentType::MARKET_DATA:
                targetCores = assignment.marketDataCores;
                componentName = "marketData";
                break;
              case ComponentType::EXECUTION:
                targetCores = assignment.executionCores;
                componentName = "execution";
                break;
              case ComponentType::STRATEGY:
                targetCores = assignment.strategyCores;
                componentName = "strategy";
                break;
              case ComponentType::RISK:
                targetCores = assignment.riskCores;
                componentName = "risk";
                break;
              case ComponentType::GENERAL:
                targetCores = assignment.generalCores;
                componentName = "general";
                break;
            }
            
            // Pin to appropriate core
            if (!targetCores.empty())
            {
              int coreId = targetCores[0];  // Use first assigned core
              bool pinned = performance::CpuAffinity::pinToCore(coreId);
              
              if (pinned && assignment.hasIsolatedCores)
              {
                // Check if we're using an isolated core
                bool isIsolated = std::find(assignment.allIsolatedCores.begin(), 
                                          assignment.allIsolatedCores.end(), coreId) 
                                != assignment.allIsolatedCores.end();
              }
              
              // Set real-time priority if enabled
              if (config.enableRealTimePriority)
              {
                performance::CpuAffinity::setRealTimePriority(config.realTimePriority);
              }
            }
          }
          else if (_coreAssignment.has_value())
          {
            // Direct assignment fallback - pin to market data cores as default
            auto& assignment = _coreAssignment.value();
            if (!assignment.marketDataCores.empty())
            {
              performance::CpuAffinity::pinToCore(assignment.marketDataCores[0]);
              performance::CpuAffinity::setRealTimePriority(90);
            }
          }

          {
            std::lock_guard<std::mutex> lk(_readyMutex);
            if (--_active == 0) _cv.notify_one();
          }

          while (_running.load(std::memory_order_acquire)) {
            if (auto* item = queue->try_pop())
            {
              Policy::dispatch(*item, listener);
              item->~QueueItem();
            } else {
              std::this_thread::yield();
            }
          }

          while (auto* item = queue->try_pop())
          {
            if (_drainOnStop)
            {
              Policy::dispatch(*item, listener);
            }

            item->~QueueItem();
          } });
      }
    }

    std::unique_lock lk(_readyMutex);
    _cv.wait(lk, [&]
             { return _active == 0; });
  }

  void stop()
  {
    if (!_running.exchange(false))
      return;

    decltype(_subs) localSubs;
    {
      std::lock_guard lk(_mutex);
      localSubs.swap(_subs);
    }
  }

  void publish(Event ev)
  {
    if (!_running.load(std::memory_order_acquire))
      return;

    uint64_t seq = _tickCounter.fetch_add(1, std::memory_order_relaxed);

    if constexpr (requires { ev->tickSequence; })
    {
      ev->tickSequence = seq;
    }
    if constexpr (requires { ev.tickSequence; })
    {
      ev.tickSequence = seq;
    }

    [[maybe_unused]] TickBarrier barrier(_subs.size());

    std::lock_guard lock(_mutex);
    for (auto& [_, e] : _subs)
    {
      if constexpr (std::is_same_v<Policy, SyncPolicy<Event>>)
      {
        e.queue->emplace(Policy::makeItem(ev, &barrier));
      }
      else
      {
        e.queue->emplace(Policy::makeItem(ev, nullptr));
      }
    }

    if constexpr (std::is_same_v<Policy, SyncPolicy<Event>>)
    {
      barrier.wait();
    }
  }

  std::optional<std::reference_wrapper<Queue>> getQueue(SubscriberId id) const
  {
    std::lock_guard lock(_mutex);
    auto it = _subs.find(id);
    if (it != _subs.end() && it->second.mode == SubscriberMode::PULL && it->second.queue)
    {
      return std::ref(*it->second.queue);
    }

    return std::nullopt;
  }

  uint64_t currentTickId() const
  {
    return _tickCounter.load(std::memory_order_relaxed);
  }

  void enableDrainOnStop()
  {
    _drainOnStop = true;
  }

  /**
   * @brief Event bus component types for CPU affinity assignment
   */
  enum class ComponentType
  {
    MARKET_DATA,  // Market data processing (trade events, book updates)
    EXECUTION,    // Order execution and fills
    STRATEGY,     // Strategy computation and signals
    RISK,         // Risk management and validation
    GENERAL       // General purpose / logging / metrics
  };

  /**
   * @brief Configuration for event bus CPU affinity
   */
  struct AffinityConfig
  {
    ComponentType componentType = ComponentType::GENERAL;
    bool enableRealTimePriority = true;
    int realTimePriority = 80;
    bool enableNumaAwareness = true;
    bool preferIsolatedCores = true;

    AffinityConfig() = default;

    AffinityConfig(ComponentType type, int priority = 80)
        : componentType(type), realTimePriority(priority) {}
  };

  /**
   * @brief Configure CPU affinity for event bus threads using enhanced isolated core functionality
   * @param config Affinity configuration including component type and priorities
   */
  void setAffinityConfig(const AffinityConfig& config)
  {
    _affinityConfig = config;

    // Generate optimal core assignment using isolated cores
    performance::CpuAffinity::CriticalComponentConfig coreConfig;
    coreConfig.preferIsolatedCores = config.preferIsolatedCores;
    coreConfig.exclusiveIsolatedCores = true;
    coreConfig.allowSharedCriticalCores = false;

    if (config.enableNumaAwareness)
    {
      _coreAssignment = performance::CpuAffinity::getNumaAwareCoreAssignment(coreConfig);
    }
    else
    {
      _coreAssignment = performance::CpuAffinity::getRecommendedCoreAssignment(coreConfig);
    }
  }

  /**
   * @brief Configure CPU affinity using direct core assignment
   * @param assignment Core assignment configuration
   * @note For advanced use cases. Most users should use setupOptimalConfiguration() instead
   */
  void setCoreAssignment(const performance::CpuAffinity::CoreAssignment& assignment)
  {
    _coreAssignment = assignment;
    // Set default affinity config for compatibility
    _affinityConfig = AffinityConfig{ComponentType::GENERAL, 80};
  }

  /**
   * @brief Get current CPU affinity configuration
   * @return Optional core assignment
   */
  std::optional<performance::CpuAffinity::CoreAssignment> getCoreAssignment() const
  {
    return _coreAssignment;
  }

  /**
   * @brief Get current affinity configuration
   * @return Optional affinity configuration
   */
  std::optional<AffinityConfig> getAffinityConfig() const
  {
    return _affinityConfig;
  }

  /**
   * @brief Setup optimal performance configuration for this event bus
   * @param componentType Type of component this event bus serves
   * @param enablePerformanceOptimizations Enable CPU frequency scaling and other optimizations
   * @return true if setup was successful
   */
  bool setupOptimalConfiguration(ComponentType componentType, bool enablePerformanceOptimizations = false)
  {
    AffinityConfig config;
    config.componentType = componentType;
    config.enableRealTimePriority = true;
    config.enableNumaAwareness = true;
    config.preferIsolatedCores = true;

    // Set component-specific priorities
    switch (componentType)
    {
      case ComponentType::MARKET_DATA:
        config.realTimePriority = 90;  // Highest priority
        break;
      case ComponentType::EXECUTION:
        config.realTimePriority = 85;  // Second highest
        break;
      case ComponentType::STRATEGY:
        config.realTimePriority = 80;  // Third priority
        break;
      case ComponentType::RISK:
        config.realTimePriority = 75;  // Fourth priority
        break;
      case ComponentType::GENERAL:
        config.realTimePriority = 70;           // Lower priority
        config.enableRealTimePriority = false;  // Don't use RT priority for general
        break;
    }

    setAffinityConfig(config);

    if (enablePerformanceOptimizations)
    {
      // Optionally disable frequency scaling for performance
      performance::CpuAffinity::disableCpuFrequencyScaling();
    }

    return _coreAssignment.has_value();
  }

  /**
   * @brief Verify that this event bus is properly configured for isolated cores
   * @return true if using isolated cores optimally
   */
  bool verifyIsolatedCoreConfiguration() const
  {
    if (!_coreAssignment.has_value())
    {
      return false;
    }

    return performance::CpuAffinity::verifyCriticalCoreIsolation(_coreAssignment.value());
  }

 private:
  struct Entry
  {
    SubscriberMode mode = SubscriberMode::PUSH;
    Listener listener;
    std::unique_ptr<Queue> queue;
    std::unique_ptr<std::thread> thread;

    Entry(SubscriberMode mode, Listener&& listener, std::unique_ptr<Queue>&& queue)
        : mode(mode), listener(std::move(listener)), queue(std::move(queue)) {}

    Entry(const Entry&) = delete;
    Entry& operator=(const Entry&) = delete;

    Entry(Entry&&) = default;
    Entry& operator=(Entry&&) = default;

    ~Entry()
    {
      if (thread && thread->joinable())
      {
        thread->join();
      }

      thread.reset();
    }
  };

  std::unordered_map<SubscriberId, Entry> _subs;
  mutable std::mutex _mutex;
  std::atomic<bool> _running{false};
  std::atomic<size_t> _active{0};
  std::condition_variable _cv;
  std::mutex _readyMutex;
  std::atomic<uint64_t> _tickCounter{0};
  bool _drainOnStop = false;
  std::optional<performance::CpuAffinity::CoreAssignment> _coreAssignment;
  std::optional<AffinityConfig> _affinityConfig;
};

}  // namespace flox
