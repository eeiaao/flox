#include "demo/demo_connector.h"

#include <chrono>

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
  std::uniform_real_distribution<double> step(-0.5, 0.5);
  auto nextBookUpdate = std::chrono::steady_clock::now();

  while (_running.load())
  {
    price = Price::fromDouble(price.toDouble() + step(_rng));

    TradeEvent te{};
    te.trade.symbol = _symbol;
    te.trade.price = price;
    te.trade.quantity = Quantity::fromDouble(1.0);
    te.trade.isBuy = true;
    te.trade.timestamp = std::chrono::system_clock::now();
    _bus.publish(te);

    if (std::chrono::steady_clock::now() >= nextBookUpdate)
    {
      auto evOpt = _bookPool.acquire();
      if (evOpt)
      {
        auto& ev = *evOpt;
        ev->update.symbol = _symbol;
        ev->update.type = BookUpdateType::SNAPSHOT;
        ev->update.bids.push_back({price - Price::fromDouble(0.5), Quantity::fromDouble(2)});
        ev->update.asks.push_back({price + Price::fromDouble(0.5), Quantity::fromDouble(2)});
        _bus.publish(std::move(ev));
      }
      nextBookUpdate = std::chrono::steady_clock::now() + std::chrono::seconds(1);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
}

}  // namespace demo
