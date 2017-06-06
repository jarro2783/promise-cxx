#ifndef PROMISE_EV_HPP_INCLUDED
#define PROMISE_EV_HPP_INCLUDED

#include <ev++.h>

#include <functional>
#include <list>

namespace
{
  class Instant
  {
    public:

    template <typename F>
    Instant(F&& func)
    : m_func(std::forward<F>(func))
    {
      m_timer.set(0, 0);
      m_timer.set(this);
      m_timer.start();
    }

    void
    operator()(ev::timer&, int)
    {
      m_timer.stop();
      m_func();
    }

    private:
    std::function<void()> m_func;
    ev::timer m_timer;
  };

  std::list<Instant> timers;
}


template <typename F>
void
run_async(F func)
{
  timers.emplace_front(func);
}

#endif
