/*
 * Flox Engine
 * Developed by Evgenii Makarov (https://github.com/eeiaao)
 *
 * Copyright (c) 2025 Evgenii Makarov
 * Licensed under the MIT License. See LICENSE file in the project root for full
 * license information.
 */

#pragma once

#include <cstdint>

namespace flox
{

using SubscriberId = uint64_t;
enum class SubscriberMode
{
  PUSH,
  PULL
};

struct ISubscriber
{
  virtual SubscriberId id() const = 0;
  virtual SubscriberMode mode() const { return SubscriberMode::PUSH; }
};

}  // namespace flox