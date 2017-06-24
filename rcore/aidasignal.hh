// Licensed CC0 Public Domain: http://creativecommons.org/publicdomain/zero/1.0
#ifndef __RAPICORN_AIDA_SIGNAL_HH__
#define __RAPICORN_AIDA_SIGNAL_HH__

namespace Rapicorn { namespace Aida {

namespace Lib {

/// ProtoSignal is the template implementation for callback list.
template<typename,typename> class ProtoSignal;  // left undefined

/// AsyncSignal is the template implementation for callback lists with std::future returns.
template<typename> class AsyncSignal;           // left undefined

/// CollectorInvocation invokes handlers differently depending on return type.
template<typename,typename> struct CollectorInvocation;

/// PromiseInvocation invokes handlers differently depending on the promise value type.
template<typename,typename> struct PromiseInvocation;

/// CollectorLast returns the result of the last handler from a signal emission.
template<typename Result>
struct CollectorLast {
  typedef Result CollectorResult;
  explicit        CollectorLast ()              : last_() {}
  inline bool     operator()    (Result r)      { last_ = r; return true; }
  CollectorResult result        ()              { return last_; }
private:
  Result last_;
};

/// CollectorDefault implements the default handler collection behaviour.
template<typename Result>
struct CollectorDefault : CollectorLast<Result>
{};

/// CollectorDefault specialisation for handlers with void return type.
template<>
struct CollectorDefault<void> {
  typedef void CollectorResult;
  void                  result     ()           {}
  inline bool           operator() (void)       { return true; }
};

/// CollectorInvocation specialisation for regular handlers.
template<class Collector, class R, class... Args>
struct CollectorInvocation<Collector, R (Args...)> {
  static inline bool
  invoke (Collector &collector, const std::function<R (Args...)> &cbf, Args... args)
  {
    return collector (cbf (args...));
  }
};

/// CollectorInvocation specialisation for handlers with void return type.
template<class Collector, class... Args>
struct CollectorInvocation<Collector, void (Args...)> {
  static inline bool
  invoke (Collector &collector, const std::function<void (Args...)> &cbf, Args... args)
  {
    cbf (args...); return collector();
  }
};

/// HandlerLink implements a doubly-linked ring with ref-counted nodes containing callback links.
template<class Function>
struct HandlerLink {
  HandlerLink *next, *prev;
  Function     function;
  int          ref_count;
  explicit     HandlerLink (const Function &callback) : next (NULL), prev (NULL), function (callback), ref_count (1) {}
  /*dtor*/    ~HandlerLink ()           { AIDA_ASSERT_RETURN (ref_count == 0); }
  void         incref      ()           { ref_count += 1; AIDA_ASSERT_RETURN (ref_count > 0); }
  void         decref      ()           { ref_count -= 1; if (!ref_count) delete this; else AIDA_ASSERT_RETURN (ref_count > 0); }
  void
  unlink ()
  {
    function = NULL;
    if (next)
      next->prev = prev;
    if (prev)
      prev->next = next;
    next = prev = NULL;
    decref();
  }
  size_t
  add_before (const Function &callback)
  {
    HandlerLink *link = new HandlerLink (callback);
    link->prev = prev; // link to last
    link->next = this;
    prev->next = link; // link from last
    prev = link;
    static_assert (sizeof (link) == sizeof (size_t), "sizeof size_t");
    return size_t (link);
  }
  bool
  deactivate (const Function &callback)
  {
    if (callback == function)
      {
        function = NULL;      // deactivate static head
        return true;
      }
    for (HandlerLink *link = this->next ? this->next : this; link != this; link = link->next)
      if (callback == link->function)
        {
          link->unlink();     // deactivate and unlink sibling
          return true;
        }
    return false;
  }
  bool
  remove_sibling (size_t id)
  {
    for (HandlerLink *link = this->next ? this->next : this; link != this; link = link->next)
      if (id == size_t (link))
        {
          link->unlink();     // deactivate and unlink sibling
          return true;
        }
    return false;
  }
};

/// ProtoSignal template specialised for the callback signature and collector.
template<class Collector, class R, class... Args>
class ProtoSignal<R (Args...), Collector> : private CollectorInvocation<Collector, R (Args...)> {
protected:
  typedef std::function<R (Args...)>          CbFunction;
  typedef typename CbFunction::result_type    Result;
  typedef typename Collector::CollectorResult CollectorResult;
  typedef HandlerLink<CbFunction>             SignalLink;
private:
  SignalLink   *callback_ring_; // linked ring of callback nodes
  /*copy-ctor*/ ProtoSignal (const ProtoSignal&) = delete;
  ProtoSignal&  operator=   (const ProtoSignal&) = delete;
  void
  ensure_ring ()
  {
    if (!callback_ring_)
      {
        callback_ring_ = new SignalLink (CbFunction()); // ref_count = 1
        callback_ring_->incref(); // ref_count = 2, head of ring, can be deactivated but not removed
        callback_ring_->next = callback_ring_; // ring head initialization
        callback_ring_->prev = callback_ring_; // ring tail initialization
      }
  }
protected:
  /// ProtoSignal constructor, connects default callback if non-NULL.
  ProtoSignal (const CbFunction &method) :
    callback_ring_ (NULL)
  {
    if (method != NULL)
      {
        ensure_ring();
        callback_ring_->function = method;
      }
  }
  /// ProtoSignal destructor releases all resources associated with this signal.
  ~ProtoSignal ()
  {
    if (callback_ring_)
      {
        while (callback_ring_->next != callback_ring_)
          callback_ring_->next->unlink();
        AIDA_ASSERT_RETURN (callback_ring_->ref_count >= 2);
        callback_ring_->decref();
        callback_ring_->decref();
      }
  }
public:
  /// Operator to add a new function or lambda as signal handler, returns a handler connection ID.
  size_t connect    (const CbFunction &cb)      { ensure_ring(); return callback_ring_->add_before (cb); }
  /// Operator to remove a signal handler through its connection ID, returns if a handler was removed.
  bool   disconnect (size_t connection)         { return callback_ring_ ? callback_ring_->remove_sibling (connection) : false; }
  /// Emit a signal, i.e. invoke all its callbacks and collect return types with the Collector.
  CollectorResult
  emit (Args... args)
  {
    // check if an emission is needed
    Collector collector;
    if (!callback_ring_)
      return collector.result();
    // capture and incref signal handler list
    const bool have_link0 = callback_ring_->function != NULL;
    size_t capacity = 1 * have_link0;
    SignalLink *link;
    for (link = callback_ring_->next; link != callback_ring_; link = link->next)
      capacity++;       // capacity measuring is O(n), but we expect n to be small generally
    SignalLink *links[capacity];
    size_t nlinks = 0;
    if (have_link0)
      {
        callback_ring_->incref();
        links[nlinks++] = callback_ring_;
      }
    for (link = callback_ring_->next; link != callback_ring_; link = link->next)
      {
        link->incref();
        links[nlinks++] = link;
      }
    AIDA_ASSERT_RETURN (nlinks <= capacity, collector.result());
    // walk signal handler list, invoke and decref
    size_t i;
    for (i = 0; i < nlinks; i++)
      {
        SignalLink *link = links[i];
        if (link->function)
          {
            const bool continue_emission = this->invoke (collector, link->function, args...);
            if (!continue_emission)
              break;
          }
        link->decref();
      }
    for (; i < nlinks; i++)
      links[i]->decref();       // continue decref after 'break'
    // done
    return collector.result();
  }
};

/// PromiseInvocation specialisation for regular handlers.
template<class Promise, class R, class... Args>
struct PromiseInvocation<Promise, R (Args...)> {
  static inline void
  invoke (Promise &promise, const std::function<R (Args...)> &callback, Args&&... args)
  {
    promise.set_value (callback (std::forward<Args> (args)...));
  }
};

/// PromiseInvocation specialisation for handlers with void return type.
template<class Promise, class... Args>
struct PromiseInvocation<Promise, void (Args...)> {
  static inline void
  invoke (Promise &promise, const std::function<void (Args...)> &callback, Args&&... args)
  {
    callback (std::forward<Args> (args)...);
    promise.set_value();
  }
};

/// AsyncSignal template specialised for the callback signature.
template<class R, class... Args>
class AsyncSignal<R (Args...)> : private PromiseInvocation<std::promise<R>, R (Args...)> {
protected:
  typedef std::function<std::future<R> (Args...)> FutureFunction;
  typedef std::function<R (Args...)>              CbFunction;
  typedef HandlerLink<FutureFunction>             SignalLink;
private:
  SignalLink   *callback_ring_; // linked ring of callback nodes
  explicit      AsyncSignal (const AsyncSignal&) = delete;  // non-copyable
  AsyncSignal&  operator=   (const AsyncSignal&) = delete;  // non-assignable
  void
  ensure_ring ()
  {
    if (!callback_ring_)
      {
        callback_ring_ = new SignalLink (FutureFunction()); // ref_count = 1
        callback_ring_->incref(); // ref_count = 2, head of ring, can be deactivated but not removed
        callback_ring_->next = callback_ring_; // ring head initialization
        callback_ring_->prev = callback_ring_; // ring tail initialization
      }
  }
protected:
  /// AsyncSignal constructor, connects default callback if non-NULL.
  AsyncSignal (const FutureFunction &method) :
    callback_ring_ (NULL)
  {
    if (method != NULL)
      {
        ensure_ring();
        callback_ring_->function = method;
      }
  }
  /// AsyncSignal destructor releases all resources associated with this signal.
  ~AsyncSignal ()
  {
    if (callback_ring_)
      {
        while (callback_ring_->next != callback_ring_)
          callback_ring_->next->unlink();
        AIDA_ASSERT_RETURN (callback_ring_->ref_count >= 2);
        callback_ring_->decref();
        callback_ring_->decref();
      }
  }
public:
  class Emission {
    typedef std::function<std::future<R> (const FutureFunction&)> FunctionTrampoline;
    std::vector<SignalLink*> links_;
    size_t                   current_;
    std::future<R>           future_;
    FunctionTrampoline       trampoline_;
  public:
    Emission (SignalLink *start_link, Args&&... args) :
      current_ (0)
    {
      SignalLink *link = start_link;
      if (link)
        do
          {
            if (link->function != NULL)
              {
                link->incref();
                links_.push_back (link);
              }
            link = link->next;
          }
        while (link != start_link);
      // wrap args into lambda for deferred execution
      auto lambda =
        [link] (const FutureFunction &ff, Args... args) // -> std::future<R>
        {
          return std::move (ff (args...));
        };
      // FIXME: instead of std::bind, use lambda capture: [args...](){return ff(args...);}, but see gcc bug #41933
      trampoline_ = std::bind (lambda, std::placeholders::_1, std::forward<Args> (args)...);
    }
    ~Emission()
    {
      for (size_t i = 0; i < links_.size(); i++)
        links_[i]->decref();
    }
    bool has_value ()   { return future_.valid() && future_ready(); }
    R    get_value ()   { future_.wait(); return future_.get(); }
    bool done      ()   { return current_ >= links_.size() && !future_.valid(); }
    bool pending   ()   { return has_value() || (!future_.valid() && current_ < links_.size()); }
    bool dispatch  ()   { emit_stepwise(); return !done(); }
  private:
    bool
    future_ready ()
    {
      return future_.wait_for (std::chrono::nanoseconds (0)) == std::future_status::ready;
    }
    inline void
    emit_stepwise()
    {
      if (future_.valid())
        return;                                         // processing handler, use has_value
      if (current_ >= links_.size())
        return;                                         // done
      const FutureFunction &function = links_[current_]->function;
      current_++;
      if (AIDA_ISLIKELY (function != NULL))
        future_ = std::move (trampoline_ (function));   // valid() == true
    }
    RAPICORN_CLASS_NON_COPYABLE (Emission);
  };
  /// Operator to add a new function or lambda as signal handler, returns a handler connection ID.
  size_t connect_future (const FutureFunction &fcb) { ensure_ring(); return callback_ring_->add_before (fcb); }
  size_t connect        (const CbFunction &callback)
  {
    auto lambda =
      [this, callback] (Args&&... args) // -> std::future<R>
      {
        std::promise<R> promise;
        this->invoke (promise, callback, std::forward<Args> (args)...);
        return std::move (promise.get_future());
      };
    return connect_future (lambda);
  }
  /// Operator to remove a signal handler through its connection ID, returns if a handler was removed.
  bool   disconnect (size_t connection)         { return callback_ring_ ? callback_ring_->remove_sibling (connection) : false; }
  /// Create an asynchronous signal emission.
  Emission*
  emission (Args&&... args)
  {
    return new Emission (callback_ring_, std::forward<Args> (args)...);
  }
};

} // Lib
// namespace Rapicorn::Aida

/**
 * Signal is a template type providing an interface for arbitrary callback lists.
 * A signal type needs to be declared with the function signature of its callbacks,
 * and optionally a return result collector class type.
 * Signal callbacks can be added with operator+= to a signal and removed with operator-=, using
 * a callback connection ID return by operator+= as argument.
 * The callbacks of a signal are invoked with the emit() method and arguments according to the signature.
 * The result returned by emit() depends on the signal collector class. By default, the result of
 * the last callback is returned from emit(). Collectors can be implemented to accumulate callback
 * results or to halt a running emissions in correspondance to callback results.
 * The signal implementation is safe against recursion, so callbacks may be removed and
 * added during a signal emission and recursive emit() calls are also safe.
 * The overhead of an unused signal is intentionally kept very low, around the size of a single pointer.
 * Note that the Signal template types is non-copyable.
 */
template <typename SignalSignature, class Collector = Lib::CollectorDefault<typename std::function<SignalSignature>::result_type> >
class Signal : protected Lib::ProtoSignal<SignalSignature, Collector>
{
  typedef Lib::ProtoSignal<SignalSignature, Collector> ProtoSignal;
  typedef typename ProtoSignal::CbFunction             CbFunction;
public:
  explicit Signal    (const Signal&) = delete;  // non-copyable
  Signal&  operator= (const Signal&) = delete;  // non-assignable
  using ProtoSignal::emit;
  class Connector {
    friend     class Signal;
    Signal    &signal_;
    Connector& operator= (const Connector&) = delete;
    explicit   Connector (Signal &signal) : signal_ (signal) {}
  public:
    /// Operator to add a new function or lambda as signal handler, returns a handler connection ID.
    size_t operator+= (const CbFunction &cb)              { return signal_.connect (cb); }
    /// Operator to remove a signal handler through its connection ID, returns if a handler was removed.
    bool   operator-= (size_t connection_id)              { return signal_.disconnect (connection_id); }
  };
  /// Signal constructor, supports a default callback as argument.
  Signal (const CbFunction &method = CbFunction()) : ProtoSignal (method) {}
  /// Retrieve a connector object with operator+= and operator-= to connect and disconnect signal handlers.
  Connector operator() ()                                 { return Connector (*this); }
};

/// This function creates a std::function by binding @a object to the member function pointer @a method.
template<class Instance, class Class, class R, class... Args> std::function<R (Args...)>
slot (Instance &object, R (Class::*method) (Args...))
{
  return [&object, method] (Args... args) { return (object .* method) (args...); };
}

/// This function creates a std::function by binding @a object to the member function pointer @a method.
template<class Class, class R, class... Args> std::function<R (Args...)>
slot (Class *object, R (Class::*method) (Args...))
{
  return [object, method] (Args... args) { return (object ->* method) (args...); };
}

/// Keep signal emissions going while all handlers return !0 (true).
template<typename Result>
struct CollectorUntil0 {
  typedef Result CollectorResult;
  explicit                      CollectorUntil0 ()      : result_() {}
  const CollectorResult&        result          ()      { return result_; }
  inline bool
  operator() (Result r)
  {
    result_ = r;
    return result_ ? true : false;
  }
private:
  CollectorResult result_;
};

/// Keep signal emissions going while all handlers return 0 (false).
template<typename Result>
struct CollectorWhile0 {
  typedef Result CollectorResult;
  explicit                      CollectorWhile0 ()      : result_() {}
  const CollectorResult&        result          ()      { return result_; }
  inline bool
  operator() (Result r)
  {
    result_ = r;
    return result_ ? false : true;
  }
private:
  CollectorResult result_;
};

/// CollectorVector returns the result of the all signal handlers from a signal emission in a std::vector.
template<typename Result>
struct CollectorVector {
  typedef std::vector<Result> CollectorResult;
  const CollectorResult&        result ()       { return result_; }
  inline bool
  operator() (Result r)
  {
    result_.push_back (r);
    return true;
  }
private:
  CollectorResult result_;
};

/// Connector provides a simple (dis-)connect interfaces for signals on RemoteHandle
template<class Object, class SignalSignature>
class Connector {
  typedef std::function<SignalSignature> CbFunction;
  typedef size_t (Object::*PMF) (size_t, const CbFunction&);
  Object &instance_;
  PMF     method_;
public:
  Connector (Object &instance, PMF method) : instance_ (instance), method_ (method) {}
  /// Operator to add a new function or lambda as signal handler, returns a handler connection ID.
  size_t operator+= (const CbFunction &cb)              { return (instance_.*method_) (0, cb); }
  /// Operator to remove a signal handler through its connection ID, returns if a handler was removed.
  bool   operator-= (size_t connection_id)              { return connection_id ? (instance_.*method_) (connection_id, *(CbFunction*) NULL) : false; }
};

/**
 * AsyncSignal is a Signal type with support for std::future returns from handlers.
 * The API is genrally analogous to Signal, however handlers that return a std::future<ReturnValue>
 * are supported via the connect_future() method, and emissions are handled differently.
 * Note that even simple handlers are wrapped into std::promise/std::future constructs, which
 * makes AsyncSignal emissions significantly slower than Signal.emit().
 * The emisison() method creates an Emission object, subject to a delete() call later on.
 * The general idiom for using emission objects is as follows:
 * @code
 * MySignal::Emission *emi = sig_my_signal.emission (args...);
 * while (!emi->done()) {
 *   if (emi->pending())
 *     emi->dispatch();         // this calls signal handlers
 *   if (emi->has_value())
 *     use (emi->get_value());  // value return from a signal handler via resolved future
 *   else
 *     ThisThread::yield();     // allow for (asynchronous) signal handler execution
 * }
 * delete emi;
 * @endcode
 * Multiple emission objects may be created and used at the same time, and deletion of an
 * emission before done() returns true is also supported.
 */
template <typename SignalSignature>
class AsyncSignal : protected Lib::AsyncSignal<SignalSignature>
{
  typedef Lib::AsyncSignal<SignalSignature>   BaseSignal;
  typedef typename BaseSignal::CbFunction     CbFunction;
  typedef typename BaseSignal::FutureFunction FutureFunction;
  class Connector {
    AsyncSignal &signal_;
    friend class AsyncSignal;
    Connector& operator= (const Connector&) = delete;
    explicit Connector (AsyncSignal &signal) : signal_ (signal) {}
  public:
    /// Method to add a new std::future<ReturnValue> function or lambda as signal handler, returns a handler connection ID.
    size_t connect_future (const FutureFunction &ff)    { return signal_.connect_future (ff); }
    /// Operator to add a new function or lambda as signal handler, returns a handler connection ID.
    size_t operator+= (const CbFunction &cb)            { return signal_.connect (cb); }
    /// Operator to remove a signal handler through its connection ID, returns if a handler was removed.
    bool   operator-= (size_t connection_id)            { return signal_.disconnect (connection_id); }
  };
public:
  using typename BaseSignal::Emission;
  using BaseSignal::emission;
  /// Signal constructor, supports a default callback as argument.
  AsyncSignal (const FutureFunction &method = FutureFunction()) : BaseSignal (method) {}
  /// Retrieve a connector object with operator+= and operator-= to connect and disconnect signal handlers.
  Connector operator() ()                               { return Connector (*this); }
};

} } // Rapicorn::Aida

#endif // __RAPICORN_AIDA_SIGNAL_HH__
