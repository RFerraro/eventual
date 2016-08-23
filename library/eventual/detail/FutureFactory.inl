#pragma once

#include "FutureFactory.h"

#include "future.h"

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
