# MultiExecutionListener

Aggregates several `OrderExecutionListener` instances and forwards every order-lifecycle callback to them.

~~~cpp
class MultiExecutionListener {
public:
  explicit MultiExecutionListener(SubscriberId id);

  void start();
  void stop();

  SubscriberId  id()   const;
  SubscriberMode mode() const;          // always PUSH

  void addListener(OrderExecutionListenerRef l);

  // Order-lifecycle fan-out
  void onOrderSubmitted     (const Order&);
  void onOrderAccepted      (const Order&);
  void onOrderPartiallyFilled(const Order&, Quantity);
  void onOrderFilled        (const Order&);
  void onOrderCanceled      (const Order&);
  void onOrderExpired       (const Order&);
  void onOrderRejected      (const Order&, const std::string& reason);
  void onOrderReplaced      (const Order& oldOrd, const Order& newOrd);
};
~~~

## Purpose
* Act as a **broadcast hub** so multiple components (PNL tracker, logger, GUI) can receive execution events without each being subscribed separately.

## Behaviour
| Action            | Detail                                              |
|-------------------|-----------------------------------------------------|
| `addListener()`   | Stores unique listeners; duplicates (by `id()`) are ignored. |
| Callbacks         | Loops through `_listeners` and forwards the event.  |
| Threading         | Declares `SubscriberMode::PUSH`; intended for EventBus push threads. |

## Notes
* No dynamic removal; recreate the object if listener set changes.
* `static_assert` ensures the class itself satisfies `OrderExecutionListener` concept.
