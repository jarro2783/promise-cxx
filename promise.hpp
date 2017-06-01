#include <iostream>
#include <functional>
#include <memory>
#include <vector>

#include "async.h"

namespace promise
{
  template <typename T>
  class Value
  {
    public:

    Value() = default;

    template <typename U>
    Value(U&& u)
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
  class Value<void>
  {
  };

  template <typename T>
  struct PromiseTypes
  {
    typedef std::function<void(T)> Resolved;
  };

  template <>
  struct PromiseTypes<void>
  {
    typedef std::function<void()> Resolved;
  };

  template <typename T, typename Returns>
  struct ThenTypes
  {
    typedef std::function<Returns(T)> Handler;
  };

  template <typename Returns>
  struct ThenTypes<void, Returns>
  {
    typedef std::function<Returns()> Handler;
  };

  template <typename F, typename V>
  struct FunctionReturn
  {
    typedef decltype(std::declval<F>()(std::declval<V>())) type;
  };

  template <typename F>
  struct FunctionReturn<F, void>
  {
    typedef decltype(std::declval<F>()()) type;
  };

  template <typename T, typename R>
  class Promise;

  template <typename H>
  void
  run_handler(H&& handler, Value<void>)
  {
    run_async([=](){
      handler();
    });
  }

  template <typename H, typename V>
  void
  run_handler(H&& handler, V& v)
  {
    run_async([handler, &v]() {
      handler(v);
    });
  }

  template <typename T, typename U, typename V>
  void
  value_resolver
  (
    const T& resolve,
    const U& fulfilled,
    const V& value
  )
  {
    resolve(fulfilled(value));
  }

  template <typename T, typename V>
  void
  value_resolver
  (
    std::function<void()> resolve,
    T fulfilled,
    const V& value
  )
  {
    fulfilled(value);
    resolve();
  }

  void
  value_resolver
  (
    std::function<void()> resolve,
    std::function<void()> fulfilled
  )
  {
    fulfilled();
    resolve();
  }

  // A promise that resolves to T, and rejects with Reason
  template <typename T, typename Reason>
  class PromiseBase : public
    std::enable_shared_from_this<PromiseBase<T, Reason>>
  {
    public:

    using Types = PromiseTypes<T>;

    typedef typename Types::Resolved Resolved;
    typedef std::function<void(Reason)> Rejected;

    typedef std::function<void(Resolved, Rejected)> Handler;

    template <typename URea, typename U>
    std::shared_ptr<Promise<U, URea>>
    then_impl
    (
      typename ThenTypes<T, U>::Handler on_ful,
      std::function<U(URea)> on_rej
    )
    {
      auto self = this->shared_from_this();

      return Promise<U, URea>::create(
      [=] (
        typename PromiseTypes<U>::Resolved resolve,
        std::function<void(Reason)> reject
      ) {
        self->handle_fulfilled(
          [self, resolve, reject, on_ful, on_rej] (auto... result) {
            try {
              value_resolver(resolve, on_ful, result...);
            } catch (const URea& rea) {
              value_resolver(resolve, on_rej, rea);
            }
          }
        );
        self->handle_rejected(
          [self, resolve, on_rej, reject] (Reason reason) {
            try {
              value_resolver(resolve, on_rej, reason);
            } catch (const URea& rea) {
              reject(rea);
            }
          }
        );
      });
    }

    // std::function<U(T)>
    template <typename U>
    std::shared_ptr<Promise<U, Reason>>
    then_impl(typename ThenTypes<T, U>::Handler fulfilled)
    {
      auto self = this->shared_from_this();

      return Promise<U, Reason>::create(
      [=] (
        typename PromiseTypes<U>::Resolved resolve,
        std::function<void(Reason)> reject
      ) {
        self->handle_fulfilled(
          [self, resolve, fulfilled, reject] (auto... result) {
            try {
              value_resolver(resolve, fulfilled, result...);
            } catch (Reason& r) {
              reject(r);
            }
          }
        );
        self->handle_rejected(
          [self, reject] (const Reason& reason) {
            reject(reason);
          }
        );
      });
    }

    template <typename F>
    auto
    then(F fulfilled)
    {
      using ReturnType = typename FunctionReturn<F, T>::type;
      return this->template then_impl<ReturnType>(fulfilled);
    }

    template <typename F, typename R>
    auto
    then(F fulfilled, R rejected)
    {
      using ReturnType = typename FunctionReturn<F, T>::type;
      return this->template then_impl<Reason, ReturnType>(fulfilled, rejected);
    }

    protected:
    PromiseBase()
    : m_state(0)
    {
    }

    void
    handle_fulfilled(typename PromiseTypes<T>::Resolved handler)
    {
      switch (m_state)
      {
        case 0:
        m_handleFulfilled.push_back(handler);
        break;
        case 1:
        run_handler(handler, m_value);
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
    fulfill(Value<T> value = Value<T>())
    {
      if (m_state != 0)
      {
        return;
      }

      m_state = 1;
      m_value = value;

      m_handleRejected.clear();

      if (m_handleFulfilled.size() > 0)
      {
        auto ptr = this->shared_from_this();
        run_async([ handlers{std::move(m_handleFulfilled)}, ptr ]() {
          for (auto& handler : handlers)
          {
            run_handler(handler, ptr->m_value);
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

      m_handleFulfilled.clear();

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
    Value<T> m_value;
    Reason m_reason;
    std::vector<Resolved> m_handleFulfilled;
    std::vector<Rejected> m_handleRejected;
  };

  template <typename T, typename Reason>
  class PromiseValue : public PromiseBase<T, Reason>
  {
    public:

    static
    void
    resolver(std::shared_ptr<Promise<T, Reason>> promise, const T& value)
    {
      promise->fulfill(value);
    }

    template <typename U, typename UReason>
    static
    void
    resolver(
      std::shared_ptr<Promise<T, Reason>> promise,
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
      value->template then(std::function<void(U)>([promise] (auto result) {
        promise->fulfill(result);
      }));
    }
  };

  template <typename Reason>
  class PromiseValue<void, Reason> : public PromiseBase<void, Reason>
  {
    protected:

    static
    void
    resolver(std::shared_ptr<Promise<void, Reason>> promise)
    {
      promise->fulfill();
    }
  };

  template <typename T, typename Reason>
  class Promise : public PromiseValue<T, Reason>
  {
    public:

    using base = PromiseValue<T, Reason>;

    template <typename U>
    static
    std::shared_ptr<Promise<T, Reason>>
    create (U&& p)
    {
      using namespace std::placeholders;

      std::shared_ptr<Promise> ptr(new Promise);

      try
      {
        p([ptr](auto ...value) {
          PromiseValue<T, Reason>::resolver(ptr, value...);
        }, [ptr](auto reason) {
          ptr->reject(reason);
        });
      } catch (Reason& r)
      {
        ptr->reject(r);
      }

      return ptr;
    }
  };

}
