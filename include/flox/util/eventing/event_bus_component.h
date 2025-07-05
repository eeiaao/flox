#pragma once

#include "flox/engine/event_dispatcher.h"
#include "flox/engine/tick_barrier.h"
#include "flox/engine/tick_guard.h"
#include "flox/util/memory/pool.h"
#include "flox/util/meta/meta.h"

namespace flox
{

template <typename T>
struct ListenerType
{
  using type = typename T::Listener;
};

template <typename T>
struct ListenerType<pool::Handle<T>>
{
  using type = typename T::Listener;
};

namespace concepts
{

template <typename T, typename Event, typename Queue>
concept EventBus = requires(T t) {
  { t.publish(std::declval<Event>()) };
  { t.subscribe(std::declval<typename ListenerType<Event>::type>()) };
  { t.getQueue(SubscriberId{}) } -> std::same_as<std::optional<std::reference_wrapper<Queue>>>;
  { t.currentTickId() } -> std::same_as<uint64_t>;
  { t.enableDrainOnStop() } -> std::same_as<void>;
};

}  // namespace concepts

template <typename Event>
struct SyncPolicy
{
  using QueueItem = std::pair<Event, TickBarrier*>;
  static QueueItem makeItem(Event ev, TickBarrier* barrier) { return {ev, barrier}; }
  static void dispatch(const QueueItem& item, typename ListenerType<Event>::type& listener)
  {
    TickGuard guard(*item.second);
    EventDispatcher<Event>::dispatch(item.first, listener);
  }
};

template <typename Event>
struct AsyncPolicy
{
  using QueueItem = Event;
  static QueueItem makeItem(Event ev, void*) { return ev; }
  static void dispatch(const QueueItem& item, typename ListenerType<Event>::type& listener)
  {
    EventDispatcher<Event>::dispatch(item, listener);
  }
};

template <typename Event, typename Policy, size_t QueueSize>
class EventBus;

namespace traits
{

template <typename Event, typename Queue>
struct EventBusTrait
{
  struct VTable
  {
    const SubsystemTrait::VTable* subsystem;

    void (*publish)(void*, Event);
    void (*subscribe)(void*, typename ListenerType<Event>::type);
    std::optional<std::reference_wrapper<Queue>> (*getQueue)(const void*, SubscriberId);
    uint64_t (*currentTickId)(const void*);
    void (*enableDrainOnStop)(void*);

    template <typename Trait>
    const typename Trait::VTable* as() const
    {
      if constexpr (std::is_same_v<Trait, SubsystemTrait>)
        return subsystem;
      else
        static_assert(sizeof(Trait) == 0, "Trait not supported by this VTable");
    }
  };

  template <typename T>
    requires concepts::EventBus<T, Event, Queue>
  static const VTable makeVTable()
  {
    static const auto subsystem = SubsystemTrait::makeVTable<T>();
    return VTable{
        .subsystem = &subsystem,
        .publish = meta::wrap<&T::publish>(),
        .subscribe = meta::wrap<&T::subscribe>(),
        .getQueue = meta::wrap<&T::getQueue>(),
        .currentTickId = meta::wrap<&T::currentTickId>(),
        .enableDrainOnStop = meta::wrap<&T::enableDrainOnStop>()};
  }
};  // namespace traits

}  // namespace traits

template <typename Event, typename Queue>
class EventBusRef : public RefBase<EventBusRef<Event, Queue>, traits::EventBusTrait<Event, Queue>>
{
  using VTable = traits::EventBusTrait<Event, Queue>::VTable;
  using Base = RefBase<EventBusRef<Event, Queue>, traits::EventBusTrait<Event, Queue>>;

 public:
  using Listener = typename ListenerType<Event>::type;

  using Base::Base;

  void start() const { this->_vtable->subsystem->start(this->_ptr); }
  void stop() const { this->_vtable->subsystem->stop(this->_ptr); }

  void publish(Event ev) const { this->_vtable->publish(this->_ptr, std::move(ev)); }

  void subscribe(Listener listener) const
  {
    this->_vtable->subscribe(this->_ptr, std::move(listener));
  }

  std::optional<std::reference_wrapper<Queue>> getQueue(SubscriberId id) const
  {
    return this->_vtable->getQueue(this->_ptr, id);
  }

  uint64_t currentTickId() const
  {
    return this->_vtable->currentTickId(this->_ptr);
  }

  void enableDrainOnStop() const
  {
    this->_vtable->enableDrainOnStop(this->_ptr);
  }
};

template <typename Event, typename Queue>
struct RefFor<traits::EventBusTrait<Event, Queue>>
{
  using type = EventBusRef<Event, Queue>;
};

}  // namespace flox