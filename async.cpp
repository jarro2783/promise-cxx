#include <deque>
#include <iostream>

#include "async.h"

namespace {
  std::deque<std::function<void()>> m_events;
}

void
run_async(std::function<void()> f)
{
  std::cout << "adding async" << std::endl;
  m_events.push_back(f);
}

void
run_events()
{
  while (m_events.size())
  {
    auto& event = m_events.front();
    std::cout << "Running async " << std::endl;
    event();
    m_events.pop_front();
  }
}
