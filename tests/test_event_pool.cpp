/*
 * Flox Engine
 * Developed by Evgenii Makarov (https://github.com/eeiaao)
 *
 * Copyright (c) 2025 Evgenii Makarov
 * Licensed under the MIT License. See LICENSE file in the project root for full
 * license information.
 */

#include "flox/util/memory/pool.h"

#include <gtest/gtest.h>

using namespace flox;

namespace
{

class DummyEvent : public pool::PoolableBase<DummyEvent>
{
 public:
  explicit DummyEvent(std::pmr::memory_resource*) {}

  void clear() { cleared = true; }

  bool cleared = false;
};
static_assert(concepts::Poolable<DummyEvent>);

}  // namespace

TEST(PoolTest, AcquireReturnsValidHandle)
{
  pool::Pool<DummyEvent, 3> pool;

  auto h = pool.acquire();
  EXPECT_TRUE(h.has_value());
  EXPECT_NE(h.value().get(), nullptr);
}

TEST(PoolTest, ReleasingReturnsToPool)
{
  pool::Pool<DummyEvent, 1> pool;

  auto h1 = pool.acquire();
  EXPECT_TRUE(h1.has_value());

  DummyEvent* raw = h1.value().get();
  h1.reset();  // handle released

  auto h2 = pool.acquire();
  EXPECT_TRUE(h2.has_value());
  EXPECT_EQ(h2.value().get(), raw);  // reused
}

TEST(PoolTest, InUseIsTrackedCorrectly)
{
  pool::Pool<DummyEvent, 3> pool;

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

TEST(PoolHandleTest, UpcastRetainsReference)
{
  class Base : public pool::PoolableBase<Base>
  {
   public:
    explicit Base(std::pmr::memory_resource*) {}
  };

  class Derived : public Base
  {
   public:
    explicit Derived(std::pmr::memory_resource* r) : Base(r) {}
  };

  pool::Pool<Derived, 1> pool;
  auto handle = pool.acquire();
  auto upcasted = handle.value().upcast<Base>();

  EXPECT_NE(upcasted.get(), nullptr);
}

TEST(PoolHandleTest, MoveReleasesPrevious)
{
  pool::Pool<DummyEvent, 1> pool;
  auto h1 = pool.acquire();
  DummyEvent* ptr = h1->get();
  EXPECT_EQ(ptr->refCount(), 1);

  {
    pool::Handle<DummyEvent> h2 = std::move(*h1);
    EXPECT_EQ(h1->get(), nullptr);
    EXPECT_EQ(h2.get(), ptr);
    EXPECT_EQ(ptr->refCount(), 1);
  }

  EXPECT_EQ(pool.inUse(), 0u);
}

TEST(PoolHandleTest, DoubleMoveStillValid)
{
  using namespace flox::pool;

  Pool<DummyEvent, 1> pool;

  {
    std::optional<Handle<DummyEvent>> h1 = pool.acquire();

    EXPECT_TRUE(h1.has_value());
    DummyEvent* ptr = h1->get();

    Handle<DummyEvent> h2 = std::move(*h1);
    Handle<DummyEvent> h3 = std::move(h2);

    EXPECT_EQ(h3.get(), ptr);
    EXPECT_EQ(pool.inUse(), 1u);
  }

  EXPECT_EQ(pool.inUse(), 0u);
}

TEST(PoolHandleTest, NullHandleIsSafe)
{
  std::optional<pool::Handle<DummyEvent>> h;
  EXPECT_FALSE(h.has_value());

  // Should not crash:
  h.reset();  // reassignment of nullptr
}

TEST(PoolTest, ClearIsCalledOnRelease)
{
  pool::Pool<DummyEvent, 1> pool;

  auto h = pool.acquire();
  DummyEvent* raw = h.value().get();
  EXPECT_FALSE(raw->cleared);

  h.reset();

  auto reused = pool.acquire();
  EXPECT_TRUE(reused.value().get()->cleared);
}
