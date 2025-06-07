#pragma once

#include "demo/demo_connector.h"
#include "demo/demo_strategy.h"
#include "demo/simple_components.h"
#include "flox/aggregator/candle_aggregator.h"
#include "flox/engine/abstract_engine_builder.h"
#include "flox/engine/engine.h"
#include "flox/engine/engine_config.h"
#include "flox/engine/subsystem.h"

#include <memory>
#include <vector>

namespace demo
{
using namespace flox;

class DemoBuilder : public IEngineBuilder
{
 public:
  explicit DemoBuilder(const EngineConfig& cfg);
  std::unique_ptr<IEngine> build() override;

 private:
  EngineConfig _config;
};

}  // namespace demo
