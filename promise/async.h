#ifndef ASYNC_H
#define ASYNC_H

#include <functional>

void
run_async(std::function<void()> f);

void
run_events();

#endif
