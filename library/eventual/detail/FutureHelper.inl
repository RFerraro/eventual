#pragma once

#include "FutureHelper.h"

#include "BasicFuture.inl"
#include "BasicPromise.inl"

namespace eventual
{
    namespace detail
    {
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
            return future.GetResult<TResult>();
        }
    }
}