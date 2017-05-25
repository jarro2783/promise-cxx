#include <iostream>
#include <functional>
#include <memory>
#include <vector>

#include "async.h"

namespace promise
{
  template <typename T>
  class PromiseValue
  {
    public:

    PromiseValue() = default;

    template <typename U>
    PromiseValue(U&& u)
    : m_value(std::forward<U>(u))
    {
    }

    operator T() {
      return m_value;
    }

    private:
    T m_value;
  };

  template <>
  class PromiseValue<void>
  {
  };

  // A promise that resolves to T, and rejects with Reason
  template <typename T, typename Reason>
  class Promise : public std::enable_shared_from_this<Promise<T, Reason>>
  {
    public:

    typedef std::function<void(T)> Resolved;
    typedef std::function<void(Reason)> Rejected;

    typedef std::function<void(Resolved, Rejected)> Handler;

    template <typename U>
    static
    std::shared_ptr<Promise<T, Reason>>
    create (U&& p)
    {
      using namespace std::placeholders;

      std::shared_ptr<Promise> ptr(new Promise);

      p([ptr](auto value) {
        Promise::resolver(ptr, value);
      }, [ptr](auto reason) {
        ptr->reject(reason);
      });

      return ptr;
    }

    static
    void
    resolver(std::shared_ptr<Promise> promise, const T& value)
    {
      promise->fulfill(value);
    }

    template <typename U, typename UReason>
    static
    void
    resolver(
      std::shared_ptr<Promise> promise,
      std::shared_ptr<Promise<U, UReason>> value
    )
    {
      // TODO:
      //this actually returns a promise that doesn't return
      //a value, and we don't even care about its value
      //so this is actually a void function
      //
      // This resolves as type T when "value" resolves with type U
      // obviously U has to be convertible to T
      value->then(std::function<T(U)>([promise] (auto result) {
        promise->fulfill(result);
        return result;
      }));
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
        case 2:
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

      if (m_handleFulfilled.size() > 0)
      {
        auto ptr = this->shared_from_this();
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

    int m_state;
    PromiseValue<T> m_value;
    Reason m_reason;
    std::vector<Resolved> m_handleFulfilled;
    std::vector<Rejected> m_handleRejected;
  };

}
