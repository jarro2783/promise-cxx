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

    template <typename URea, typename U>
    std::shared_ptr<Promise<U, URea>>
    then(std::function<U(T)> on_ful, std::function<U(Reason)> on_rej)
    {
      auto self = this->shared_from_this();

      return Promise<U, URea>::create(
      [=] (
        auto resolve,
        auto reject
      ) {
        run_async(
          handle_fulfilled(
            [=] (T result) {
              try {
                return resolve(on_ful(result));
              } catch (...) {
                // TODO: fix these
                return reject();
              }
            }
          )
        );
        run_async(
          handle_fulfilled(
            [=] (Reason reason) {
              try {
                return resolve(on_rej(reason));
              } catch (...) {
                return reject();
              }
            }
          )
        );
      });
    }

    template <typename U>
    std::shared_ptr<Promise<U, Reason>>
    then(std::function<U(T)> fulfilled)
    {
      auto self = this->shared_from_this();

      return Promise<U, Reason>::create(
      [=] (
        auto resolve,
        auto reject
      ) {
        run_async([=]() {
          this->handle_fulfilled(
            [=] (T result) {
              try {
                return resolve(fulfilled(result));
              } catch (...) {
                // TODO: fix these
                return reject(Reason());
              }
            }
          );
        });
        run_async([=]() {
          this->handle_rejected(
            [=] (Reason reason) {
              return reject(reason);
            }
          );
        });
      });
    }

    private:
    Promise()
    : m_state(0)
    {
    }

    void
    handle_fulfilled(std::function<void(T)> handler)
    {
      switch (m_state)
      {
        case 0:
        m_handleFulfilled.push_back(handler);
        break;
        case 1:
        handler(m_value);
        break;
      }
    }

    void
    handle_rejected(std::function<void(Reason)> handler)
    {
      switch(m_state)
      {
        case 0:
        m_handleRejected.push_back(handler);
        break;
        case 1:
        handler(m_reason);
        break;
      }
    }

    void
    fulfill(T value)
    {
      if (m_state != 0)
      {
        return;
      }

      m_state = 1;
      m_value = value;

      auto ptr = this->shared_from_this();

      if (m_handleFulfilled.size() > 0)
      {
        run_async([ handlers{std::move(m_handleFulfilled)}, ptr ]() {
          for (auto& handler : handlers)
          {
            handler(ptr->m_value);
          }
        });
      }
    }

    void reject(Reason reason)
    {
      if (m_state != 0)
      {
        return;
      }

      m_state = 2;
      m_reason = reason;

      auto ptr = this->shared_from_this();

      if (m_handleRejected.size() > 0)
      {
        run_async([ handlers{std::move(m_handleRejected)}, ptr ]() {
          for (auto& handler: handlers)
          {
            handler(ptr->m_reason);
          }
        });
      }

    }

    void
    do_resolve(Handler fn, Resolved resolved, Rejected rejected)
    {
      fn([this, resolved] (T value) {
        resolved(value);
      }, [this, rejected] (Reason reason) {
        rejected(reason);
      });
    }

    int m_state;
    T m_value;
    Reason m_reason;
    std::vector<Resolved> m_handleFulfilled;
    std::vector<Rejected> m_handleRejected;
  };

}
