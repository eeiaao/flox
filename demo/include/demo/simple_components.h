#pragma once

#include "flox/execution/abstract_executor.h"
#include "flox/execution/bus/order_execution_bus.h"
#include "flox/killswitch/abstract_killswitch.h"
#include "flox/metrics/abstract_execution_tracker.h"
#include "flox/metrics/abstract_pnl_tracker.h"
#include "flox/position/position_manager.h"
#include "flox/risk/abstract_risk_manager.h"
#include "flox/sink/storage_sink.h"
#include "flox/validation/abstract_order_validator.h"

#include <chrono>
#include <iostream>
#include <string>
#include <thread>

namespace demo
{
using namespace flox;

class ConsoleExecutionTracker : public IExecutionTracker
{
 public:
  void onOrderSubmitted(const Order& order, std::chrono::steady_clock::time_point ts) override
  {
    std::cout << "[tracker] submitted " << order.id << " at " << ts.time_since_epoch().count() << '\n';
  }
  void onOrderFilled(const Order& order, std::chrono::steady_clock::time_point ts) override
  {
    std::cout << "[tracker] filled " << order.id << " after " << ts.time_since_epoch().count() << '\n';
  }
  void onOrderRejected(const Order& order, const std::string& reason,
                       std::chrono::steady_clock::time_point) override
  {
    std::cout << "[tracker] rejected " << order.id << " reason=" << reason << '\n';
  }
};

class SimplePnLTracker : public IPnLTracker
{
 public:
  void onOrderFilled(const Order& order) override
  {
    double value = order.price.toDouble() * order.quantity.toDouble();
    _pnl += (order.side == Side::BUY ? -value : value);
    std::cout << "[pnl] " << _pnl << '\n';
  }

 private:
  double _pnl = 0.0;
};

class StdoutStorageSink : public StorageSink
{
 public:
  void start() override {}
  void stop() override {}
  void store(const Order& order) override { std::cout << "[storage] order " << order.id << '\n'; }
};

class SimpleOrderValidator : public IOrderValidator
{
 public:
  bool validate(const Order& order, std::string& reason) const override
  {
    if (order.quantity.raw() <= 0)
    {
      reason = "qty";
      return false;
    }
    if (order.price.raw() <= 0)
    {
      reason = "price";
      return false;
    }
    return true;
  }
};

class SimpleKillSwitch : public IKillSwitch
{
 public:
  void check(const Order& order) override
  {
    if (order.price.toDouble() > 200.0)
      trigger("price too high");
  }
  void trigger(const std::string& r) override
  {
    _triggered = true;
    _reason = r;
  }
  bool isTriggered() const override { return _triggered; }
  std::string reason() const override { return _reason; }

 private:
  bool _triggered = false;
  std::string _reason;
};

class SimpleRiskManager : public IRiskManager
{
 public:
  explicit SimpleRiskManager(SimpleKillSwitch* ks) : _ks(ks) {}
  void start() override {}
  void stop() override {}
  bool allow(const Order& order) const override
  {
    _ks->check(order);
    return !_ks->isTriggered();
  }

 private:
  SimpleKillSwitch* _ks;
};

class SimpleOrderExecutor : public IOrderExecutor
{
 public:
  explicit SimpleOrderExecutor(OrderExecutionBus& bus) : _bus(bus) {}
  void start() override { _bus.start(); }
  void stop() override { _bus.stop(); }

  void submitOrder(const Order& order) override
  {
    auto now = std::chrono::steady_clock::now();
    if (auto tracker = getExecutionTracker())
      tracker->onOrderSubmitted(order, now);

    // accepted
    OrderEvent ev{OrderEventType::ACCEPTED};
    ev.order = order;
    _bus.publish(ev);

    // simulate partial fill
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    Quantity half = Quantity::fromRaw(order.quantity.raw() / 2);
    ev = {};
    ev.type = OrderEventType::PARTIALLY_FILLED;
    ev.order = order;
    ev.fillQty = half;
    _bus.publish(ev);
    if (_pnlTracker)
    {
      Order part = order;
      part.quantity = half;
      _pnlTracker->onOrderFilled(part);
    }
    if (_posMgr)
    {
      Order part = order;
      part.quantity = half;
      _posMgr->onOrderFilled(part);
    }

    // simulate replace
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    Order newOrder = order;
    newOrder.price += Price::fromDouble(0.1);
    ev = {};
    ev.type = OrderEventType::REPLACED;
    ev.order = order;
    ev.newOrder = newOrder;
    _bus.publish(ev);

    // final fill of remaining quantity
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    ev = {};
    ev.type = OrderEventType::FILLED;
    ev.order = newOrder;
    ev.fillQty = order.quantity - half;
    _bus.publish(ev);
    if (_pnlTracker)
    {
      Order rest = newOrder;
      rest.quantity = order.quantity - half;
      _pnlTracker->onOrderFilled(rest);
    }
    if (_storage)
      _storage->store(newOrder);
    if (_posMgr)
    {
      Order rest = newOrder;
      rest.quantity = order.quantity - half;
      _posMgr->onOrderFilled(rest);
    }
  }
  void cancelOrder(OrderId) override {}
  void replaceOrder(OrderId, const Order&) override {}

  void setPnLTracker(IPnLTracker* t) { _pnlTracker = t; }
  void setStorageSink(StorageSink* s) { _storage = s; }
  void setPositionManager(PositionManager* p) { _posMgr = p; }

 private:
  OrderExecutionBus& _bus;
  IPnLTracker* _pnlTracker{nullptr};
  StorageSink* _storage{nullptr};
  PositionManager* _posMgr{nullptr};
};

}  // namespace demo
