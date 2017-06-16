# Warning

This is extremely experimental, and may change at any moment.

# Promise C++

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

## Basics

Create a promise of type `T` which can be rejected with type `R` with:

```
promise::Promise<T, R>::create(foo)
```

where `foo` is a function taking two arguments:

```
void foo(std::function<void(T)>, std::function<void(R)>)
```

The first function is called to resolve the promise, and the second to
reject it.

The returned promise is actually a shared pointer; there is a type alias for
this as `::Promise<T, R>`.

The return promise has a `then` function, which when called with a function
that takes a `T`, and returns a `U`, returns a new promise with value type `U`.

## Limitations

It is impossible in C++ to catch all exceptions with a value, because
we wouldn't know the type of the exception. Therefore, a promise cannot
be rejected with an arbitrary reason. Arbitrary exceptions will cause
the program to exit unless caught somewhere else.
