#pragma once

#include <vector>
#include <iterator>
#include <type_traits>
#include <tuple>

#include "../eventual_export.h"

namespace eventual
{
    template<class TResult> class future;
    template<class Sequence> struct when_any_result;
    
    namespace detail
    {
        class memory_resource;
        
        template<class InputIterator>
        using all_futures_vector = future<std::vector<typename std::iterator_traits<InputIterator>::value_type>>;

        template<class... Futures>
        using all_futures_tuple = future<std::tuple<std::decay_t<Futures>...>>;

        template<class InputIterator>
        using any_future_result_vector = future<when_any_result<std::vector<typename std::iterator_traits<InputIterator>::value_type>>>;

        template<class... Futures>
        using any_futures_result_tuple = future<when_any_result<std::tuple<std::decay_t<Futures>...>>>;
        
        EVENTUAL_API memory_resource* get_default_resource() noexcept;

        template<class Future, class... Futures>
        static decltype(auto) When_All_(Future&& head, Futures&&... others);

        static EVENTUAL_API future<std::tuple<>> When_All_();
    }
}
