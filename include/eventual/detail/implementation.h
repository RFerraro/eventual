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
#include <queue>
#include <mutex>
#include <chrono>
#include <cassert>
#include <memory>
#include <cstddef>

#include <list>

#include "traits.h"
#include "utility.h"
#include "allocation.h"

namespace eventual
{
    template <class R> class future;
    template <class R> class shared_future;

    template<class Sequence>
    struct when_any_result;

    namespace detail
    {
        template<class Future, class... Futures>
        static decltype(auto) When_All_(Future&& head, Futures&&... others);

        static future<std::tuple<>> When_All_();

        template <class R> class CommonPromise;
        template <class R> class BasicPromise;
        template <class R> class BasicFuture;

        // Aliases

        using future_errc = std::future_errc;
        using future_error = std::future_error;
        using future_status = std::future_status;

        template<class InputIterator>
        using all_futures_vector = future<std::vector<typename std::iterator_traits<InputIterator>::value_type>>;

        template<class... Futures>
        using all_futures_tuple = future<std::tuple<std::decay_t<Futures>...>>;

        template<class InputIterator>
        using any_future_result_vector = future<when_any_result<std::vector<typename std::iterator_traits<InputIterator>::value_type>>>;

        template<class... Futures>
        using any_futures_result_tuple = future<when_any_result<std::tuple<std::decay_t<Futures>...>>>;

        class FutureFactory
        {
        public:
            template<class TResult>
            static future<TResult> Create(const CommonPromise<TResult>& promise);
        };

        class FutureHelper
        {
        public:

            template<class TResult, class T>
            static void SetCallback(BasicFuture<TResult>& future, T&& callback);

            template<class TResult, class T>
            static bool TrySetValue(BasicPromise<TResult>& promise, const T& value);

            template<class TResult>
            static decltype(auto) GetState(BasicFuture<TResult>& future);

            template<class TResult>
            static decltype(auto) GetResult(BasicFuture<TResult>& future);
        };

        template<class T>
        struct PlacementDestructor
        {
            void operator()(T* item) const
            {
                item->~T();
                (void)item; // suppresses C4100 in VS2015
            }
        };

        class SimpleDelegate
        {
            class Delegate
            {
            public:
                virtual ~Delegate() { }
                virtual void operator()() = 0;
            };

            template<class T>
            class SmallDelegate : public Delegate
            {
            public:

                SmallDelegate(T&& callable)
                    : _callable(std::forward<T>(callable))
                { }

                virtual void operator()() override
                {
                    _callable();
                }

            private:
                T _callable;
            };

            template<class T>
            class LargeDelegate : public Delegate
            {
            public:

                LargeDelegate(T&& callable, polymorphic_allocator<T>&& allocator)
                    : _callable(Create(std::forward<T>(callable), allocator))
                { }

                virtual void operator()() override
                {
                    (*_callable)();
                }

            private:

                struct Deleter
                {
                    Deleter(const polymorphic_allocator<T>& resource)
                        : _resource(resource)
                    {
                    }

                    void operator()(T* functor)
                    {
                        using traits = std::allocator_traits<polymorphic_allocator<T>>;

                        traits::destroy(_resource, functor);
                        traits::deallocate(_resource, functor, 1);
                    }

                    polymorphic_allocator<T> _resource;
                };

                std::unique_ptr<T, Deleter> Create(T&& callable, polymorphic_allocator<T>& resource)
                {
                    using traits = std::allocator_traits<polymorphic_allocator<T>>;

                    auto ptr = traits::allocate(resource, 1);
                    traits::construct(resource, ptr, std::forward<T>(callable));

                    return std::unique_ptr<T, Deleter>(ptr, Deleter(resource));
                }

                std::unique_ptr<T, Deleter> _callable;
            };

            // dummy type
            struct Callable
            {
                void operator()() { }
            };
            
            //todo: fine-tune this size
            using storage_t = aligned_union_t<32, SmallDelegate<Callable>, LargeDelegate<Callable>>;

            using delegate_pointer_t = std::unique_ptr<Delegate, PlacementDestructor<Delegate>>;

        public:

            template<class T, class = typename enable_if_not_same<SimpleDelegate, T>::type>
            SimpleDelegate(T&& functor, const polymorphic_allocator<SimpleDelegate>& alloc)
                : _data(),
                  _object(Create(std::forward<T>(functor), alloc))
            { }

            // only contructable in-place. No move/copy.
            SimpleDelegate(SimpleDelegate&&) = delete;
            SimpleDelegate& operator=(const SimpleDelegate&) = delete;
            SimpleDelegate(const SimpleDelegate&) = delete;
            SimpleDelegate& operator=(SimpleDelegate&&) = default;

            void operator()() const
            {
                // this internal object should never be null
                assert(_object);

                _object->operator()();
            }

        private:

            template<class T>
            enable_if_size_is_less_than_or_eq_t<SmallDelegate<T>, storage_t, delegate_pointer_t>
            Create(T&& functor, const polymorphic_allocator<SimpleDelegate>)
            {
                static_assert(sizeof(SmallDelegate<T>) <= sizeof(storage_t), "Delegate does not fit in storage buffer.");

                return delegate_pointer_t(new(&_data) SmallDelegate<T>(std::forward<T>(functor)));
            }

            template<class T>
            enable_if_size_is_greater_than_t<SmallDelegate<T>, storage_t, delegate_pointer_t>
            Create(T&& functor, const polymorphic_allocator<SimpleDelegate>& alloc)
            {
                static_assert(sizeof(SmallDelegate<T>) > sizeof(storage_t), "Incorrect delegate choosen.");
                static_assert(sizeof(LargeDelegate<T>) <= sizeof(storage_t), "Pointer delegate does not fit in storage buffer.");

                return delegate_pointer_t(new(&_data) LargeDelegate<T>(std::forward<T>(functor), polymorphic_allocator<T>(alloc)));
            }
            
            storage_t _data;
            delegate_pointer_t _object;
        };

        template<typename T>
        class ResultBlock
        {
            using storage_t = std::aligned_storage_t<sizeof(T), alignof(T)>;
            using result_pointer_t = std::unique_ptr<T, PlacementDestructor<T>>;

        public:

            ResultBlock() { }

            ResultBlock(ResultBlock&&) = delete;
            ResultBlock(const ResultBlock&) = delete;
            ResultBlock& operator=(ResultBlock&&) = delete;
            ResultBlock& operator=(const ResultBlock&) = delete;

            template<typename R>
            void Set(R&& result)
            {
                assert(!_result);

                // placement new
                _result.reset(new(&_data) T(std::forward<R>(result)));
            }

            T get()
            {
                assert(_result);

                return std::move(*_result);
            }

            const T& get() const
            {
                assert(_result != nullptr);

                return *_result;
            }

        private:
            storage_t _data;
            result_pointer_t _result;
        };

        struct StateTag { explicit StateTag(int) { } };

        template<typename T>
        class State
        {

        private:

            using unique_lock = std::unique_lock<std::mutex>;
            using strong_reference = std::shared_ptr<State>;
            using weak_reference = std::weak_ptr<State>;
            using allocator_t = strong_polymorphic_allocator<State>;
            using delegate_allocator_t = strong_polymorphic_allocator<SimpleDelegate>;
            using callbacks_t = std::list<SimpleDelegate, delegate_allocator_t>;

        public:

            template<class Alloc>
            State(const StateTag&, const Alloc& alloc) :
                _retrieved(false),
                _hasResult(false),
                _result(),
                _exception(nullptr),
                _self(),
                _ready(0),
                _mutex(),
                _condition(),
                _allocator(allocator_t(alloc)),
                _callbacks(delegate_allocator_t(_allocator))
            { }
            
            static std::shared_ptr<State> MakeState()
            {
                return AllocState(std::allocator<State>());
            }

            template<class Alloc, class = typename enable_if_not_same<State, Alloc>::type>
            static std::shared_ptr<State> AllocState(const Alloc& alloc)
            {
                auto state = std::allocate_shared<State>(alloc, StateTag(0), alloc);

                state->SetNotifier(state);

                return state;
            }

            State(State&&) = delete;
            State(const State&) = delete;
            State& operator=(State&&) = delete;
            State& operator=(const State&) = delete;

            const allocator_t& Get_Allocator() const
            {
                return _allocator;
            }

            bool Is_Ready() const { return _ready != 0; }

            void Wait() const
            {
                auto lock = AquireLock();
                Wait(lock);
            }

            template <class TDuration>
            bool Wait_For(const TDuration& rel_time)
            {
                auto lock = AquireLock();
                return _condition.wait_for(lock, rel_time, [this]() { return Is_Ready(); });
            }

            template <class TTime>
            bool Wait_Until(const TTime& abs_time)
            {
                auto lock = AquireLock();
                return _condition.wait_until(lock, abs_time, [this]() { return Is_Ready(); });
            }

            template<class TCallback>
            void SetCallback(TCallback&& callback)
            {
                {
                    auto lock = AquireLock();

                    if (!Is_Ready())
                    {
                        _callbacks.emplace_back(std::forward<TCallback>(callback), _callbacks.get_allocator());
                        return;
                    }
                }

                //promise is ready, invoke immediately
                callback();
            }

            void NotifyCompletion()
            {
                {
                    auto lock = AquireLock();
                    _ready = true;
                    _condition.notify_all();
                }

                InvokeContinuations();
            }

            bool HasException() { return _exception ? true : false; }
            std::exception_ptr GetException() { return _exception; }
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

            T GetResult()
            {
                auto lock = AquireLock();

                Wait(lock);
                CheckException();
                return _result.get();
            }

            const T& GetResult() const
            {
                auto lock = AquireLock();

                Wait(lock);
                CheckException();
                return _result.get();
            }

            template<class TValue>
            bool SetResult(TValue&& value)
            {
                {
                    auto lock = AquireLock();
                    
                    if (!SetHasResult())
                        return false;

                    _result.Set(std::forward<TValue>(value));
                }

                NotifyPromiseFullfilled();
                return true;
            }

            template<class TValue>
            bool SetResultAtThreadExit(TValue&& value)
            {
                {
                    auto lock = AquireLock();
                    
                    if (!SetHasResult())
                        return false;

                    _result.Set(std::forward<TValue>(value));
                }

                NotifyPromiseFullfilledAtThreadExit();
                return true;
            }

        protected:

            strong_reference GetNotifier()
            {
                return _self.lock();
            }

            void SetNotifier(const weak_reference& notifier)
            {
                _self = notifier;
            }

            unique_lock AquireLock() const
            {
                return unique_lock(_mutex);
            }

            void InvokeContinuations()
            {
                for (auto& cb : _callbacks)
                    cb(); // todo: exceptions?
            }

            void Wait(unique_lock& lock) const
            {
                _condition.wait(lock, [this]() { return Is_Ready(); });
            }

        private:

            void NotifyPromiseFullfilled()
            {
                NotifyCompletion();
            }

            void NotifyPromiseFullfilledAtThreadExit()
            {
                auto block = _self.lock();
                assert(block);

                using namespace std;
                class ExitNotifier
                {
                public:
                    ExitNotifier() = default;
                    ExitNotifier(const ExitNotifier&) = delete;
                    ExitNotifier& operator=(const ExitNotifier&) = delete;

                    ~ExitNotifier() noexcept
                    {
                        while (!_exitFunctions.empty())
                        {
                            _exitFunctions.front()->NotifyCompletion();
                            _exitFunctions.pop();
                        }
                    }

                    void Add(strong_reference&& block)
                    {
                        _exitFunctions.emplace(forward<strong_reference>(block));
                    }

                private:
                    queue<strong_reference> _exitFunctions;
                };

                thread_local ExitNotifier notifier;
                notifier.Add(move(block));
            }

            bool SetHasResult()
            {
                if (_hasResult)
                    return false;

                _hasResult = true;
                return _hasResult;
            }

            void CheckException() const
            {
                if (_exception)
                    std::rethrow_exception(_exception);
            }

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
            ResultBlock<T> _result;
            std::exception_ptr _exception;
            weak_reference _self;

            int _ready;
            mutable std::mutex _mutex;
            mutable std::condition_variable _condition;
            allocator_t _allocator;
            callbacks_t _callbacks;
        };

        template<typename TPrimaryState, typename TSecondaryState>
        class CompositeState : public TPrimaryState, public TSecondaryState
        {
        public:

            template<class Alloc>
            CompositeState(const StateTag& tag, const Alloc& alloc)
                : TPrimaryState(tag, alloc), TSecondaryState(tag, alloc)
            {
                SetCallback([this]() mutable
                {
                    if (HasException())
                    {
                        TSecondaryState::SetException(GetException());
                        return;
                    }

                    auto innerFuture = GetResult();
                    if (!innerFuture.valid())
                    {
                        TSecondaryState::SetException(CreateFutureExceptionPtr(future_errc::broken_promise));
                        return;
                    }

                    auto futureState = TSecondaryState::GetNotifier();
                    innerFuture.then([futureState = std::move(futureState)](auto future)
                    {
                        CompositeState::SetResultFromFuture(*futureState, future);
                    });
                });
            }

            template<class TCallback>
            void SetCallback(TCallback&& callback)
            {
                TPrimaryState::SetCallback(std::forward<TCallback>(callback));
            }

            polymorphic_allocator<TPrimaryState> Get_Allocator()
            {
                return TPrimaryState::Get_Allocator();
            }

            bool HasResult() { return TPrimaryState::HasResult(); }

            bool SetRetrieved() { return TPrimaryState::SetRetrieved(); }

            bool HasException()
            {
                return TPrimaryState::HasException();
            }

            std::exception_ptr GetException()
            {
                return TPrimaryState::GetException();
            }

            bool SetException(std::exception_ptr ex)
            {
                return TPrimaryState::SetException(ex);
            }

            decltype(auto) GetResult()
            {
                return TPrimaryState::GetResult();
            }

            decltype(auto) GetResult() const
            {
                return TPrimaryState::GetResult();
            }

            template<class TValue>
            bool SetResult(TValue&& value)
            {
                return TPrimaryState::SetResult(std::forward<TValue>(value));
            }

            static std::shared_ptr<CompositeState> MakeState()
            {
                return AllocState(std::allocator<CompositeState>());
            }

            template<class Alloc, class = typename enable_if_not_same<CompositeState, Alloc>::type>
            static std::shared_ptr<CompositeState> AllocState(const Alloc& alloc)
            {
                auto state = std::allocate_shared<CompositeState>(alloc, StateTag(0), alloc);

                state->SetNotifier(state);

                return state;
            }

        protected:

            void SetNotifier(const std::weak_ptr<CompositeState>& notifier)
            {
                TPrimaryState::SetNotifier(notifier);
                TSecondaryState::SetNotifier(notifier);
            }

            std::shared_ptr<TPrimaryState> GetNotifier()
            {
                return TPrimaryState::GetNotifier();
            }

        private:
            template<class TState, class T>
            static void SetResultFromFuture(TState& state, BasicFuture<T>& future);
        };

        template<class R>
        class CommonPromise
        {
            friend class BasicFuture<R>;

        protected:
            using StateType = get_state_t<R>;
            using SharedState = get_shared_state_t<R>;

        public:

            CommonPromise() : _state(StateType::MakeState()) { }
            CommonPromise(const CommonPromise& other) = delete;
            CommonPromise(CommonPromise&& other) noexcept = default;

            CommonPromise(SharedState state) : _state(state) { }

            template<class Alloc>
            CommonPromise(std::allocator_arg_t, const Alloc& alloc) :
                _state(StateType::AllocState(alloc))
            { }

            ~CommonPromise() noexcept
            {
                auto state = _state;
                if (!state || state->HasResult())
                    return;

                state->SetException(CreateFutureExceptionPtr(future_errc::broken_promise));
            }

            CommonPromise& operator=(const CommonPromise& rhs) = delete;
            CommonPromise& operator=(CommonPromise&& other) noexcept = default;

            void swap(CommonPromise& other) noexcept
            {
                _state.swap(other._state);
            }

            future<R> get_future();

        protected:

            template<class TValue>
            void SetValue(TValue&& value)
            {
                auto state = ValidateState();

                if (!state->SetResult(std::forward<TValue>(value)))
                    throw CreateFutureError(future_errc::promise_already_satisfied);
            }

            template<class TValue>
            void SetValueAtThreadExit(TValue&& value)
            {
                auto state = ValidateState();

                if (!state->SetResultAtThreadExit(std::forward<TValue>(value)))
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

            template<class TValue>
            bool TrySetValue(TValue&& value)
            {
                auto state = ValidateState();

                return state->SetResult(std::forward<TValue>(value));
            }

            void Reset()
            {
                auto state = ValidateState();

                const auto& allocator = state->Get_Allocator();
                auto newState = StateType::AllocState(allocator);

                std::swap(_state, newState);
            }

            bool Valid() const noexcept
            {
                auto state = _state;
                return state ? true : false;
            }

            SharedState ValidateState() const
            {
                auto state = _state;
                if (!state)
                    throw CreateFutureError(future_errc::no_state);

                return state;
            }

            SharedState CopyState() const
            {
                auto state = ValidateState();

                if (!state->SetRetrieved())
                    throw CreateFutureError(future_errc::future_already_retrieved);

                return state;
            }

        private:

            SharedState _state;
        };

        template<class R>
        class BasicPromise : public CommonPromise<R>
        {
            using Base = CommonPromise<R>;
            using SharedState = typename Base::SharedState;

            friend class FutureHelper;
            friend class BasicFuture<R>;

        public:
            BasicPromise() { }
            BasicPromise(const BasicPromise& other) = delete;
            BasicPromise(BasicPromise&& other) noexcept = default;

            BasicPromise(SharedState state) 
                : Base(state) { }

            template<class Alloc>
            BasicPromise(std::allocator_arg_t arg, const Alloc& alloc) 
                : Base(arg, alloc) { }

            BasicPromise& operator=(const BasicPromise& rhs) = delete;
            BasicPromise& operator=(BasicPromise&& other) noexcept = default;

            void set_exception(std::exception_ptr exceptionPtr) { Base::SetException(exceptionPtr); }
            void set_exception_at_thread_exit(std::exception_ptr exceptionPtr) { Base::SetExceptionAtThreadExit(exceptionPtr); }

        protected:

            template<class TValue>
            void SetValue(TValue&& value)
            {
                Base::SetValue(std::forward<TValue>(value));
            }

            template<class TValue>
            void SetValueAtThreadExit(TValue&& value)
            {
                Base::SetValueAtThreadExit(std::forward<TValue>(value));
            }

            void SetException(std::exception_ptr exceptionPtr)
            {
                Base::SetException(exceptionPtr);
            }

            void SetExceptionAtThreadExit(std::exception_ptr exceptionPtr)
            {
                Base::SetExceptionAtThreadExit(exceptionPtr);
            }
        };

        template<class TFunctor, class R, class... ArgTypes>
        class BasicTask : public CommonPromise<R>
        {
            using Base = CommonPromise<R>;

        public:

            BasicTask() noexcept : Base(nullptr) { }
            BasicTask(const BasicTask&) = delete;
            BasicTask(BasicTask&& other) noexcept
                : Base(std::forward<BasicTask>(other)), _task(std::move(other._task))
            { }

            template<class F, class Allocator, class = typename enable_if_not_same<BasicTask, F>::type>
            BasicTask(std::allocator_arg_t arg, const Allocator& alloc, F&& task)
                : Base(arg, alloc), _task(CreateFunctor<TFunctor>(alloc, std::forward<F>(task)))
            { }

            template<class F, class = typename enable_if_not_same<BasicTask, F>::type>
            BasicTask(F&& task)
                : Base(), _task(std::forward<F>(task))
            { }

            BasicTask& operator=(BasicTask&& other) noexcept
            {
                _task = std::move(other._task);
                Base::operator=(std::forward<BasicTask>(other));
                return *this;
            }

            void reset()
            {
                Base::Reset();
            }

            bool valid() const { return Base::Valid(); }

            void operator()(ArgTypes... args)
            {
                try
                {
                    Base::SetValue(ExecuteTask<R>(std::forward<ArgTypes>(args)...));
                }
                catch (const future_error& error)
                {
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

            void make_ready_at_thread_exit(ArgTypes... args)
            {
                try
                {
                    Base::SetValueAtThreadExit(ExecuteTask<R>(std::forward<ArgTypes>(args)...));
                }
                catch (const future_error& error)
                {
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

            void swap(BasicTask& other) noexcept
            {
                Base::swap(other);
                std::swap(_task, other._task);
            }

        private:

            template<class TResult>
            std::enable_if_t<!std::is_void<TResult>::value, TResult>
            ExecuteTask(ArgTypes... args)
            {
                return _task(std::forward<ArgTypes>(args)...);
            }

            template<class TResult>
            std::enable_if_t<std::is_void<TResult>::value, Unit>
            ExecuteTask(ArgTypes... args)
            {
                _task(std::forward<ArgTypes>(args)...);
                return Unit();
            }

            TFunctor _task;
        };

        template<class R>
        class BasicFuture
        {
            template<class>
            friend class BasicFuture;
            friend class FutureHelper;

        protected:
            using SharedState = get_shared_state_t<R>;

        public:
            BasicFuture() noexcept = default;
            BasicFuture(const BasicFuture& other) = default;
            BasicFuture(BasicFuture&& other) noexcept = default;

            BasicFuture(const CommonPromise<R>& promise)
                : BasicFuture(promise.CopyState()) { }

            BasicFuture(SharedState&& state) noexcept 
                : _state(std::forward<SharedState>(state)) { }

            BasicFuture& operator=(const BasicFuture& rhs) = default;
            BasicFuture& operator=(BasicFuture&& other) noexcept = default;

            bool valid() const noexcept { return _state ? true : false; }
            bool is_ready() const noexcept { return (_state && _state->Is_Ready()); }

            void wait() const
            {
                auto state = ValidateState();
                state->Wait();
            }

            template <class Rep, class Period>
            future_status wait_for(const std::chrono::duration<Rep, Period>& rel_time) const
            {
                auto state = ValidateState();
                return state->Wait_For(rel_time) ? future_status::ready : future_status::timeout;
            }

            template <class Clock, class Duration>
            future_status wait_until(const std::chrono::time_point<Clock, Duration>& abs_time) const
            {
                auto state = ValidateState();
                return state->Wait_Until(abs_time) ? future_status::ready : future_status::timeout;
            }

        protected:

            template<template<typename> class NestedFuture>
            BasicFuture(BasicFuture<NestedFuture<R>>&& other) noexcept
                : BasicFuture(std::move(other._state))
            { }

            SharedState ValidateState() const
            {
                auto state = _state;
                if (!state)
                    throw CreateFutureError(future_errc::no_state);

                return state;
            }

            decltype(auto) GetResult()
            {
                auto state = ValidateState();
                ResetState();
                return state->GetResult();
            }

            decltype(auto) GetResult() const
            {
                auto state = ValidateState();
                const auto& stateRef = *state;
                return stateRef.GetResult();
            }

            SharedState MoveState()
            {
                auto state = std::move(_state);

                if (!state)
                    throw CreateFutureError(future_errc::no_state);

                return state;
            }

            void CheckState()
            {
                auto state = _state;

                if (!state)
                    throw CreateFutureError(future_errc::no_state);
            }

            static shared_future<R> Share(future<R>&& future);

            template<class TContinuation, class TFuture>
            static decltype(auto) ThenMove(TContinuation&& continuation, TFuture& future)
            {
                return ThenImpl(decay_copy(std::forward<TContinuation>(continuation)), std::move(future));
            }

            template<class TContinuation, class TFuture>
            static decltype(auto) ThenShare(TContinuation&& continuation, const TFuture& future)
            {
                return ThenImpl(decay_copy(std::forward<TContinuation>(continuation)), TFuture(future));
            }

            template<class TContinuation, class TFuture>
            static decltype(auto) ThenImpl(TContinuation&& continuation, TFuture&& future);

        private:

            decltype(auto) GetState()
            {
                return _state;
            }

            template<class TResult>
            static decltype(auto) GetUnwrappedFuture(const CommonPromise<TResult>& promise)
            {
                return Unwrap(FutureFactory::Create(promise));
            }

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

        template<class TResult, class T>
        void FutureHelper::SetCallback(BasicFuture<TResult>& future, T&& callback)
        {
            future._state->SetCallback(std::forward<T>(callback));
        }

        template<class TResult, class T>
        bool FutureHelper::TrySetValue(BasicPromise<TResult>& promise, const T& value)
        {
            return promise.TrySetValue(value);
        }

        template<class TResult>
        decltype(auto) FutureHelper::GetState(BasicFuture<TResult>& future)
        {
            return future.GetState();
        }

        template<class TResult>
        decltype(auto) FutureHelper::GetResult(BasicFuture<TResult>& future)
        {
            return future.GetResult();
        }

        template<class TPrimaryState, class TSecondaryState>
        template<class TState, class T>
        void CompositeState<TPrimaryState, TSecondaryState>::SetResultFromFuture(TState& state, BasicFuture<T>& future)
        {
            auto innerState = FutureHelper::GetState(future);
            if (innerState->HasException())
            {
                state.SetException(innerState->GetException());
                return;
            }

            state.SetResult(FutureHelper::GetResult(future));
        }
    }
}
