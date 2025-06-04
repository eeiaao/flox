/*
 * Flox Engine
 * Developed by Evgenii Makarov (https://github.com/eeiaao)
 *
 * Copyright (c) 2025 Evgenii Makarov
 * Licensed under the MIT License. See LICENSE file in the project root for full
 * license information.
 */

#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>

#include <memory_resource>

class ArenaResource : public std::pmr::memory_resource
{
 public:
  ArenaResource(void* buffer, std::size_t size,
                std::pmr::memory_resource* upstream = std::pmr::null_memory_resource())
      : _buffer(static_cast<std::byte*>(buffer)), _capacity(size), _upstream(upstream), _offset(0)
  {
  }

  void reset() noexcept { _offset = 0; }

 protected:
  void* do_allocate(std::size_t bytes, std::size_t alignment) override
  {
    std::size_t current = reinterpret_cast<std::size_t>(_buffer) + _offset;
    std::size_t aligned = align_up(current, alignment);
    std::size_t padding = aligned - current;
    std::size_t total = bytes + padding;

    if (_offset + total > _capacity)
    {
      return _upstream->allocate(bytes, alignment);  // fallback
    }

    void* ptr = _buffer + _offset + padding;
    _offset += total;
    return ptr;
  }

  void do_deallocate(void* /*p*/, std::size_t /*bytes*/, std::size_t /*alignment*/) override
  {
    // bump allocator does not free
  }

  bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override
  {
    return this == &other;
  }

 private:
  std::size_t align_up(std::size_t ptr, std::size_t alignment) const
  {
    return (ptr + alignment - 1) & ~(alignment - 1);
  }

  std::byte* _buffer;
  std::size_t _capacity;
  std::pmr::memory_resource* _upstream;
  std::size_t _offset;
};
