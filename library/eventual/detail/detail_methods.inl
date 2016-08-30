#pragma once

#include "detail_methods.h"

#if defined(EVENTUAL_HEADER_ONLY)
#include "detail_methods.cpp"
#endif

namespace eventual
{
    namespace detail
    {
        template<class Future, class... Futures>
        decltype(auto) When_All_(Future&& head, Futures&&... others)
        {
            using namespace std;

            auto tailfuture = When_All_(forward<Futures>(others)...);
            return tailfuture.then([head = move(head)](auto& tf) mutable
            {
                return head.then([tf = move(tf)](auto& h) mutable
                {
                    return tuple_cat(make_tuple(move(h)), tf.get());
                });
            });
        }
    }
}
