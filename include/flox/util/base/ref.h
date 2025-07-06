/*
 * Flox Engine
 * Developed by FLOX Foundation (https://github.com/FLOX-Foundation)
 *
 * Copyright (c) 2025 FLOX Foundation
 * Licensed under the MIT License. See LICENSE file in the project root for full
 * license information.
 */

#pragma once

#include <cassert>
#include <cstring>
#include <utility>

#include "flox/util/memory/allocators/free_list_allocator.h"

namespace flox
{

template <typename T, typename = void>
struct RefFor;

template <typename Trait>
class Ref
{
 public:
  using VTable = typename Trait::VTable;

  template <typename T>
  static Ref from(T* ptr)
  {
    static constexpr VTable vt = Trait::template makeVTable<T>();
    return Ref{ptr, &vt};
  }

  template <typename OtherTrait>
  typename RefFor<OtherTrait>::type as() const
  {
    return RefFor<OtherTrait>::type::from(_ptr, _vtable->template as<OtherTrait>());
  }

  template <typename T>
  T& get() const
  {
    static_assert(!std::is_void_v<T>, "T must be a concrete type");
    return *static_cast<T*>(_ptr);
  }

  void* raw() const { return _ptr; }
  const VTable* vtable() const { return _vtable; }

 protected:
  Ref(void* ptr, const VTable* vtable) : _ptr(ptr), _vtable(vtable)
  {
  }

  void* _ptr = nullptr;
  const VTable* _vtable = nullptr;

  template <typename T>
  friend Ref<typename T::Trait> make();

  template <typename OtherTrait>
  friend class Ref;
};

template <typename DerivedT, typename TraitT>
struct RefBase : public Ref<TraitT>
{
  using VTable = typename TraitT::VTable;

  static DerivedT from(void* ptr, const VTable* vtable)
  {
    return DerivedT{ptr, vtable};
  }

  template <typename Impl>
  static DerivedT from(Impl* obj)
  {
    static const VTable vt = TraitT::template makeVTable<Impl>();
    return DerivedT{obj, &vt};
  }

 protected:
  RefBase(void* ptr, const VTable* vt)
      : Ref<TraitT>(ptr, vt) {}

  friend DerivedT;
};

template <typename T, typename... Args>
auto make(Args&&... args)
{
  using Trait = typename T::Trait;
  using RefType = typename RefFor<Trait>::type;
  using Alloc = typename T::Allocator;
  return Alloc::template create<RefType, T>(std::forward<Args>(args)...);
}

template <typename Trait, size_t N>
struct PoolAllocator
{
  template <typename RefType, typename ImplType, typename... Args>
  static RefType create(Args&&... args)
  {
    static thread_local FreeListAllocator<ImplType, N> pool;
    auto* obj = pool.allocate(std::forward<Args>(args)...);
    return RefType::from(obj);
  }
};

}  // namespace flox
