/*
 * Flox Engine
 * Developed by Evgenii Makarov (https://github.com/eeiaao)
 *
 * Copyright (c) 2025 Evgenii Makarov
 * Licensed under the MIT License. See LICENSE file in the project root for full
 * license information.
 */

#pragma once

#include <memory>

namespace flox
{

class ISubsystem
{
 public:
  virtual ~ISubsystem() = default;
  virtual void start() = 0;
  virtual void stop() = 0;
};

// Generic wrapper that makes any component an ISubsystem
template <typename T>
class Subsystem : public ISubsystem
{
 public:
  explicit Subsystem(std::unique_ptr<T> impl) : _impl(std::move(impl)) {}

  void start() override
  {
    if constexpr (requires(T& t) { t.start(); })
    {
      _impl->start();
    }
  }

  void stop() override
  {
    if constexpr (requires(T& t) { t.stop(); })
    {
      _impl->stop();
    }
  }

  T* get() const { return _impl.get(); }

 private:
  std::unique_ptr<T> _impl;
};
}  // namespace flox