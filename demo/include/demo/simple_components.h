/*
 * Flox Engine
 * Developed by FLOX Foundation (https://github.com/FLOX-Foundation)
 *
 * Copyright (c) 2025 FLOX Foundation
 * Licensed under the MIT License. See LICENSE file in the project root for full
 * license information.
 */

#pragma once

#include "demo/latency_collector.h"
#include "flox/execution/bus/order_execution_bus.h"
#include "flox/execution/order_execution_listener_component.h"
#include "flox/execution/order_executor_component.h"
#include "flox/killswitch/killswitch_component.h"
#include "flox/metrics/execution_tracker_component.h"
#include "flox/position/position_manager_component.h"
#include "flox/risk/risk_manager_component.h"
#include "flox/sink/storage_sink_component.h"
#include "flox/util/base/ref.h"
#include "flox/validation/order_validator_component.h"

#include <chrono>
#include <cstddef>
#include <iostream>
#include <random>
#include <string>

namespace demo
{

using namespace flox;

class ConsoleExecutionTracker
{
 public:
  using Trait = traits::ExecutionTrackerTrait;
  using Allocator = PoolAllocator<Trait, 8>;

  explicit ConsoleExecutionTracker(SubscriberId id) : _id(id) {}

  SubscriberId id() const { return _id; }
  SubscriberMode mode() const { return SubscriberMode::PUSH; }

  void start() {};
  void stop() {};

  void onOrderSubmitted(const Order& order, std::chrono::steady_clock::time_point ts)
  {
    std::cout << "[tracker] submitted " << order.id << " at " << ts.time_since_epoch().count() << '\n';
  }
  void onOrderAccepted(const Order& order, std::chrono::steady_clock::time_point ts)
  {
    std::cout << "[tracker] accepted " << order.id << " at " << ts.time_since_epoch().count() << '\n';
  }

  void onOrderPartiallyFilled(const Order& order, Quantity qty,
                              std::chrono::steady_clock::time_point ts)
  {
    std::cout << "[tracker] partial fill " << order.id << " qty=" << qty.toDouble()
              << " at " << ts.time_since_epoch().count() << '\n';
  }
  void onOrderFilled(const Order& order, std::chrono::steady_clock::time_point ts)
  {
    std::cout << "[tracker] filled " << order.id << " after " << ts.time_since_epoch().count() << '\n';
  }
  void onOrderCanceled(const Order& order, std::chrono::steady_clock::time_point ts)
  {
    std::cout << "[tracker] canceled " << order.id << " at " << ts.time_since_epoch().count() << '\n';
  }

  void onOrderExpired(const Order& order, std::chrono::steady_clock::time_point ts)
  {
    std::cout << "[tracker] expired " << order.id << " at " << ts.time_since_epoch().count() << '\n';
  }
  void onOrderRejected(const Order& order, const std::string& reason,
                       std::chrono::steady_clock::time_point)
  {
    std::cout << "[tracker] rejected " << order.id << " reason=" << reason << '\n';
  }

  void onOrderReplaced(const Order& oldOrder, const Order& newOrder,
                       std::chrono::steady_clock::time_point ts)
  {
    std::cout << "[tracker] replaced old=" << oldOrder.id << " new=" << newOrder.id
              << " at " << ts.time_since_epoch().count() << '\n';
  }

 private:
  SubscriberId _id;
};
static_assert(concepts::ExecutionTracker<ConsoleExecutionTracker>);

template <typename T>
class ExecutionTrackerAdapter
{
 public:
  using Trait = traits::OrderExecutionListenerTrait;
  using Allocator = PoolAllocator<Trait, 8>;

  explicit ExecutionTrackerAdapter(ExecutionTrackerRef impl)
      : _impl(std::move(impl)) {}

  SubscriberId id() const { return _impl.id(); }
  SubscriberMode mode() const { return _impl.mode(); }

  void start() { _impl.start(); }
  void stop() { _impl.stop(); }

  void onOrderSubmitted(const Order& o) { _impl.onOrderSubmitted(o, now()); }
  void onOrderAccepted(const Order& o) { _impl.onOrderAccepted(o, now()); }
  void onOrderPartiallyFilled(const Order& o, Quantity q) { _impl.onOrderPartiallyFilled(o, q, now()); }
  void onOrderFilled(const Order& o) { _impl.onOrderFilled(o, now()); }
  void onOrderCanceled(const Order& o) { _impl.onOrderCanceled(o, now()); }
  void onOrderExpired(const Order& o) { _impl.onOrderExpired(o, now()); }
  void onOrderRejected(const Order& o, const std::string& r) { _impl.onOrderRejected(o, r, now()); }
  void onOrderReplaced(const Order& o1, const Order& o2) { _impl.onOrderReplaced(o1, o2, now()); }

 private:
  static std::chrono::steady_clock::time_point now()
  {
    return std::chrono::steady_clock::now();
  }

  ExecutionTrackerRef _impl;
};

class SimplePnLTracker
{
 public:
  using Trait = traits::OrderExecutionListenerTrait;
  using Allocator = PoolAllocator<Trait, 8>;

  SimplePnLTracker(SubscriberId id) : _id(id) {}

  void start() {}
  void stop() {}

  SubscriberId id() const { return _id; }
  SubscriberMode mode() const { return SubscriberMode::PUSH; }

  void onOrderFilled(const Order& order)
  {
    double value = order.price.toDouble() * order.quantity.toDouble();
    _pnl += (order.side == Side::BUY ? -value : value);
    std::cout << "[pnl] " << _pnl << '\n';
  }

  void onOrderSubmitted(const Order& order) {}
  void onOrderAccepted(const Order& order) {}
  void onOrderPartiallyFilled(const Order& order, Quantity qty) {}
  void onOrderCanceled(const Order& order) {}
  void onOrderExpired(const Order& order) {}
  void onOrderRejected(const Order&, const std::string&) {}
  void onOrderReplaced(const Order& oldOrder, const Order& newOrder) {}

 private:
  SubscriberId _id;

  double _pnl = 0.0;
};
static_assert(concepts::OrderExecutionListener<SimplePnLTracker>);

class StdoutStorageSink
{
 public:
  using Trait = traits::StorageSinkTrait;
  using Allocator = PoolAllocator<Trait, 8>;

  void start() {}
  void stop() {}
  void store(const Order& order) { std::cout << "[storage] order " << order.id << '\n'; }
};
static_assert(concepts::StorageSink<StdoutStorageSink>);

class SimpleOrderValidator
{
 public:
  using Trait = flox::traits::OrderValidatorTrait;
  using Allocator = PoolAllocator<Trait, 8>;

  bool validate(const Order& order, std::string& reason)
  {
    static thread_local std::mt19937 rng(std::random_device{}());
    static std::uniform_int_distribution<int> dist(0, 19);

    if (dist(rng) == 0)
    {
      reason = "random rejection";
      return false;
    }
    return true;
  }
};
static_assert(concepts::OrderValidator<SimpleOrderValidator>);

class SimpleKillSwitch
{
 public:
  using Trait = traits::KillSwitchTrait;
  using Allocator = PoolAllocator<Trait, 8>;

  void check(const Order& order)
  {
  }

  void trigger(const std::string& r)
  {
    _triggered = true;
    _reason = r;
    _since = std::chrono::steady_clock::now();
  }

  bool isTriggered() { return _triggered; }

  std::string reason() { return _reason; }

 private:
  void reset()
  {
    _triggered = false;
    _reason.clear();
  }

  bool _triggered = false;
  std::string _reason;
  std::chrono::steady_clock::time_point _since{};
};
static_assert(concepts::KillSwitch<SimpleKillSwitch>);

class SimpleRiskManager
{
 public:
  using Trait = traits::RiskManagerTrait;
  using Allocator = PoolAllocator<Trait, 8>;

  explicit SimpleRiskManager(KillSwitchRef ks) : _ks(std::move(ks)) {}

  void start() {}
  void stop() {}

  bool allow(const Order& order)
  {
    static thread_local std::mt19937 rng(std::random_device{}());
    static std::uniform_real_distribution<> dist(0.0, 1.0);

    if (dist(rng) < 0.05)
    {
      std::cout << "[risk] rejected order id=" << order.id << " (random)\n";
      return false;
    }

    return true;
  }

 private:
  KillSwitchRef _ks;
};
static_assert(concepts::RiskManager<SimpleRiskManager>);

class SimpleOrderExecutor
{
 public:
  using Trait = traits::OrderExecutorTrait;
  using Allocator = PoolAllocator<Trait, 8>;

  explicit SimpleOrderExecutor(OrderExecutionBusRef bus,
                               OrderExecutionListenerRef pnl,
                               StorageSinkRef storage,
                               PositionManagerRef posMgr)
      : _bus(bus),
        _pnlTracker(std::move(pnl)),
        _storage(std::move(storage)),
        _posMgr(std::move(posMgr)) {}

  void submitOrder(const Order& order)
  {
    // accepted
    OrderEvent ev{OrderEventType::ACCEPTED};
    ev.order = order;
    _bus.publish(ev);

    // simulate partial fill
    Quantity half = Quantity::fromRaw(order.quantity.raw() / 2);
    ev = {};
    ev.type = OrderEventType::PARTIALLY_FILLED;
    ev.order = order;
    ev.fillQty = half;
    _bus.publish(ev);

    Order part = order;
    part.quantity = half;
    _pnlTracker.onOrderFilled(part);
    _posMgr.onOrderFilled(part);

    // simulate replace
    Order newOrder = order;
    newOrder.price += Price::fromDouble(0.1);
    ev = {};
    ev.type = OrderEventType::REPLACED;
    ev.order = order;
    ev.newOrder = newOrder;
    _bus.publish(ev);

    // final fill of remaining quantity
    ev = {};
    ev.type = OrderEventType::FILLED;
    ev.order = newOrder;
    ev.fillQty = order.quantity - half;
    _bus.publish(ev);

    _storage.store(newOrder);
    Order rest = newOrder;
    rest.quantity = order.quantity - half;
    _pnlTracker.onOrderFilled(rest);
    _posMgr.onOrderFilled(rest);

    collector.record(LatencyCollector::EndToEnd,
                     std::chrono::steady_clock::now() - order.createdAt);
  }

  void cancelOrder(OrderId) {}
  void replaceOrder(OrderId, const Order&) {}

 private:
  OrderExecutionBusRef _bus;
  OrderExecutionListenerRef _pnlTracker;
  StorageSinkRef _storage;
  PositionManagerRef _posMgr;
};
static_assert(concepts::OrderExecutor<SimpleOrderExecutor>, "SimpleOrderExecutor must satisfy OrderExecutor");

class SimplePositionManager
{
  static constexpr size_t MAX_SYMBOLS = 65'536;

 public:
  using Trait = traits::PositionManagerTrait;
  using Allocator = PoolAllocator<Trait, 8>;

  explicit SimplePositionManager(SubscriberId id) : _id(id) {}

  SubscriberId id() const { return _id; }
  SubscriberMode mode() const { return SubscriberMode::PUSH; }

  void start() {}
  void stop() {}

  void onOrderSubmitted(const Order& order)
  {
    std::cout << "[position] order submitted: id=" << order.id << '\n';
  }
  void onOrderAccepted(const Order& order)
  {
    std::cout << "[position] order accepted: id=" << order.id << '\n';
  }
  void onOrderPartiallyFilled(const Order& order, Quantity qty)
  {
    std::cout << "[position] order partially filled: id=" << order.id
              << ", qty=" << qty.toDouble() << '\n';
    update(order, qty);
  }

  void onOrderFilled(const Order& order)
  {
    std::cout << "[position] order filled: id=" << order.id
              << ", qty=" << order.quantity.toDouble() << '\n';
    update(order, order.quantity);
  }

  void onOrderCanceled(const Order& order)
  {
    std::cout << "[position] order canceled: id=" << order.id << '\n';
  }

  void onOrderExpired(const Order& order)
  {
    std::cout << "[position] order expired: id=" << order.id << '\n';
  }

  void onOrderRejected(const Order& order, const std::string& reason)
  {
    std::cout << "[position] order rejected: id=" << order.id << " reason: " << reason << '\n';
  }

  void onOrderReplaced(const Order& oldOrder, const Order& newOrder)
  {
    std::cout << "[position] order replaced: old_id=" << oldOrder.id
              << ", new_id=" << newOrder.id << '\n';
  }

  Quantity getPosition(SymbolId symbol)
  {
    return _positions[symbol];
  }

  void printPositions() const
  {
    for (SymbolId i = 0; i < MAX_SYMBOLS; ++i)
    {
      if (!_positions[i].isZero())
      {
        std::cout << "Symbol " << i << ": " << _positions[i].toDouble() << "\n";
      }
    }
  }

 private:
  void update(const Order& order, Quantity qty)
  {
    if (order.side == Side::BUY)
    {
      _positions[order.symbol] += qty;
    }
    else
    {
      _positions[order.symbol] -= qty;
    }
  }

  SubscriberId _id;
  Quantity _positions[MAX_SYMBOLS]{};
};

static_assert(concepts::PositionManager<SimplePositionManager>);

}  // namespace demo
