#include <iostream>
#include <functional>
#include <memory>
#include <vector>

#include "async.h"

namespace promise
{
  // A promise that resolves to T
  template <typename T, typename Reason>
  class Promise : public std::enable_shared_from_this<Promise<T, Reason>>
  {
    public:
    //template <typename U>
    //std::shared_ptr<promise<U, void()>>
    //then(std::function<U(T)>)
    //{
    //}

    typedef std::function<void(T)> Resolved;
    typedef std::function<void(Reason)> Rejected;

    typedef std::function<void(Resolved, Rejected)> Handler;

    static
    std::shared_ptr<Promise<T, Reason>>
    create
    (
      Handler p
    )
    {
      using namespace std::placeholders;

      std::shared_ptr<Promise> ptr(new Promise);

      ptr->do_resolve(p,
        std::bind(std::mem_fn(&Promise::fulfill), ptr, _1),
        std::bind(std::mem_fn(&Promise::reject), ptr, _1));

      return ptr;
    }

    template <typename U, typename URea>
    std::shared_ptr<Promise<U, URea>>
    then(std::function<U(T)> on_ful, std::function<URea(Reason)> on_rej)
    {
      auto self = this->shared_from_this();

      return Promise<U, URea>::create([=] (
        auto resolve,
        auto reject
      ) {
        return this->done([=] (T result) {
            return resolve(on_ful(result));
          },
          [=] (Reason reason) {
            return resolve(on_rej(reason));
          }
        );
      });
    }

    private:
    Promise()
    : m_state(0)
    {
    }

    void fulfill(T value)
    {
      m_state = 1;
      m_value = value;

      for (auto& h: m_handlers)
      {
        handle(h);
      }
    }

    void
    handle(std::pair<Resolved, Rejected> handler)
    {
      switch(m_state)
      {
        case 0:
        m_handlers.push_back(handler);
        break;
        case 1:
        handler.first(m_value);
        break;
        case 2:
        handler.second(m_reason);
        break;
      }
    }

    template <typename F, typename R>
    void
    done(F f, R r)
    {
      run_async([=] () {
        this->handle(std::make_pair(f, r));
      });
    }

    void reject(Reason reason)
    {
      m_state = 2;
      m_reason = reason;

      for (auto& h: m_handlers)
      {
        handle(h);
      }
    }

    void
    resolve(T value)
    {
      try
      {
        fulfill(value);
      } catch (const Reason& r)
      {
        reject(r);
      }
    }

    void
    do_resolve(Handler fn, Resolved resolved, Rejected rejected)
    {
      auto done = std::make_shared<bool>(false);

      fn([done, resolved] (T value) {
        if (*done) return;
        *done = true;
        resolved(value);
      }, [done, rejected] (Reason reason) {
        if (*done) return;
        *done = true;
        rejected(reason);
      });
    }

    int m_state;
    T m_value;
    Reason m_reason;
    std::vector<std::pair<Resolved, Rejected>> m_handlers;
  };

}
