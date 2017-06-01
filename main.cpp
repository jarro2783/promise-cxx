#include "async.h"
#include "promise.hpp"

void foo(
  std::function<void(int)> resolve,
  std::function<void(const char*)> reject
)
{
  run_async([=] () {
    resolve(5);
  });
}

std::shared_ptr<promise::Promise<int, const char*>>
returnsAPromise(int v)
{
  std::cout << v << std::endl;
  return promise::Promise<int, const char*>::create(foo);
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
  }));

  auto pp = promise::Promise<int, const char*>::create(
    [] (auto resolve, auto reject)
    {
      resolve(returnsAPromise(6));
    }
  )->then(//std::function<int(int)>(
    [] (int x) {
      std::cout << x << std::endl;
      return 0;
    }//)
  );

  auto pv = promise::Promise<void, int>::create(
    [] (auto resolve, auto reject)
    {
      resolve();
    }
  );

  pv->then(//std::function<void()>(
    []() {
      std::cout << "It worked: " << std::endl;
    }//)
  );

  run_events();
  return 0;
}
