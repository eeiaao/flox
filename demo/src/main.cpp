#include "demo/demo_builder.h"

#include <chrono>
#include <iostream>
#include <thread>

using namespace demo;

int main()
{
  EngineConfig cfg{};
  DemoBuilder builder(cfg);
  auto engine = builder.build();
  engine->start();
  std::this_thread::sleep_for(std::chrono::seconds(5));
  engine->stop();
  std::cout << "demo finished" << std::endl;
}
