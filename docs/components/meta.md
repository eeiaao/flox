# `meta::wrap`

Tiny compile-time utility that converts a **member-function pointer** into a plain C-function pointer with a `void*`/`const void*` self parameter — ideal for populating static v-tables without virtual inheritance.

~~~cpp
namespace flox::meta {

template <auto Method>
constexpr auto wrap();

} // namespace flox::meta
~~~

## How It Works

1. `WrapImpl<Method>` is specialised for mutable vs. `const` methods.  
2. Generates a lambda that:
   * casts the `void*` pointer back to the concrete `Class*`;
   * invokes the member function `Method` forwarding all arguments.
3. `wrap<Method>()` returns this lambda as a **function pointer** suitable for v-tables.

## Example

````cpp
struct Foo {
  int bar(double);
  int baz(double) const;
};

using BarFn = int (*)(void*,  double);
using BazFn = int (*)(const void*, double);

BarFn barPtr = meta::wrap<&Foo::bar>();
BazFn bazPtr = meta::wrap<&Foo::baz>();

Foo f;
int x = barPtr(&f, 1.0);   // calls f.bar(1.0)
int y = bazPtr(&f, 2.0);   // calls f.baz(2.0)
````

## Purpose

* Build **static v-tables** for type-erased handles (`RefBase<Trait>` pattern) with zero runtime cost.
* Avoid `std::function` and virtual dispatch; the wrapper pointer is resolved at compile time.

## Notes

* Works with both non-`const` and `const` member functions.
* Preserves return type and argument list through template deduction.
