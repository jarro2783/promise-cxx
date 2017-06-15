#include <assert.h>

#include "async.h"
#include "promise.hpp"

namespace tests
{

using promise::Promise;

struct Exception
{
};

template <typename T, typename U>
void
check(T&& t, U&& u)
{
  if (t != u)
  {
    std::cout << t << " != " << u << std::endl;
  }
}

// It resolves to a value once
void
test_resolve()
{

  int v1 = 0;
  int v2 = 0;
  int v3 = 0;

  auto p = Promise<int, int>::create(
    [](auto resolve, auto reject)
    {
      resolve(1);
      resolve(2);
      reject(3);
    }
  )->then([&](int result) -> void
    {
      v1 = result;
    }
  );

  // With both parameters set
  auto pp = Promise<int, Exception>::create(
    [](auto resolve, auto reject)
    {
      resolve(42);
      resolve(43);
      reject(Exception());
    }
  )->then(
    [&](auto v)
    {
      v2 = v;
    },
    [&](auto e)
    {
      v2 = -1;
    }
  );

  //after a delay
  Promise<int, Exception>::create(
    [](auto resolve, auto reject)
    {
      run_async([=]() {
        resolve(3);
      });
    }
  )->then(
    [&](auto v)
    {
      v3 = v;
    }
  );

  run_events();

  check(v1, 1);
  check(v2, 42);
  check(v3, 3);
}

// Catches rejections
void
test_reject()
{
  int foo = 0;
  int v2 = 0;
  int v3 = 0;
  int v4 = 0;

  auto p = Promise<int, Exception>::create(
    [](auto resolve, auto reject)
    {
      throw Exception();
    }
  )->then(
    [](int x) {
      return 100;
    }
  )->then(
    [&](int y) {
      foo = y + 10;
    },
    [&](const Exception& z) {
      foo = -1;
    }
  );

  Promise<int, Exception>::create(
    [] (auto resolve, auto reject)
    {
      reject(Exception());
    }
  )->then(
    [](int v)
    {
      return 1;
    },
    [](Exception e)
    {
      return 2;
    }
  )->then(
    [&] (int v)
    {
      v2 = v;
    }
  );

  Promise<int, Exception>::create(
    [](auto resolve, auto reject)
    {
      resolve(5);
    }
  )->then(
    [](int v)
    {
      throw Exception();
      return 4;
    },
    [](Exception e)
    {
      return 5;
    }
  )->then(
    [&](int v)
    {
      v3 = v;
    }
  );

  //if onRejected throws, reject promise2 with reason
  Promise<int, Exception>::create(
    [] (auto resolve, auto reject)
    {
      reject(Exception());
    }
  )->then(
    [](int v){
      return 3;
    },
    [](Exception e){
      throw Exception();
      return 4;
    }
  )->then(
    [&](int v)
    {
      v4 = v;
    },
    [&](Exception e)
    {
      v4 = -1;
    }
  );

  run_events();

  check(foo, -1);
  check(v2, 2);
  check(v3, 5);
  check(v4, -1);
}

void
test_multiple()
{
  int v = 0;

  auto p = Promise<int, Exception>::create(
    [](auto resolve, auto reject)
    {
      resolve(1);
    }
  );
  p->then([&](int result){
    check(v, 0);
    ++v;
  });
  p->then([&](int result){
    check(v, 1);
    ++v;
  });

  run_events();

  check(v, 2);
}

}

int main(int argc, char** argv)
{
  tests::test_resolve();
  tests::test_reject();
  tests::test_multiple();
  return 0;
}
