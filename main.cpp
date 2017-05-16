#include "async.h"
#include "promise.hpp"

void foo(
  std::function<void(int)> resolve,
  std::function<void(const char*)> reject
)
{
  run_async([=] () { resolve(5); });
}

template <typename T>
int nothing(T&& t)
{
  return 0;
}

int main(int argc, char** argv)
{
  auto p = promise::Promise<int, const char*>::create(foo);
  p->then(std::function<int(int)>([] (int x) {
    std::cout << x << std::endl;
    return 42;
  }))->then(std::function<int(int)>([] (int x) {
    std::cout << x * 5 << std::endl;
    return 0;
  }));;

  run_events();
  return 0;
}
