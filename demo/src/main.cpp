/*
 * Flox Engine
 * Developed by FLOX Foundation (https://github.com/FLOX-Foundation)
 *
 * Copyright (c) 2025 FLOX Foundation
 * Licensed under the MIT License. See LICENSE file in the project root for full
 * license information.
 */

#define NO_COUT 0

#include "demo/demo_builder.h"
#include "demo/latency_collector.h"

#include <chrono>
#include <iostream>
#include <thread>

demo::LatencyCollector collector;

int main()
{
#if NO_COUT
  std::cout.setstate(std::ios::badbit);
#endif

  demo::EngineConfig cfg{};
  demo::DemoBuilder builder(cfg);
  auto engine = builder.build();
  engine.start();

  std::this_thread::sleep_for(std::chrono::seconds(30));

  engine.stop();

#if NO_COUT
  std::cout.clear();
#endif

  std::cout << "demo finished" << std::endl;

#if NO_COUT
  collector.report();
#endif
}
