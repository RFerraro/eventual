#pragma once

#include "eventual_methods.h"
#include "future.h"
#include "promise.h"

namespace eventual
{
    future<void> make_ready_future()
    {
        promise<void> promise;
        promise.set_value();
        return promise.get_future();
    }

    future<when_any_result<std::tuple<>>> when_any()
    {
        return make_ready_future(when_any_result<std::tuple<>>());
    }
}
