#ifndef PROMISE_NET_NET_HPP_INCLUDED
#define PROMISE_NET_NET_HPP_INCLUDED

#include <promise/ev.hpp>
#include <promise.hpp>
#include <ev++.h>

namespace promise
{
  namespace net
  {
    ::Promise<int, int>
    connect(const std::string& address, int port);

    void run();
  }
}

#endif
