#pragma once

#include "flox/execution/order.h"
#include "flox/util/base/ref.h"
#include "flox/util/meta/meta.h"

#include <string>

namespace flox
{

namespace concepts
{

template <typename T>
concept OrderValidator =
    requires(T t, const Order& order, std::string& reason) {
      { t.validate(order, reason) } -> std::same_as<bool>;
    };

}  // namespace concepts

namespace traits
{

struct OrderValidatorTrait
{
  struct VTable
  {
    bool (*validate)(void*, const Order&, std::string&);
  };

  template <typename T>
    requires concepts::OrderValidator<T>
  static constexpr VTable makeVTable()
  {
    return {
        .validate = meta::wrap<&T::validate>(),
    };
  }
};

}  // namespace traits

class OrderValidatorRef : public RefBase<OrderValidatorRef, traits::OrderValidatorTrait>
{
  using VTable = traits::OrderValidatorTrait::VTable;
  using Base = RefBase<OrderValidatorRef, traits::OrderValidatorTrait>;

 public:
  using Base::Base;

  bool validate(const Order& order, std::string& reason) const
  {
    return _vtable->validate(_ptr, order, reason);
  }
};
static_assert(concepts::OrderValidator<OrderValidatorRef>);

template <>
struct RefFor<traits::OrderValidatorTrait>
{
  using type = OrderValidatorRef;
};

}  // namespace flox
