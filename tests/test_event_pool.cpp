/*
 * Flox Engine
 * Developed by Evgenii Makarov (https://github.com/eeiaao)
 *
 * Copyright (c) 2025 Evgenii Makarov
 * Licensed under the MIT License. See LICENSE file in the project root for full
 * license information.
 */

#include "flox/engine/events/market_data_event.h"
#include "flox/engine/market_data_event_pool.h"

#include <gtest/gtest.h>

using namespace flox;

namespace
{

class DummyEvent : public IMarketDataEvent
{
 public:
  explicit DummyEvent(std::pmr::memory_resource*) {}

  void clear() override { cleared = true; }

  bool cleared = false;
};

}  // namespace

TEST(EventPoolTest, AcquireReturnsValidHandle)
{
  EventPool<DummyEvent, 3> pool;

  auto h = pool.acquire();
  EXPECT_TRUE(h.has_value());
  EXPECT_NE(h.value().get(), nullptr);
}

TEST(EventPoolTest, ReleasingReturnsToPool)
{
  EventPool<DummyEvent, 1> pool;

  auto h1 = pool.acquire();
  EXPECT_TRUE(h1.has_value());

  DummyEvent* raw = h1.value().get();
  h1.reset();  // handle released

  auto h2 = pool.acquire();
  EXPECT_EQ(h2.value().get(), raw);  // reused
}

TEST(EventPoolTest, InUseIsTrackedCorrectly)
{
  EventPool<DummyEvent, 3> pool;

  EXPECT_EQ(pool.inUse(), 0u);

  auto h1 = pool.acquire();
  EXPECT_EQ(pool.inUse(), 1u);

  auto h2 = pool.acquire();
  EXPECT_EQ(pool.inUse(), 2u);

  h1.reset();
  EXPECT_EQ(pool.inUse(), 1u);

  h2.reset();
  EXPECT_EQ(pool.inUse(), 0u);
}

TEST(EventHandleTest, UpcastRetainsReference)
{
  class Base : public IMarketDataEvent
  {
   public:
    explicit Base(std::pmr::memory_resource*) {}
  };

  class Derived : public Base
  {
   public:
    explicit Derived(std::pmr::memory_resource* r) : Base(r) {}
  };

  EventPool<Derived, 1> pool;
  auto handle = pool.acquire();
  auto upcasted = handle.value().upcast<Base>();

  EXPECT_NE(upcasted.get(), nullptr);
}

TEST(EventHandleTest, MoveReleasesPrevious)
{
  EventPool<DummyEvent, 1> pool;
  auto h1 = pool.acquire();
  DummyEvent* ptr = h1->get();
  EXPECT_EQ(ptr->refCount(), 1);

  {
    EventHandle<DummyEvent> h2 = std::move(*h1);
    EXPECT_EQ(h1->get(), nullptr);
    EXPECT_EQ(h2.get(), ptr);
    EXPECT_EQ(ptr->refCount(), 1);
  }

  EXPECT_EQ(pool.inUse(), 0u);
}

TEST(EventHandleTest, DoubleMoveStillValid)
{
  EventPool<DummyEvent, 1> pool;
  std::optional<EventHandle<DummyEvent>> h1 = pool.acquire();

  EXPECT_TRUE(h1.has_value());
  DummyEvent* ptr = h1->get();

  EventHandle<DummyEvent> h2 = std::move(*h1);
  EventHandle<DummyEvent> h3 = std::move(h2);

  EXPECT_EQ(h3.get(), ptr);
  EXPECT_EQ(pool.inUse(), 1u);

  h3.~EventHandle();
  EXPECT_EQ(pool.inUse(), 0u);
}

TEST(EventHandleTest, NullHandleIsSafe)
{
  std::optional<EventHandle<DummyEvent>> h;
  EXPECT_FALSE(h.has_value());

  // Should not crash:
  h.reset();  // reassignment of nullptr
}

TEST(EventPoolTest, ClearIsCalledOnRelease)
{
  EventPool<DummyEvent, 1> pool;

  auto h = pool.acquire();
  DummyEvent* raw = h.value().get();
  EXPECT_FALSE(raw->cleared);

  h.reset();

  auto reused = pool.acquire();
  EXPECT_TRUE(reused.value().get()->cleared);
}
