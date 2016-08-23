#pragma once

#include "FutureFactory.h"

#include "future.inl"

namespace eventual
{
    namespace detail
    {
        template<class TResult>
        future<TResult> FutureFactory::Create(const CommonPromise<TResult>& promise)
        {
            return future<TResult>(promise);
        }
    }
}
