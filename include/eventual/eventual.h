#pragma once

// The MIT License(MIT)
// 
// Copyright(c) 2016 Ryan Ferraro
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <memory>
#include <exception>
#include <type_traits>
#include <future>
#include <vector>
#include <stack>
#include <tuple>

namespace eventual
{
   template <class R> class Future;
   template <class R> class Shared_Future;
   template <class> class Packaged_Task; // undefined
   template <class R, class... ArgTypes>
   class Packaged_Task<R(ArgTypes...)>;

   template<class Sequence>
   struct When_Any_Result;

   using future_errc = std::future_errc;
   using future_error = std::future_error;
   using future_status = std::future_status;
   
   namespace detail
   {
      // Traits

      template<class R>
      struct is_future : std::false_type { };

      template<class R>
      struct is_future<Future<R>> : std::true_type { };

      template<class R>
      struct is_future<Shared_Future<R>> : std::true_type { };

      template<class T>
      using enable_if_future_t = std::enable_if_t<is_future<T>::value, T>;

      template<class T, class TResult>
      using enable_if_not_future_t = std::enable_if_t<!is_future<T>::value, TResult>;

      template<class R>
      struct decay_future_result
      {
         typedef std::decay_t<R> type;
      };

      template<class R>
      struct decay_future_result<std::reference_wrapper<R>>
      {
         typedef R& type;
      };

      template<class R>
      using decay_future_result_t = typename decay_future_result<R>::type;

      template<class...>
      struct has_parameter
      {
         typedef void type;
      };

      template<class, class = void>
      struct is_iterator : std::false_type { };

      template<class T>
      struct is_iterator<T, typename has_parameter<
         typename T::iterator_category,
         typename T::value_type,
         typename T::difference_type,
         typename T::pointer,
         typename T::reference>::type>
         : std::true_type
      { };

      template<class T>
      using enable_if_iterator_t = std::enable_if_t<is_iterator<T>::value, T>;

      template<class T, class U>
      using enable_if_not_same = std::enable_if<!std::is_same<std::decay_t<T>, std::decay_t<U>>::value>;

      // Aliases

      template<class InputIterator>
      using all_futures_vector = Future<std::vector<typename std::iterator_traits<InputIterator>::value_type>>;

      template<class... Futures>
      using all_futures_tuple = Future<std::tuple<std::decay_t<Futures>...>>;

      template<class InputIterator>
      using any_future_result_vector = Future<When_Any_Result<std::vector<typename std::iterator_traits<InputIterator>::value_type>>>;

      template<class... Futures>
      using any_futures_result_tuple = Future<When_Any_Result<std::tuple<std::decay_t<Futures>...>>>;

      // Helper Methods

      template<class T>
      decltype(auto) decay_copy(T&& v)
      {
         return std::forward<T>(v);
      }

      template<std::size_t I = 0, typename TTuple, typename Func>
      std::enable_if_t<I != std::tuple_size<TTuple>::value>
         for_each(TTuple& tuple, Func&& func)
      {
         func(std::get<I>(tuple));
         for_each<I + 1>(tuple, std::forward<Func>(func));
      }

      template<std::size_t I = 0, typename TTuple, typename Func>
      std::enable_if_t<I == std::tuple_size<TTuple>::value>
         for_each(TTuple& tuple, Func&& func)
      {
         //do nothing
      }

      inline void ThrowFutureError(future_errc error);
      inline std::exception_ptr CreateFutureExceptionPtr(future_errc error);

      // Detail classes

      template<class F>
      class SharedFunction
      {
      public:
         SharedFunction() = default;
         SharedFunction(F&& function) 
            : _function(std::make_shared<F>(std::forward<F>(function))) { }

         template<class Allocator>
         SharedFunction(const Allocator& alloc, F&& function)
            : _function(std::allocate_shared<F>(alloc, std::forward<F>(function))) { }

         SharedFunction(const SharedFunction&) = default;
         SharedFunction(SharedFunction&&) = default;
         SharedFunction& operator=(const SharedFunction&) = default;
         SharedFunction& operator=(SharedFunction&&) = default;

         template<class... Args>
         decltype(auto) operator()(Args&&... args) const
         {
            return (*_function)(std::forward<Args>(args)...);
         }

      private:
         std::shared_ptr<F> _function;
      };

      template<class F>
      SharedFunction<std::decay_t<F>> MakeSharedFunction(F&& function)
      {
         return SharedFunction<std::decay_t<F>>(std::move(function));
      }

      template<class Allocator, class F>
      SharedFunction<std::decay_t<F>> AllocateSharedFunction(const Allocator& alloc, F&& function)
      {
         return SharedFunction<std::decay_t<F>>(alloc, std::move(function));
      }

      class BasicState
      {
      protected:
         using unique_lock = std::unique_lock<std::mutex>;

      public:
         BasicState() :
            _retrieved(false),
            _block(std::make_shared<NotifierBlock>()),
            _hasResult(false),
            _exception(nullptr)
         { }

         BasicState(BasicState&&) = delete;
         BasicState(const BasicState&) = delete;
         BasicState& operator=(BasicState&&) = delete;
         BasicState& operator=(const BasicState&) = delete;

         bool HasException() { return _exception ? true : false; }
         std::exception_ptr GetException() { return _exception; }
         bool Is_Ready() { return _block->Is_Ready(); }
         bool HasResult() { return _hasResult; }

         // future retrieved
         bool SetRetrieved()
         {
            auto lock = AquireLock();
            if (_retrieved)
               return false;

            _retrieved = true;
            return _retrieved;
         }

         template<class T>
         void SetCallback(T&& callback)
         {
            auto lock = AquireLock();
            _block->_callbacks.emplace_back(MakeSharedFunction(std::forward<T>(callback)));

            auto cb = _block->_callbacks.back();
            if (Is_Ready())
            {
               lock.unlock();
               cb();
            }
         }

         bool SetException(std::exception_ptr ex)
         {
            if (!SetExceptionImpl(ex))
               return false;

            NotifyPromiseFullfilled();
            return true;
         }

         bool SetExceptionAtThreadExit(std::exception_ptr ex)
         {
            if (!SetExceptionImpl(ex))
               return false;

            NotifyPromiseFullfilledAtThreadExit();
            return true;
         }

         void Wait()
         {
            auto lock = AquireLock();
            Wait(lock);
         }

         template <class TDuration>
         bool Wait_For(const TDuration& rel_time)
         {
            auto lock = AquireLock();
            return _block->_condition.wait_for(lock, rel_time, [block = _block]() { return block->Is_Ready(); });
         }

         template <class TTime>
         bool Wait_Until(const TTime& abs_time)
         {
            auto lock = AquireLock();
            return _block->_condition.wait_until(lock, abs_time, [block = _block]() { return block->Is_Ready(); });
         }

      protected:

         void NotifyPromiseFullfilled()
         {
            _block->InvokeContinuations();
         }

         void NotifyPromiseFullfilledAtThreadExit()
         {
            auto block = _block;

            using namespace std;
            class ExitNotifier
            {
            private:
               using callback_t = std::function<void()>;

               stack<callback_t> _exitFunctions;
            public:
               ExitNotifier() = default;
               ExitNotifier(const ExitNotifier&) = delete;
               ExitNotifier& operator=(const ExitNotifier&) = delete;

               ~ExitNotifier() noexcept
               {
                  while (!_exitFunctions.empty())
                  {
                     _exitFunctions.top()();
                     _exitFunctions.pop();
                  }
               }

               void Add(callback_t&& callback)
               {
                  _exitFunctions.push(forward<callback_t>(callback));
               }
            };

            thread_local ExitNotifier notifier;
            notifier.Add([block]() { block->InvokeContinuations(); });
         }

         // lock before calling
         void SetResult(const BasicState& other)
         {
            if(!SetHasResult())
               ThrowFutureError(future_errc::promise_already_satisfied);

            _hasResult = other._hasResult;
            _exception = other._exception;
         }

         void Wait(unique_lock& lock)
         {
            _block->_condition.wait(lock, [block = _block]() { return block->Is_Ready(); });
         }

         unique_lock AquireLock()
         {
            return _block->AquireLock();
         }

         bool SetHasResult()
         {
            if (_hasResult)
               return false;

            _hasResult = true;
            return _hasResult;
         }

         void CheckException()
         {
            if(_exception)
               std::rethrow_exception(_exception);
         }

      private:

         struct NotifierBlock
         {
            NotifierBlock() 
               : _ready(0), _mutex(), _condition(), _callbacks() { }

            unique_lock AquireLock() { return unique_lock(_mutex); }

            void InvokeContinuations()
            {
               {
                  auto lock = AquireLock();
                  _ready = true;
                  _condition.notify_all();
               }
               
               for (auto& cb : _callbacks)
                  cb(); // todo: exceptions?
            }

            bool Is_Ready() { return _ready != 0; }
            
            int _ready;
            std::mutex _mutex;
            std::condition_variable _condition;
            std::vector<std::function<void()>> _callbacks;
         };

         bool SetExceptionImpl(std::exception_ptr ex)
         {
            auto lock = AquireLock();
            if (!SetHasResult())
               return false;

            _exception = ex;
            return true;
         }

         bool _retrieved;
         bool _hasResult;
         std::exception_ptr _exception;
         std::shared_ptr<NotifierBlock> _block;
      };

      template<typename R>
      class State : public BasicState
      {
      public:

         void SetResult(State& other)
         {
            {
               auto lock = AquireLock();
               _result = std::move(other._result);
               BasicState::SetResult(other);
            }
            NotifyPromiseFullfilled();
         }

         R GetResult()
         {
            auto lock = AquireLock();

            if (Is_Ready())
            {
               CheckException();
               return std::move(_result);
            }

            Wait(lock);
            CheckException();
            return std::move(_result);
         }

         bool SetResult(const R& value)
         {
            if(!SetResultImpl(value))
               return false;
            
            NotifyPromiseFullfilled();
            return true;
         }

         bool SetResult(R&& value)
         {
            if (!SetResultImpl(std::forward<R>(value)))
               return false;

            NotifyPromiseFullfilled();
            return true;
         }

         bool SetResultAtThreadExit(const R& value)
         {
            if (!SetResultImpl(value))
               return false;

            NotifyPromiseFullfilledAtThreadExit();
            return true;
         }

         bool SetResultAtThreadExit(R&& value)
         {
            if (!SetResultImpl(std::forward<R>(value)))
               return false;

            NotifyPromiseFullfilledAtThreadExit();
            return true;
         }

      private:

         bool SetResultImpl(const R& value)
         {
            auto lock = AquireLock();
            if (!SetHasResult())
               return false;

            _result = value;
            return true;
         }
         
         bool SetResultImpl(R&& value)
         {
            auto lock = AquireLock();
            if (!SetHasResult())
               return false;

            _result = std::forward<R>(value);
            return true;
         }

         R _result;
      };

      template<typename R>
      class State<R&> : public BasicState
      {
      public:
         State() : _result(nullptr) { }

         void SetResult(State& other)
         {
            {
               auto lock = AquireLock();

               _result = other._result;
               BasicState::SetResult(other);
            }
            NotifyPromiseFullfilled();
         }

         R& GetResult() 
         { 
            auto lock = AquireLock();

            if (Is_Ready())
            {
               CheckException();
               return *_result;
            }

            Wait(lock);
            CheckException();
            return *_result; 
         }

         bool SetResult(R& value)
         {
            if (!SetResultImpl(value))
               return false;

            NotifyPromiseFullfilled();
            return true;
         }

         bool SetResultAtThreadExit(R& value)
         {
            if (!SetResultImpl(value))
               return false;

            NotifyPromiseFullfilledAtThreadExit();
            return true;
         }

      private:

         bool SetResultImpl(R& value)
         {
            auto lock = AquireLock();
            if (!SetHasResult())
               return false;

            _result = &value;
            return true;
         }

         R* _result;
      };

      template<>
      class State<void> : public BasicState
      {
      public:

         void SetResult(State& other)
         {
            {
               auto lock = AquireLock();
               BasicState::SetResult(other);
            }
            NotifyPromiseFullfilled();
         }

         void GetResult()
         {
            auto lock = AquireLock();

            if (Is_Ready())
               CheckException();

            Wait(lock);
            CheckException();
         }

         bool SetResult()
         {
            if (!SetResultImpl())
               return false;

            NotifyPromiseFullfilled();
            return true;
         }

         bool SetResultAtThreadExit()
         {
            if (!SetResultImpl())
               return false;

            NotifyPromiseFullfilledAtThreadExit();
            return true;
         }

      private:

         bool SetResultImpl()
         {
            auto lock = AquireLock();
            if (!SetHasResult())
               return false;

            return true;
         }
      };

      class FutureHelper
      {
      private:
         template<class>
         struct result_of_future { };

         template<class R>
         struct result_of_future<Shared_Future<R>>
         {
            typedef R result_type;
         };

         template<class R>
         struct result_of_future<Future<R>>
         {
            typedef R result_type;
         };

         template<class TFuture>
         using future_state = std::shared_ptr<State<typename result_of_future<TFuture>::result_type>>;

      public:

         template<class T>
         static T CreateFuture(const future_state<T>& state);

         template<class InputIterator>
         static decltype(auto) When_Any(enable_if_iterator_t<InputIterator> begin, InputIterator end)
         {
            using namespace std;
            using future_t = typename iterator_traits<InputIterator>::value_type;
            using result_t = When_Any_Result<vector<future_t>>;

            const auto noIdx = static_cast<size_t>(-1);
            auto indexPromise = make_shared<Promise<size_t>>();
            auto indexFuture = indexPromise->Get_Future();
            vector<future_t> futures;

            size_t index = 0;
            for (auto iterator = begin; iterator != end; ++iterator)
            {
               futures.emplace_back(move(*iterator));

               auto state = futures.back()._state;
               state->SetCallback([indexPromise, index]()
               {
                  indexPromise->Try_Set_Value_(index);
               });

               index++;
            }

            if (futures.empty())
               indexPromise->SetValue(noIdx);

            return indexFuture.ThenForward(
               [futures = std::move(futures)](auto& f) mutable
            {
               result_t result;
               result.index = f.Get();
               result.futures = move(futures);

               return result;
            });
         }

         inline static Future<When_Any_Result<std::tuple<>>> When_Any();

         template<class... Futures>
         static decltype(auto) When_Any(Futures&&... futures)
         {
            using namespace std;
            using result_t = When_Any_Result<tuple<decay_t<Futures>...>>;
            auto futuresSequence = make_tuple<std::decay_t<Futures>...>(std::forward<Futures>(futures)...);
            auto indexPromise = make_shared<Promise<size_t>>();
            auto indexFuture = indexPromise->Get_Future();

            size_t index = 0;
            detail::for_each(futuresSequence, [indexPromise, &index](auto& future) mutable
            {
               auto state = future._state;
               state->SetCallback([indexPromise, index]()
               {
                  indexPromise->Try_Set_Value_(index);
               });

               index++;
            });

            return indexFuture.ThenForward(
               [t = move(futuresSequence)](auto& f) mutable
            {
               result_t result;
               result.index = f.Get();
               result.futures = move(t);

               return move(result);
            });
         }

         template<class InputIterator>
         static decltype(auto) When_All(enable_if_iterator_t<InputIterator> first, InputIterator last)
         {
            using namespace std;
            using future_vector_t = vector<typename iterator_traits<InputIterator>::value_type>;

            auto vectorFuture = Make_Ready_Future(future_vector_t());

            for (auto iterator = first; iterator != last; ++iterator)
            {
               auto& future = *iterator;
               vectorFuture = vectorFuture.ThenForward(
                  [future = move(future)](auto& vf) mutable
               {
                  return future.ThenForward(
                     [vf = move(vf)](auto& future) mutable
                  {
                     auto resultVector = vf.Get();
                     resultVector.emplace_back(move(future));
                     return resultVector;
                  });
               });
            }

            return vectorFuture;
         }

         static inline decltype(auto) When_All();

         template<class TFuture, class... TFutures>
         static decltype(auto) When_All(TFuture&& head, TFutures&&... others)
         {
            using namespace std;

            auto tailFuture = When_All(forward<TFutures>(others)...);
            return tailFuture.ThenForward([head = move(head)](auto& tf) mutable
            {
               return head.ThenForward([tf = move(tf)](auto& h) mutable
               {
                  return tuple_cat(make_tuple(move(h)), tf.Get());
               });
            });
         }
      };

      template<class R>
      class BasicPromise
      {
         using SharedState = std::shared_ptr<State<R>>;
         using unique_lock = std::unique_lock<std::mutex>;
         friend class FutureHelper;
      public:
         BasicPromise() : _state(std::make_shared<State<R>>()) { }
         BasicPromise(const BasicPromise& other) = delete;
         BasicPromise(BasicPromise&& other) noexcept = default;

         BasicPromise(SharedState state) : _state(state) { }

         template<class Alloc>
         BasicPromise(std::allocator_arg_t, const Alloc& alloc) :
            _state(std::allocate_shared<State<R>, Alloc>(alloc))
         { }

         virtual ~BasicPromise() noexcept
         {
            if (!Valid() || _state->HasResult())
               return;

            _state->SetException(CreateFutureExceptionPtr(future_errc::broken_promise));
         }

         BasicPromise& operator=(const BasicPromise& rhs) = delete;
         BasicPromise& operator=(BasicPromise&& other) noexcept = default;

         void Swap(BasicPromise& other) noexcept
         {
            _state.swap(other._state);
         }

         Future<R> Get_Future()
         {
            auto state = ValidateState();

            if(!state->SetRetrieved())
               ThrowFutureError(future_errc::future_already_retrieved);

            return CreateFuture(state);
         }

      protected:

         template<class... T>
         void SetValue(T&&... value)
         {
            auto state = ValidateState();

            if(!state->SetResult(std::forward<T>(value)...))
               ThrowFutureError(future_errc::promise_already_satisfied);
         }

         template<class... T>
         void SetValueAtThreadExit(T&&... value)
         {
            auto state = ValidateState();

            if(!state->SetResultAtThreadExit(std::forward<T>(value)...))
               ThrowFutureError(future_errc::promise_already_satisfied);
         }

         void SetException(std::exception_ptr exceptionPtr)
         {
            auto state = ValidateState();

            if(!state->SetException(exceptionPtr))
               ThrowFutureError(future_errc::promise_already_satisfied);
         }

         void SetExceptionAtThreadExit(std::exception_ptr exceptionPtr)
         {
            auto state = ValidateState();

            if(!state->SetExceptionAtThreadExit(exceptionPtr))
               ThrowFutureError(future_errc::promise_already_satisfied);
         }

         template<class... T>
         bool TrySetValue(T&&... value) noexcept
         {
            auto state = _state;
            if(!state)
               return false;
            
            return state->SetResult(std::forward<T>(value)...);
         }

         bool Try_Set_Exception(std::exception_ptr exceptionPtr) noexcept
         {
            auto state = _state;
            if (!state)
               return false;

            return state->SetException(exceptionPtr);
         }

         void Reset()
         {
            // todo: save allocator somewhere?
            _state = std::make_shared<State<R>>();
         }

         bool Valid() const noexcept
         {
            auto state = _state;
            return state ? true : false;
         }

         SharedState ValidateState()
         {
            auto state = _state;
            if(!state)
               ThrowFutureError(future_errc::no_state);

            return state;
         }

         void VerifyPromiseNotSatisfied()
         {
            if (_state->HasResult())
               ThrowFutureError(future_errc::promise_already_satisfied);
         }

         void NotifyPromiseFullfilled()
         {
            _state->NotifyPromiseFullfilled();
         }

      private:

         static Future<R> CreateFuture(SharedState state) noexcept
         {
            return FutureHelper::CreateFuture<Future<R>>(std::forward<SharedState>(state));
         }

         bool TrySetExceptionInternal(std::exception_ptr ex)
         {
            struct Anonymous { };

            if (_state->HasResult())
               return false;
            _state->SetException(ex ? ex : std::make_exception_ptr(Anonymous()));

            return true;
         }

         template<class T>
         bool TrySetValueInternal(T&& value) noexcept
         {
            if (_state->HasResult())
               return false;
            _state->SetResult(std::forward<T>(value));

            return true;
         }

         bool TrySetValueInternal() noexcept
         {
            if (_state->HasResult())
               return false;
            _state->SetResult();

            return true;
         }

         SharedState _state;
      };

      template<class R, class... ArgTypes>
      class BasicTask : public BasicPromise<R>
      {
         using Base = detail::BasicPromise<R>;

      public:
         BasicTask() noexcept : Base(nullptr) { }
         BasicTask(const BasicTask&) = delete;
         BasicTask(BasicTask&& other) noexcept
            : Base(std::forward<BasicTask>(other)), _task(std::move(other._task))
         { }

         template<class F, class Allocator, class = detail::enable_if_not_same<BasicTask, F>>
         BasicTask(std::allocator_arg_t arg, const Allocator& alloc, F&& task)
            : Base(arg, alloc), _task(arg, alloc, AllocateSharedFunction(alloc, std::forward<F>(task)))
         { }

         template<class F, class = detail::enable_if_not_same<BasicTask, F>>
         BasicTask(F&& task)
            : Base(), _task(MakeSharedFunction(std::forward<F>(task)))
         { }

         void Swap(BasicTask& other) noexcept
         {
            Base::Swap(other);
            std::swap(_task, other._task);
         }

         void Reset()
         {
            ValidateState();

            Base::Reset();
         }

         bool Valid() { return Base::Valid(); }

         void operator()(ArgTypes... args)
         {
            try
            {
               ExecuteTask<R>(std::forward<ArgTypes>(args)...);
            }
            catch (const future_error& error)
            {
               //todo: verify this behavior...
               if (error.code() != future_errc::promise_already_satisfied)
                  SetException(std::current_exception());
               else
                  throw;
            }
            catch (...)
            {
               SetException(std::current_exception());
            }
         }

         void Make_Ready_At_Thread_Exit(ArgTypes... args)
         {
            try
            {
               ExecuteTaskDeferred<R>(std::forward<ArgTypes>(args)...);
            }
            catch (const future_error& error)
            {
               //todo: verify this behavior...
               if (error.code() != future_errc::promise_already_satisfied)
                  SetExceptionAtThreadExit(std::current_exception());
               else
                  throw;
            }
            catch (...)
            {
               SetExceptionAtThreadExit(std::current_exception());
            }
         }

      private:

         template<typename T>
         std::enable_if_t<!std::is_void<T>::value>
            ExecuteTask(ArgTypes... args)
         {
            SetValue(_task(std::forward<ArgTypes>(args)...));
         }

         template<typename T>
         std::enable_if_t<std::is_void<T>::value>
            ExecuteTask(ArgTypes... args)
         {
            _task(std::forward<ArgTypes>(args)...);
            SetValue();
         }

         template<typename T>
         std::enable_if_t<!std::is_void<T>::value>
            ExecuteTaskDeferred(ArgTypes... args)
         {
            SetValueAtThreadExit(_task(std::forward<ArgTypes>(args)...));
         }

         template<typename T>
         std::enable_if_t<std::is_void<T>::value>
            ExecuteTaskDeferred(ArgTypes... args)
         {
            _task(std::forward<ArgTypes>(args)...);
            SetValueAtThreadExit();
         }

         std::function<R(ArgTypes...)> _task;
      };

      template<class R, class TFuture>
      class BasicFuture
      {
         using State = detail::State<R>;
         using SharedState = std::shared_ptr<State>;
         using unique_lock = std::unique_lock<std::mutex>;

         template<class, class>
         friend class BasicFuture;
         friend class FutureHelper;

      public:
         BasicFuture() noexcept = default;
         BasicFuture(const BasicFuture& other) = default;
         BasicFuture(BasicFuture&& other) noexcept = default;

         explicit BasicFuture(const SharedState& state) : _state(state) { }

         BasicFuture& operator=(const BasicFuture& rhs) = default;
         BasicFuture& operator=(BasicFuture&& other) noexcept = default;

         bool Valid() const noexcept { return _state ? true : false; }
         bool Is_Ready() const noexcept { return (_state && _state->Is_Ready()); }

         Shared_Future<R> Share()
         {
            auto state = ValidateState();
            _state.reset();

            return FutureHelper::CreateFuture<Shared_Future<R>>(state);
         }

         void Wait() const
         {
            auto state = ValidateState();
            state->Wait();
         }

         template <class Rep, class Period>
         future_status Wait_For(const std::chrono::duration<Rep, Period>& rel_time) const
         {
            auto state = ValidateState();
            return state->Wait_For(rel_time) ? future_status::ready : future_status::timeout;
         }

         template <class Clock, class Duration>
         future_status Wait_Until(const std::chrono::time_point<Clock, Duration>& abs_time) const
         {
            auto state = ValidateState();
            return state->Wait_Until(abs_time) ? future_status::ready : future_status::timeout;
         }

         decltype(auto) Get()
         {
            return GetResult(*this);
         }

         template<class F>
         decltype(auto) Then(F&& continuation)
         {
            return ThenImpl(detail::decay_copy(continuation));
         }

      protected:

         template<template<typename> class Future>
         BasicFuture(BasicFuture<R, Future<R>>&& other)
            : BasicFuture(other._state)
         { }

         template<class Future, template<typename> class NestedFuture>
         BasicFuture(BasicFuture<NestedFuture<R>, Future>&& other)
            : BasicFuture(SharedState(new State()))
         {
            auto outerState = std::move(other._state);

            // todo: Yo dawg; I heard you like continuations...
            outerState->SetCallback([outerState, futureState = _state]()
            {
               if (!outerState->HasException())
               {
                  auto innerState = outerState->GetResult()._state;
                  if (innerState)
                  {
                     innerState->SetCallback([futureState, innerState]()
                     {
                        futureState->SetResult(*innerState); // todo: may throw
                     });
                  }
                  else
                     futureState->SetException(std::make_exception_ptr(std::exception("no state"))); // todo: better error.
               }
               else
                  futureState->SetException(outerState->GetException());
            });
         }

         SharedState ValidateState() const
         {
            auto state = _state;
            if(!state)
               ThrowFutureError(future_errc::no_state);

            return state;
         }

         template<class F>
         decltype(auto) ThenForward(F&& continuation)
         {
            return ThenImpl(std::forward<F>(continuation));
         }

      private:

         template<class F>
         decltype(auto) ThenImpl(F&& continuation)
         {
            using namespace std;
            using result_t = result_of_t<F(TFuture)>;

            auto state = ValidateState();

            BasicTask<result_t, TFuture> task(forward<F>(continuation));

            auto taskFuture = Unwrap(task.Get_Future());
            auto future = CreateInternalFuture(state);

            state->SetCallback(
               [
                  task = move(task),
                  future = move(future)
               ]() mutable
            {
               task(forward<TFuture>(future));
            });

            return taskFuture;
         }

         static void TransferState(BasicFuture<R, Shared_Future<R>>& future, SharedState& state)
         {
            future._state = state;
         }

         static void TransferState(BasicFuture<R, Future<R>>& future, SharedState& state)
         {
            future._state = std::move(state);
         }

         TFuture CreateInternalFuture(SharedState state)
         {
            TFuture future;
            TransferState(future, state);

            // state was "moved"
            if (!state)
               _state.reset();

            return future;
         }

         template<typename T, template<typename> class TOuter, template<typename> class TInner>
         static enable_if_future_t<TInner<T>> Unwrap(TOuter<TInner<T>>&& future)
         {
            return TInner<T>(std::forward<TOuter<TInner<T>>>(future));
         }

         template<typename T, template<typename> class TFuture>
         static enable_if_not_future_t<T, TFuture<T>> Unwrap(TFuture<T>&& future)
         {
            return std::forward<TFuture<T>>(future);
         }

         template<class T>
         static decltype(auto) GetResult(BasicFuture<T, Future<T>>& future)
         {
            auto state = future.ValidateState();
            future._state.reset();
            return state->GetResult();
         }

         template<class T>
         static decltype(auto) GetResult(BasicFuture<T, Shared_Future<T>>& future)
         {
            auto state = future.ValidateState();
            return state->GetResult();
         }

         static void GetResult(BasicFuture<void, Future<void>>& future)
         {
            auto state = future.ValidateState();
            future._state.reset();
            state->GetResult();
         }

         static void GetResult(BasicFuture<void, Shared_Future<void>>& future)
         {
            auto state = future.ValidateState();
            state->GetResult();
         }

         template<class T>
         static void CheckException(BasicFuture<T, Future<T>>& future)
         {
            if (future._state->HasException())
            {
               auto state = std::move(future._state);
               std::rethrow_exception(state->GetException());
            }
         }

         template<class T>
         static void CheckException(BasicFuture<T, Shared_Future<T>>& future)
         {
            if (future._state->HasException())
               std::rethrow_exception(future._state->GetException());
         }

         SharedState _state;
      };
   }

   template<class Sequence>
   struct When_Any_Result
   {
      size_t index;
      Sequence futures;
   };

   template<class RType>
   class Future : public detail::BasicFuture<RType, Future<RType>>
   {
      using Base = detail::BasicFuture<RType, Future<RType>>;
      friend class detail::FutureHelper;

   public:
      Future() noexcept = default;
      Future(Future&& other) noexcept = default;
      Future(const Future& other) = delete;
      Future(Future<Future>&& other) noexcept : Base(std::forward<Future<Future>>(other)) { }

      Future& operator=(const Future& other) = delete;
      Future& operator=(Future&& other) noexcept = default;
   };

   template<class RType>
   class Future<RType&> : public detail::BasicFuture<RType&, Future<RType&>>
   {
      using Base = detail::BasicFuture<RType&, Future<RType&>>;
      friend class detail::FutureHelper;
      
   public:
      Future() noexcept = default;
      Future(Future&& other) noexcept = default;
      Future(const Future& other) = delete;
      Future(Future<Future>&& other) noexcept : Base(std::forward<Future<Future>>(other)) { }

      Future& operator=(const Future& other) = delete;
      Future& operator=(Future&& other) noexcept = default;
   };

   template<>
   class Future<void> : public detail::BasicFuture<void, Future<void>>
   {
      using Base = detail::BasicFuture<void, Future>;
      friend class detail::FutureHelper;
      
   public:
      Future() noexcept = default;
      Future(Future&& other) noexcept = default;
      Future(const Future& other) = delete;
      Future(Future<Future>&& other) noexcept : Base(std::forward<Future<Future>>(other)) { }

      Future& operator=(const Future& other) = delete;
      Future& operator=(Future&& other) noexcept = default;
   };

   template<class R>
   class Promise : public detail::BasicPromise<R>
   {
      using Base = detail::BasicPromise<R>;
      friend class detail::FutureHelper;

   public:
      Promise() = default;
      Promise(Promise&& other) noexcept = default;
      Promise(const Promise& other) = delete;

      template<class Alloc>
      Promise(std::allocator_arg_t arg, const Alloc& alloc) : Base(arg, alloc) { }

      Promise& operator=(const Promise& rhs) = delete;
      Promise& operator=(Promise&& other) noexcept = default;

      void Swap(Promise& other) noexcept { Base::Swap(other); }

      void Set_Value(const R& value)                { SetValue(value); }
      void Set_Value(R&& value)                     { SetValue(std::forward<R>(value)); }
      void Set_Value_At_Thread_Exit(const R& value) { SetValueAtThreadExit(value); }
      void Set_Value_At_Thread_Exit(R&& value)      { SetValueAtThreadExit(std::forward<R>(value)); }
      void Set_Exception(std::exception_ptr exceptionPtr) { SetException(exceptionPtr); }
      void Set_Exception_At_Thread_Exit(std::exception_ptr exceptionPtr) { SetExceptionAtThreadExit(exceptionPtr); }

   private:
      bool Try_Set_Value_(const R& value) { return TrySetValue(value); }
      bool Try_Set_Value_(R&& value) { return TrySetValue(std::forward<R>(value)); }
   };

   template<class R>
   class Promise<R&> : public detail::BasicPromise<R&>
   {
      using Base = detail::BasicPromise<R&>;
      friend class detail::FutureHelper;

   public:
      Promise() = default;
      Promise(Promise&& other) noexcept = default;
      Promise(const Promise& other) = delete;

      template<class Alloc>
      Promise(std::allocator_arg_t arg, const Alloc& alloc) : Base(arg, alloc) { }

      Promise& operator=(const Promise& rhs) = delete;
      Promise& operator=(Promise&& other) noexcept = default;

      void Swap(Promise& other) noexcept { Base::Swap(other); }

      void Set_Value(R& value)                { SetValue(value); }
      void Set_Value_At_Thread_Exit(R& value) { SetValueAtThreadExit(value); }
      void Set_Exception(std::exception_ptr exceptionPtr) { SetException(exceptionPtr); }
      void Set_Exception_At_Thread_Exit(std::exception_ptr exceptionPtr) { SetExceptionAtThreadExit(exceptionPtr); }

   private:
      bool Try_Set_Value_(R& value) { return TrySetValue(&value); }
   };

   template<>
   class Promise<void> : public detail::BasicPromise<void>
   {
      using Base = detail::BasicPromise<void>;
      friend class detail::FutureHelper;

   public:
      Promise() = default;
      Promise(Promise&& other) noexcept = default;
      Promise(const Promise& other) = delete;

      template<class Alloc>
      Promise(std::allocator_arg_t arg, const Alloc& alloc) : Base(arg, alloc) { }

      Promise& operator=(const Promise& rhs) = delete;
      Promise& operator=(Promise&& other) noexcept = default;

      void Swap(Promise& other) noexcept { Base::Swap(other); }

      void Set_Value() { SetValue(); }
      void Set_Value_At_Thread_Exit() { SetValueAtThreadExit(); }
      void Set_Exception(std::exception_ptr exceptionPtr) { SetException(exceptionPtr); }
      void Set_Exception_At_Thread_Exit(std::exception_ptr exceptionPtr) { SetExceptionAtThreadExit(exceptionPtr); }

   private:
      bool Try_Set_Value_() { return TrySetValue(); }
   };

   template<class R> 
   class Shared_Future : public detail::BasicFuture<R, Shared_Future<R>>
   {
      using Base = detail::BasicFuture<R, Shared_Future<R>>;
      friend class detail::FutureHelper;

   public:

      Shared_Future() noexcept = default;
      Shared_Future(const Shared_Future& other) = default;
      Shared_Future(Future<R>&& other) noexcept : Base(std::forward<Future<R>>(other)) { }
      Shared_Future(Shared_Future&& other) noexcept = default;
      Shared_Future(Future<Shared_Future>&& other) noexcept : Base(std::forward<Future<Shared_Future>>(other)) { }

      Shared_Future& operator=(const Shared_Future& other) = default;
      Shared_Future& operator=(Shared_Future&& other) noexcept = default;

      Shared_Future Share() = delete;
   };

   template<class R> 
   class Shared_Future<R&> : public detail::BasicFuture<R&, Shared_Future<R&>>
   {
      using Base = detail::BasicFuture<R&, Shared_Future<R&>>;
      friend class detail::FutureHelper;

   public:

      Shared_Future() noexcept = default;
      Shared_Future(const Shared_Future& other) = default;
      Shared_Future(Future<R&>&& other) noexcept : Base(std::forward<Future<R&>>(other)) { }
      Shared_Future(Shared_Future&& other) noexcept = default;
      Shared_Future(Future<Shared_Future>&& other) noexcept : Base(std::forward<Future<Shared_Future>>(other)) { }

      Shared_Future& operator=(const Shared_Future& other) = default;
      Shared_Future& operator=(Shared_Future&& other) noexcept = default;

      Shared_Future Share() = delete;
   };

   template<>
   class Shared_Future<void> : public detail::BasicFuture<void, Shared_Future<void>>
   {
      using Base = detail::BasicFuture<void, Shared_Future<void>>;
      friend class detail::FutureHelper;

   public:

      Shared_Future() noexcept = default;
      Shared_Future(const Shared_Future& other) = default;
      Shared_Future(Future<void>&& other) noexcept : Base(std::forward<Future<void>>(other)) { }
      Shared_Future(Shared_Future&& other) noexcept = default;

      Shared_Future(Future<Shared_Future>&& other) noexcept : Base(std::forward<Future<Shared_Future>>(other)) { }

      Shared_Future& operator=(const Shared_Future& other) = default;
      Shared_Future& operator=(Shared_Future&& other) noexcept = default;

      Shared_Future Share() = delete;
   };

   template<class R, class... ArgTypes>
   class Packaged_Task<R(ArgTypes...)> : public detail::BasicTask<R, ArgTypes...>
   {
      using Base = detail::BasicTask<R, ArgTypes...>;

   public: 
      Packaged_Task() noexcept = default;
      Packaged_Task(const Packaged_Task&) = delete;
      Packaged_Task(Packaged_Task&& other) noexcept = default;

      template<class F, class = detail::enable_if_not_same<Packaged_Task, F>>
      explicit Packaged_Task(F&& task)
         : Base(std::forward<F>(task)) { }

      template<class F, class Allocator, class = detail::enable_if_not_same<Packaged_Task, F>>
      Packaged_Task(std::allocator_arg_t arg, const Allocator& alloc, F&& task)
         : Base(arg, alloc, std::forward<F>(task)) { }

      Packaged_Task& operator=(const Packaged_Task&) = delete;
      Packaged_Task& operator=(Packaged_Task&& other) noexcept = default;

      void Swap(Packaged_Task& other) { Base::Swap(other); }
   };

   template<class T>
   Future<detail::decay_future_result_t<T>>
   Make_Ready_Future(T&& value)
   {
      using result_t = detail::decay_future_result_t<T>;
      Promise<result_t> promise;
      promise.Set_Value(std::forward<result_t>(value));
      return promise.Get_Future();
   }

   inline Future<void> Make_Ready_Future()
   {
      Promise<void> promise;
      promise.Set_Value();
      return promise.Get_Future();
   }

   template<class T>
   Future<T> Make_Exceptional_Future(std::exception_ptr ex)
   {
      Promise<T> promise;
      promise.Set_Exception(ex);
      return promise.Get_Future();
   }

   template<class T, class E>
   Future<T> Make_Exceptional_Future(E ex)
   {
      Promise<T> promise;
      promise.Set_Exception(std::make_exception_ptr(ex));
      return promise.Get_Future();
   }

   template<class InputIterator>
   detail::all_futures_vector<InputIterator>
   When_All(InputIterator first, InputIterator last)
   {
      return detail::FutureHelper::When_All(first, last);
   }

   template<class... Futures>
   detail::all_futures_tuple<Futures...>
   When_All(Futures&&... futures)
   {
      return detail::FutureHelper::When_All(std::forward<Futures>(futures)...);
   }

   template<class InputIterator>
   detail::any_future_result_vector<InputIterator>
   When_Any(InputIterator first, InputIterator last)
   {
      return detail::FutureHelper::When_Any(first, last);
   }

   template<class... Futures>
   detail::any_futures_result_tuple<Futures...>
   When_Any(Futures&&... futures)
   {
      return detail::FutureHelper::When_Any(std::forward<Futures>(futures)...);
   }
}

namespace std
{
   template<class RType, class Alloc>
   struct uses_allocator<eventual::Promise<RType>, Alloc> : true_type { };

   template<class RType, class Alloc>
   struct uses_allocator<eventual::Packaged_Task<RType>, Alloc> : true_type { };

   template<class Function, class... Args>
   void swap(eventual::Packaged_Task<Function(Args...)>& lhs, eventual::Packaged_Task<Function(Args...)>& rhs) noexcept
   {
      lhs.Swap(rhs);
   }

   template<class RType>
   void swap(eventual::Promise<RType> &lhs, eventual::Promise<RType> &rhs) noexcept
   {
      lhs.Swap(rhs);
   }
}

namespace eventual
{
namespace detail
{
   decltype(auto) FutureHelper::When_All()
   {
      return Make_Ready_Future(std::tuple<>());
   }
   
   template<class TFuture>
   TFuture FutureHelper::CreateFuture(const future_state<TFuture>& state)
   {
      auto future = TFuture();
      future._state = state;
      return future;
   }

   Future<When_Any_Result<std::tuple<>>> FutureHelper::When_Any()
   {
      using result_t = When_Any_Result<std::tuple<>>;

      auto result = result_t();
      result.index = static_cast<size_t>(-1);

      return Make_Ready_Future<result_t>(std::move(result));
   }

   void ThrowFutureError(future_errc error)
   {
      auto code = std::make_error_code(error);
      throw future_error(code);
   }

   std::exception_ptr CreateFutureExceptionPtr(future_errc error)
   {
      std::exception_ptr ptr;
      try
      {
         ThrowFutureError(error);
      }
      catch (const future_error&)
      {
         ptr = std::current_exception();
      }
      return ptr;
   }
}
}