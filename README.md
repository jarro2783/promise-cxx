= Warning =

This is extremely experimental, and may change at any moment.

= Promise C++ =

This is a promise implementation in C++. It has been adapted to work in
C++, and so is missing some functionality from the javascript implemantation.
The advantage of using C++ is that everything is strongly typed.

For example:
```
auto p = Promise<int, Exception>::create(foo)
  ->then(bar);
```

will do two things:

1. Pass a resolve function to foo that takes an `int`, and a reject function
   that takes an `Exception`.
2. The function `bar` must take an `int`, and the variable `p` will be a 
   promise whose resolve type is the return type of `bar`.

== Basics ==



== Limitations ==

It is impossible in C++ to catch all exceptions with a value, because
we wouldn't know the type of the exception. Therefore, a promise cannot
be rejected with an arbitrary reason.
