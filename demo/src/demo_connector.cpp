#include "demo/demo_connector.h"

#include <chrono>
#include <thread>
#include "demo/latency_collector.h"

namespace demo
{

DemoConnector::DemoConnector(const std::string& id, SymbolId symbol, MarketDataBus& bus)
    : _id(id), _symbol(symbol), _bus(bus)
{
}

void DemoConnector::start()
{
  if (_running.exchange(true))
    return;
  _thread = std::thread(&DemoConnector::run, this);
}

void DemoConnector::stop()
{
  if (!_running.exchange(false))
    return;
  if (_thread.joinable())
    _thread.join();
}

void DemoConnector::run()
{
  Price price = Price::fromDouble(100.0);
  std::uniform_real_distribution<double> step(-0.2, 0.6);
  std::uniform_real_distribution<double> qtyDist(0.5, 2.0);
  std::bernoulli_distribution sideDist(0.5);
  auto nextBookUpdate = std::chrono::steady_clock::now();

  auto lastSpike = std::chrono::steady_clock::now();
  bool spikeActive = false;
  int spikeSteps = 0;

  while (_running.load())
  {
    auto now = std::chrono::steady_clock::now();

    if (!spikeActive && now - lastSpike >= std::chrono::seconds(3))
    {
      spikeActive = true;
      spikeSteps = 10;
      lastSpike = now;
      std::cout << "[demo] price spike starting\n";
    }

    if (spikeActive && spikeSteps > 0)
    {
      price = Price::fromDouble(price.toDouble() + 200.0 + 2.0);
      --spikeSteps;
    }
    else if (spikeActive && spikeSteps == 0)
    {
      price = Price::fromDouble(100.0);
      spikeActive = false;
    }
    else
    {
      price = Price::fromDouble(std::max(1.0, price.toDouble() + step(_rng)));
    }

    TradeEvent te{};
    te.trade.symbol = _symbol;
    te.trade.price = price;
    te.trade.quantity = Quantity::fromDouble(qtyDist(_rng));
    te.trade.isBuy = sideDist(_rng);
    te.trade.timestamp = std::chrono::high_resolution_clock::now();
    {
      MEASURE_LATENCY(LatencyCollector::BusPublish);
      _bus.publish(te);
    }

    if (now >= nextBookUpdate)
    {
      auto evOpt = _bookPool.acquire();
      if (evOpt)
      {
        auto& ev = *evOpt;
        ev->update.symbol = _symbol;
        ev->update.type = BookUpdateType::SNAPSHOT;

        double center = price.toDouble();
        for (int i = 0; i < 3; ++i)
        {
          ev->update.bids.push_back({Price::fromDouble(center - 0.01 * (i + 1)),
                                     Quantity::fromDouble(qtyDist(_rng))});
          ev->update.asks.push_back({Price::fromDouble(center + 0.01 * (i + 1)),
                                     Quantity::fromDouble(qtyDist(_rng))});
        }

        {
          MEASURE_LATENCY(LatencyCollector::BusPublish);
          _bus.publish(std::move(ev));
        }
      }

      nextBookUpdate = now + std::chrono::milliseconds(1);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
}

}  // namespace demo
