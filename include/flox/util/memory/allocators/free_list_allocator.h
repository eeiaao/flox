#pragma once

#include <cstddef>
#include <memory>

namespace flox
{

template <typename T, std::size_t Capacity>
class FreeListAllocator
{
 public:
  FreeListAllocator()
  {
    for (std::size_t i = 0; i < Capacity - 1; ++i)
      _next[i] = i + 1;
    _next[Capacity - 1] = invalid;
    _freeHead = 0;
  }

  template <typename... Args>
  T* allocate(Args&&... args)
  {
    if (_freeHead == invalid)
      return nullptr;

    std::size_t index = _freeHead;
    _freeHead = _next[index];
    T* ptr = reinterpret_cast<T*>(&_storage[index]);
    std::construct_at(ptr, std::forward<Args>(args)...);
    return ptr;
  }

  void deallocate(T* ptr)
  {
    std::size_t index = reinterpret_cast<std::byte*>(ptr) - reinterpret_cast<std::byte*>(&_storage[0]);
    index /= sizeof(T);
    std::destroy_at(ptr);
    _next[index] = _freeHead;
    _freeHead = index;
  }

 private:
  static constexpr std::size_t invalid = static_cast<std::size_t>(-1);
  alignas(T) std::byte _storage[Capacity][sizeof(T)];
  std::size_t _next[Capacity];
  std::size_t _freeHead = invalid;
};

}  // namespace flox
