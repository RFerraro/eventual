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
#include <type_traits>
#include <vector>
#include <future>
#include <exception>
#include <utility>
#include <tuple>
#include <stack>
#include <mutex>
#include <chrono>

#include "traits.h"
#include "utility.h"

namespace eventual
{
    template <class R> class Future;
    template <class R> class Shared_Future;

    template<class Sequence>
    struct When_Any_Result;

    inline Future<void> Make_Ready_Future();

    namespace detail
    {
        class FutureHelper;

        // Aliases

        using future_errc = std::future_errc;
        using future_error = std::future_error;
        using future_status = std::future_status;

        template<class InputIterator>
        using all_futures_vector = Future<std::vector<typename std::iterator_traits<InputIterator>::value_type>>;

        template<class... Futures>
        using all_futures_tuple = Future<std::tuple<std::decay_t<Futures>...>>;

        template<class InputIterator>
        using any_future_result_vector = Future<When_Any_Result<std::vector<typename std::iterator_traits<InputIterator>::value_type>>>;

        template<class... Futures>
        using any_futures_result_tuple = Future<When_Any_Result<std::tuple<std::decay_t<Futures>...>>>;

        template<class F>
        class SharedFunction
        {
        public:
            SharedFunction() = default;
            SharedFunction(F&& function)
                : _function(std::make_shared<F>(std::forward<F>(function)))
            { }

            template<class Allocator>
            SharedFunction(const Allocator& alloc, F&& function)
                : _function(std::allocate_shared<F>(alloc, std::forward<F>(function)))
            { }

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
        auto MakeSharedFunction(F&& function)
        {
            return SharedFunction<std::decay_t<F>>(std::move(function));
        }

        template<class Allocator, class F>
        auto AllocateSharedFunction(const Allocator& alloc, F&& function)
        {
            return SharedFunction<std::decay_t<F>>(alloc, std::move(function));
        }

        template<class Allocator, class F, class R, class... Args>
        std::enable_if_t<std::is_constructible<std::function<R(Args...)>, std::allocator_arg_t, Allocator, F>::value, std::function<R(Args...)>>
            CreateSharedFunction(const Allocator& alloc, F&& function)
        {
            return std::function<R(Args...)>(std::allocator_arg_t(), alloc, AllocateSharedFunction(alloc, std::forward<F>(function)));
        }

        template<class Allocator, class F, class R, class... Args>
        std::enable_if_t<!std::is_constructible<std::function<R(Args...)>, std::allocator_arg_t, Allocator, F>::value, std::function<R(Args...)>>
            CreateSharedFunction(const Allocator& alloc, F&& function)
        {
            // because libstdc++...
            return std::function<R(Args...)>(AllocateSharedFunction(alloc, std::forward<F>(function)));
        }

        class BasicState
        {
        protected:
            using unique_lock = std::unique_lock<std::mutex>;

        public:
            BasicState() :
                _retrieved(false),
                _hasResult(false),
                _exception(nullptr),
                _block(std::make_shared<NotifierBlock>())
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
                if (_exception)
                    std::rethrow_exception(_exception);
            }

        private:

            struct NotifierBlock
            {
                NotifierBlock()
                    : _ready(0), _mutex(), _condition(), _callbacks()
                { }

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

                Wait(lock);
                CheckException();
                return std::move(_result);
            }

            bool SetResult(const R& value)
            {
                if (!SetResultImpl(value))
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

            void SetResult(const State& other)
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

        template<class R>
        class BasicPromise
        {
            using SharedState = std::shared_ptr<State<R>>;
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

            Future<R> Get_Future();

        protected:

            template<class... T>
            void SetValue(T&&... value)
            {
                auto state = ValidateState();

                if (!state->SetResult(std::forward<T>(value)...))
                    throw CreateFutureError(future_errc::promise_already_satisfied);
            }

            template<class... T>
            void SetValueAtThreadExit(T&&... value)
            {
                auto state = ValidateState();

                if (!state->SetResultAtThreadExit(std::forward<T>(value)...))
                    throw CreateFutureError(future_errc::promise_already_satisfied);
            }

            void SetException(std::exception_ptr exceptionPtr)
            {
                auto state = ValidateState();

                if (!state->SetException(exceptionPtr))
                    throw CreateFutureError(future_errc::promise_already_satisfied);
            }

            void SetExceptionAtThreadExit(std::exception_ptr exceptionPtr)
            {
                auto state = ValidateState();

                if (!state->SetExceptionAtThreadExit(exceptionPtr))
                    throw CreateFutureError(future_errc::promise_already_satisfied);
            }

            template<class... T>
            bool TrySetValue(T&&... value) noexcept
            {
                auto state = ValidateState();
                return state->SetResult(std::forward<T>(value)...);
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
                if (!state)
                    throw CreateFutureError(future_errc::no_state);

                return state;
            }

        private:

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
                : Base(arg, alloc), _task(CreateSharedFunction<Allocator, F, R, ArgTypes...>(alloc, std::forward<F>(task)))
            { }

            template<class F, class = detail::enable_if_not_same<BasicTask, F>>
            BasicTask(F&& task)
                : Base(), _task(MakeSharedFunction(std::forward<F>(task)))
            { }

            BasicTask& operator=(BasicTask&& other) noexcept
            {
                _task = _task(std::move(other._task));
                Base::operator=(std::forward<BasicTask>(other));
                return *this;
            }

            void Swap(BasicTask& other) noexcept
            {
                Base::Swap(other);
                std::swap(_task, other._task);
            }

            void Reset()
            {
                Base::ValidateState();

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
                        Base::SetException(std::current_exception());
                    else
                        throw;
                }
                catch (...)
                {
                    Base::SetException(std::current_exception());
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
                        Base::SetExceptionAtThreadExit(std::current_exception());
                    else
                        throw;
                }
                catch (...)
                {
                    Base::SetExceptionAtThreadExit(std::current_exception());
                }
            }

        private:

            template<typename T>
            std::enable_if_t<!std::is_void<T>::value>
                ExecuteTask(ArgTypes... args)
            {
                Base::SetValue(_task(std::forward<ArgTypes>(args)...));
            }

            template<typename T>
            std::enable_if_t<std::is_void<T>::value>
                ExecuteTask(ArgTypes... args)
            {
                _task(std::forward<ArgTypes>(args)...);
                Base::SetValue();
            }

            template<typename T>
            std::enable_if_t<!std::is_void<T>::value>
                ExecuteTaskDeferred(ArgTypes... args)
            {
                Base::SetValueAtThreadExit(_task(std::forward<ArgTypes>(args)...));
            }

            template<typename T>
            std::enable_if_t<std::is_void<T>::value>
                ExecuteTaskDeferred(ArgTypes... args)
            {
                _task(std::forward<ArgTypes>(args)...);
                Base::SetValueAtThreadExit();
            }

            std::function<R(ArgTypes...)> _task;
        };

        template<class R>
        class BasicFuture
        {
            using State = detail::State<R>;
            using SharedState = std::shared_ptr<State>;
            using unique_lock = std::unique_lock<std::mutex>;

            template<class>
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

            Shared_Future<R> Share();

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

        protected:

            template<class T>
            static decltype(auto) GetResult(BasicFuture<T>& future)
            {
                auto state = future.ValidateState();
                future.ResetState();
                return state->GetResult();
            }

            template<class T>
            static decltype(auto) GetSharedResult(BasicFuture<T>& future)
            {
                auto state = future.ValidateState();
                return state->GetResult();
            }

            static void GetResult(BasicFuture<void>& future)
            {
                auto state = future.ValidateState();
                future.ResetState();
                state->GetResult();
            }

            static void GetSharedResult(BasicFuture<void>& future)
            {
                auto state = future.ValidateState();
                state->GetResult();
            }

            template<template<typename> class NestedFuture>
            BasicFuture(BasicFuture<NestedFuture<R>>&& other)
                : BasicFuture(SharedState(new State()))
            {
                auto outerState = std::move(other._state);
                auto futureState = _state;

                // todo: Yo dawg; I heard you like continuations...
                outerState->SetCallback([outerState, futureState]()
                {
                    if (!outerState->HasException())
                    {
                        auto innerState = outerState->GetResult()._state;
                        if (innerState)
                        {
                            innerState->SetCallback([futureState, innerState]()
                            {
                                futureState->SetResult(*innerState);
                            });
                        }
                        else
                            futureState->SetException(CreateFutureExceptionPtr(future_errc::no_state));
                    }
                    else
                        futureState->SetException(outerState->GetException());
                });
            }

            SharedState ValidateState() const
            {
                auto state = _state;
                if (!state)
                    throw CreateFutureError(future_errc::no_state);

                return state;
            }

            //todo: SFINAE way to prevent [](Future&){...} continuations (clang doesn't like them...)
            template<class TContinuation, class TFuture>
            static decltype(auto) ThenImpl(TContinuation&& continuation, TFuture&& future)
            {
                using namespace std;

                using result_t = result_of_t<TContinuation(TFuture)>;

                auto current = forward<TFuture>(future);
                auto state = current.ValidateState();

                BasicTask<result_t, TFuture> task(forward<TContinuation>(continuation));
                auto taskFuture = Unwrap(task.Get_Future());

                state->SetCallback(
                    [
                        task = move(task),
                        current = move(current)
                    ]() mutable
                {
                    task(move(current));
                });

                return taskFuture;
            }

        private:

            template<typename T, template<typename> class TOuter, template<typename> class TInner>
            static enable_if_future_t<TInner<T>> Unwrap(TOuter<TInner<T>>&& future)
            {
                return TInner<T>(std::forward<TOuter<TInner<T>>>(future));
            }

            template<typename T, template<typename> class TOuter>
            static enable_if_not_future_t<T, TOuter<T>> Unwrap(TOuter<T>&& future)
            {
                return std::forward<TOuter<T>>(future);
            }

            void ResetState()
            {
                _state.reset();
            }

            SharedState _state;
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
            static decltype(auto) When_Any(enable_if_iterator_t<InputIterator> begin, InputIterator end);

            inline static Future<When_Any_Result<std::tuple<>>> When_Any();

            template<class... Futures>
            static decltype(auto) When_Any(Futures&&... futures);

            template<class InputIterator>
            static decltype(auto) When_All(enable_if_iterator_t<InputIterator> first, InputIterator last)
            {
                using namespace std;
                using future_vector_t = vector<typename iterator_traits<InputIterator>::value_type>;

                auto vectorFuture = Make_Ready_Future(future_vector_t());

                for (auto iterator = first; iterator != last; ++iterator)
                {
                    auto& future = *iterator;
                    vectorFuture = vectorFuture.Then(
                        [future = move(future)](auto vf) mutable
                    {
                        return future.Then(
                            [vf = move(vf)](auto future) mutable
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
                return tailFuture.Then([head = move(head)](auto tf) mutable
                {
                    return head.Then([tf = move(tf)](auto h) mutable
                    {
                        return tuple_cat(make_tuple(move(h)), tf.Get());
                    });
                });
            }
        };
    }
}