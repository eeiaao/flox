# OrderExecutionBus

`OrderExecutionBus` is a compile-time alias of `EventBus` specialised for `OrderEvent`.  
Delivery mode is selected via the `USE_SYNC_ORDER_BUS` macro:

~~~cpp
#ifdef USE_SYNC_ORDER_BUS
using OrderExecutionBus = EventBus<OrderEvent, SyncPolicy<OrderEvent>>;
#else
using OrderExecutionBus = EventBus<OrderEvent, AsyncPolicy<OrderEvent>>;
#endif

using OrderExecutionBusRef =
    EventBusRef<OrderEvent, OrderExecutionBus::Queue>;
~~~

## Purpose
* Broadcast **order-lifecycle events** (accepted, filled, canceled, …) from executors
  to trackers, risk modules, PnL calculators, and loggers.

## Responsibilities

| Aspect        | Details                                                         |
|---------------|-----------------------------------------------------------------|
| **Fan-out**   | One lock-free SPSC queue per subscriber.                        |
| **Policy**    | `SyncPolicy` → deterministic, barrier-synchronised; `AsyncPolicy` → low-latency. |
| **Reference** | `OrderExecutionBusRef` lets producers publish without storing the full bus. |

## Notes
* Define/undefine `USE_SYNC_ORDER_BUS` **before** including the header to switch modes.
* The bus itself allocates no memory after construction; queues are pre-sized.
