#pragma once

#include "CommonPromise.h"
#include <memory>
#include <utility>

#include "../future.h"
#include "FutureFactory.h"
#include "State.h"
#include "CompositeState.h"

namespace eventual
{
    namespace detail
    {
        template<class R>
        CommonPromise<R>::CommonPromise()
            : _state(StateType::MakeState())
        { }

        template<class R>
        CommonPromise<R>::CommonPromise(SharedState state)
            : _state(state)
        { }

        template<class R>
        template<class Alloc>
        CommonPromise<R>::CommonPromise(std::allocator_arg_t, const Alloc& alloc) :
            _state(StateType::AllocState(alloc))
        { }

        template<class R>
        CommonPromise<R>::~CommonPromise() noexcept
        {
            auto state = _state;
            if (!state || state->HasResult())
                return;

            state->SetException(CreateFutureExceptionPtr(std::future_errc::broken_promise));
        }

        template<class R>
        void CommonPromise<R>::swap(CommonPromise& other) noexcept
        {
            _state.swap(other._state);
        }

        template<class R>
        future<R> CommonPromise<R>::get_future()
        {
            return FutureFactory::Create(*this);
        }

        template<class R>
        template<class TValue>
        void CommonPromise<R>::SetValue(TValue&& value)
        {
            auto state = ValidateState();

            if (!state->SetResult(std::forward<TValue>(value)))
                throw CreateFutureError(std::future_errc::promise_already_satisfied);
        }

        template<class R>
        template<class TValue>
        void CommonPromise<R>::SetValueAtThreadExit(TValue&& value)
        {
            auto state = ValidateState();

            if (!state->SetResultAtThreadExit(std::forward<TValue>(value)))
                throw CreateFutureError(std::future_errc::promise_already_satisfied);
        }

        template<class R>
        void CommonPromise<R>::SetException(std::exception_ptr exceptionPtr)
        {
            auto state = ValidateState();

            if (!state->SetException(exceptionPtr))
                throw CreateFutureError(std::future_errc::promise_already_satisfied);
        }

        template<class R>
        void CommonPromise<R>::SetExceptionAtThreadExit(std::exception_ptr exceptionPtr)
        {
            auto state = ValidateState();

            if (!state->SetExceptionAtThreadExit(exceptionPtr))
                throw CreateFutureError(std::future_errc::promise_already_satisfied);
        }

        template<class R>
        template<class TValue>
        bool CommonPromise<R>::TrySetValue(TValue&& value)
        {
            auto state = ValidateState();

            return state->SetResult(std::forward<TValue>(value));
        }

        template<class R>
        void CommonPromise<R>::Reset()
        {
            auto state = ValidateState();

            const auto& allocator = state->Get_Allocator();
            auto newState = StateType::AllocState(allocator);

            std::swap(_state, newState);
        }

        template<class R>
        bool CommonPromise<R>::Valid() const noexcept
        {
            auto state = _state;
            return state ? true : false;
        }

        template<class R>
        typename CommonPromise<R>::SharedState CommonPromise<R>::ValidateState() const
        {
            auto state = _state;
            if (!state)
                throw CreateFutureError(std::future_errc::no_state);

            return state;
        }

        template<class R>
        typename CommonPromise<R>::SharedState CommonPromise<R>::CopyState() const
        {
            auto state = ValidateState();

            if (!state->SetRetrieved())
                throw CreateFutureError(std::future_errc::future_already_retrieved);

            return state;
        }
    }
}
