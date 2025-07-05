# OrderExecutor (concept / trait / ref)

Defines the contract for components that **submit, cancel, and replace** orders,
plus a type-erased handle for zero-cost invocation.

~~~cpp
// Concept
template <typename T>
concept OrderExecutor = requires(T ex, const Order& o, OrderId id) {
  ex.submitOrder(o);
  ex.cancelOrder(id);
  ex.replaceOrder(id, o);
};
~~~

| Piece | Responsibility |
|-------|----------------|
| **`OrderExecutorTrait`** | Generates a static v-table with three wrapped member functions. |
| **`OrderExecutorRef`**   | Two-pointer handle `{void*, VTable*}` forwarding calls without virtual inheritance. |

## OrderExecutorRef API
````cpp
void submitOrder (const Order&)           const; // send new order
void cancelOrder (OrderId)                const; // cancel by id
void replaceOrder(OrderId, const Order&)  const; // amend existing
````

## Purpose

* Abstract away the concrete execution back-end (real exchange, simulator, mock)
  while keeping a uniform interface for strategies and risk managers.
* Maintain **zero overhead**: one pointer indirection, no RTTI, no `std::function`.

## Notes

* Implement a concrete executor that satisfies `concepts::OrderExecutor`,
  then wrap it in an `OrderExecutorRef` for dependency injection.
