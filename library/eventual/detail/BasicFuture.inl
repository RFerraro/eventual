#pragma once

#include "BasicFuture.h"
#include <utility>
#include <future>

#include "future.h"
#include "shared_future.h"

#include "State.inl"
#include "CompositeState.inl"
#include "CommonPromise.inl"
#include "BasicTask.inl"
#include "FutureFactory.inl"

namespace eventual
{   
    namespace detail
    {
        template<class R>
        BasicFuture<R>::BasicFuture(const CommonPromise<R>& promise)
            : BasicFuture(promise.CopyState())
        { }

        template<class R>
        BasicFuture<R>::BasicFuture(SharedState&& state) noexcept
            : _state(std::forward<SharedState>(state))
        { }

        template<class R>
        template<class T, template<typename> class NestedFuture>
        BasicFuture<R>::BasicFuture(BasicFuture<NestedFuture<T>>&& other) noexcept
            : BasicFuture(std::move(other._state))
        { }

        template<class R>
        bool BasicFuture<R>::valid() const noexcept
        {
            return _state ? true : false;
        }

        template<class R>
        bool BasicFuture<R>::is_ready() const noexcept
        {
            return (_state && _state->Is_Ready());
        }

        template<class R>
        void BasicFuture<R>::wait() const
        {
            auto state = ValidateState();
            state->Wait();
        }

        template<class R>
        template <class Rep, class Period>
        std::future_status BasicFuture<R>::wait_for(const std::chrono::duration<Rep, Period>& rel_time) const
        {
            auto state = ValidateState();
            return state->Wait_For(rel_time) ? std::future_status::ready : std::future_status::timeout;
        }

        template<class R>
        template <class Clock, class Duration>
        std::future_status BasicFuture<R>::wait_until(const std::chrono::time_point<Clock, Duration>& abs_time) const
        {
            auto state = ValidateState();
            return state->Wait_Until(abs_time) ? std::future_status::ready : std::future_status::timeout;
        }

        template<class R>
        typename BasicFuture<R>::SharedState BasicFuture<R>::ValidateState() const
        {
            auto state = _state;
            if (!state)
                throw CreateFutureError(std::future_errc::no_state);

            return state;
        }

        template<class R>
        template<class T>
        std::enable_if_t<std::is_void<T>::value>
            BasicFuture<R>::GetResult()
        {
            auto state = ValidateState();
            ResetState();
            state->GetResult();
        }

        template<class R>
        template<class T>
        std::enable_if_t<!std::is_void<T>::value, T>
            BasicFuture<R>::GetResult()
        {
            auto state = ValidateState();
            ResetState();
            return state->GetResult();
        }

        template<class R>
        template<class T>
        std::enable_if_t<std::is_void<T>::value>
            BasicFuture<R>::GetResultRef() const
        {
            auto state = ValidateState();
            const auto& stateRef = *state;
            stateRef.GetResult();
        }

        template<class T>
        T& lvalue(T&& value)
        {
            return value;
        }

        template<class R>
        template<class T>
        std::enable_if_t<!std::is_void<T>::value, std::decay_t<T>&>
            BasicFuture<R>::GetResultRef() const
        {
            const auto state = ValidateState();
            return lvalue(state->GetResult());
        }

        template<class R>
        typename BasicFuture<R>::SharedState BasicFuture<R>::MoveState()
        {
            auto state = std::move(_state);

            if (!state)
                throw CreateFutureError(future_errc::no_state);

            return state;
        }

        template<class R>
        void BasicFuture<R>::CheckState()
        {
            auto state = _state;

            if (!state)
                throw CreateFutureError(std::future_errc::no_state);
        }

        template<class R>
        template<class T>
        shared_future<T> BasicFuture<R>::Share(future<T>&& future)
        {
            future.CheckState();
            return shared_future<T>(std::forward<eventual::future<T>>(future));
        }

        template<class R>
        template<class TContinuation, class TFuture>
        decltype(auto) BasicFuture<R>::ThenMove(TContinuation&& continuation, TFuture& future)
        {
            return ThenImpl(decay_copy(std::forward<TContinuation>(continuation)), std::move(future));
        }

        template<class R>
        template<class TContinuation, class TFuture>
        decltype(auto) BasicFuture<R>::ThenShare(TContinuation&& continuation, const TFuture& future)
        {
            return ThenImpl(decay_copy(std::forward<TContinuation>(continuation)), TFuture(future));
        }

        template<class R>
        template<class TContinuation, class TFuture>
        decltype(auto) BasicFuture<R>::ThenImpl(TContinuation&& continuation, TFuture&& future)
        {
            using namespace std;
            using task_t = get_continuation_task_t<TFuture, TContinuation>;

            auto current = forward<TFuture>(future);
            auto state = current.ValidateState();
            auto allocator = state->Get_Allocator();

            task_t task(allocator_arg_t(), allocator, forward<TContinuation>(continuation));
            auto taskFuture = GetUnwrappedFuture(task);

            state->SetCallback(CreateCallback(std::move(task), std::move(current)));

            return taskFuture;
        }

        template<class R>
        typename BasicFuture<R>::SharedState BasicFuture<R>::GetState()
        {
            return _state;
        }

        template<class R>
        template<class TResult>
        decltype(auto) BasicFuture<R>::GetUnwrappedFuture(const CommonPromise<TResult>& promise)
        {
            return Unwrap(FutureFactory::Create(promise));
        }

        template<class R>
        template<typename T, template<typename> class TOuter, template<typename> class TInner>
        enable_if_future_t<TInner<T>> BasicFuture<R>::Unwrap(TOuter<TInner<T>>&& future)
        {
            return TInner<T>(std::forward<TOuter<TInner<T>>>(future));
        }

        template<class R>
        template<typename T, template<typename> class TOuter>
        enable_if_not_future_t<T, TOuter<T>> BasicFuture<R>::Unwrap(TOuter<T>&& future)
        {
            return std::forward<TOuter<T>>(future);
        }

        template<class R>
        template<class TFunctor, class TResultType, class TArgType>
        static decltype(auto) BasicFuture<R>::CreateCallback(
            BasicTask<TFunctor, TResultType, TArgType&>&& task,
            TArgType&& argument)
        {
            using namespace std;
            using task_t = BasicTask<TFunctor, TResultType, TArgType&>;

            return
                [
                    task = forward<task_t>(task),
                    argument = forward<TArgType>(argument)
                ]() mutable { task(argument); };
        }

        template<class R>
        template<class TFunctor, class TResultType, class TArgType>
        static decltype(auto) BasicFuture<R>::CreateCallback(
            BasicTask<TFunctor, TResultType, TArgType>&& task,
            TArgType&& argument)
        {
            using namespace std;
            using task_t = BasicTask<TFunctor, TResultType, TArgType>;

            return
                [
                    task = forward<task_t>(task),
                    argument = forward<TArgType>(argument)
                ]() mutable { task(move(argument)); };
        }

        template<class R>
        template<class TFunctor, class TResultType, class TArgType>
        static decltype(auto) BasicFuture<R>::CreateCallback(
            BasicTask<TFunctor, TResultType, TArgType&&>&& task,
            TArgType&& argument)
        {
            using namespace std;
            using task_t = BasicTask<TFunctor, TResultType, TArgType&&>;

            return
                [
                    task = forward<task_t>(task),
                    argument = forward<TArgType>(argument)
                ]() mutable { task(move(argument)); };
        }

        template<class R>
        void BasicFuture<R>::ResetState()
        {
            _state.reset();
        }
    }
}