#include <assert.h>

#include "promise.hpp"

// It resolves to a value once
void
test_resolve()
{
  using promise::Promise;

  int value = 0;

  auto p = Promise<int, int>::create(
    [](auto resolve, auto reject)
    {
      resolve(1);
      resolve(2);
      reject(3);
    }
  )->then<void>([&](int result) -> void
    {
      value = result;
    }
  );

  run_events();

  assert(value == 1);
}

int main(int argc, char** argv)
{
  test_resolve();
  return 0;
}
