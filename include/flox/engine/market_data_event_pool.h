/*
 * Flox Engine
 * Developed by Evgenii Makarov (https://github.com/eeiaao)
 *
 * Copyright (c) 2025 Evgenii Makarov
 * Licensed under the MIT License. See LICENSE file in the project root for full
 * license information.
 */

#pragma once

#include "flox/engine/abstract_event_pool.h"
#include "flox/util/spsc_queue.h"

#include <array>
#include <cassert>
#include <cstddef>
#include <memory_resource>
#include <new>

namespace flox {

template <typename EventT>
  requires std::derived_from<EventT, IMarketDataEvent>
class EventHandle {
public:
  EventHandle() noexcept : _event(nullptr) {}

  explicit EventHandle(EventT *event) noexcept : _event(event) {}

  EventHandle(const EventHandle &) = delete;
  EventHandle &operator=(const EventHandle &) = delete;

  EventHandle(EventHandle &&other) noexcept : _event(other._event) {
    other._event = nullptr;
  }

  EventHandle &operator=(EventHandle &&other) noexcept {
    if (this != &other) {
      release();
      _event = other._event;
      other._event = nullptr;
    }
    return *this;
  }

  ~EventHandle() { release(); }

  EventT *get() const noexcept { return _event; }
  EventT *operator->() const noexcept { return _event; }
  EventT &operator*() const noexcept { return *_event; }

  explicit operator bool() const noexcept { return _event != nullptr; }

  template <typename U> EventHandle<U> upcast() const {
    static_assert(std::is_base_of_v<U, EventT>, "Can only upcast to base type");
    if (_event) {
      _event->retain();
    }

    return EventHandle<U>(_event);
  }

private:
  void release() {
    if (_event && _event->release()) {
      _event->releaseToPool();
    }

    _event = nullptr;
  }

  EventT *_event;
};

template <typename EventT, size_t Capacity>
  requires std::derived_from<EventT, IMarketDataEvent>
class EventPool : public IEventPool {
public:
  EventPool() : _arena(_buffer.data(), _buffer.size()), _pool(&_arena) {
    for (size_t i = 0; i < Capacity; ++i) {
      auto *event = new (&_slots[i]) EventT(&_pool);
      event->setPool(this);
      _queue.push(event);
    }
  }

  EventHandle<EventT> acquire() {
    EventT *event = nullptr;
    if (_queue.pop(event)) {
      event->resetRefCount(); // refCount = 1
      event->setPool(this);

      ++_acquired;

      return EventHandle(event);
    }

    return {}; // null
  }

  void release(IMarketDataEvent *base) override {
    EventT *concrete = static_cast<EventT *>(base);
    concrete->clear();
    _queue.push(concrete);

    ++_released;
  }

  size_t inUse() const { return _acquired - _released; }

private:
  using Storage = std::aligned_storage_t<sizeof(EventT), alignof(EventT)>;
  Storage _slots[Capacity];

  std::array<std::byte, 128 * 1024> _buffer;
  std::pmr::monotonic_buffer_resource _arena;
  std::pmr::unsynchronized_pool_resource _pool;
  SPSCQueue<EventT *, Capacity + 1> _queue;

  size_t _acquired = 0;
  size_t _released = 0;
};

} // namespace flox
