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

#include <type_traits>
#include <functional>

namespace eventual
{
    template <class R> class Future;
    template <class R> class Shared_Future;
    
    namespace detail
    {
        template<class R>
        struct is_future : std::false_type { };

        template<class R>
        struct is_future<Future<R>> : std::true_type { };

        template<class R>
        struct is_future<Shared_Future<R>> : std::true_type { };

        template<class T>
        using enable_if_future_t = std::enable_if_t<is_future<T>::value, T>;

        template<class T, class TResult>
        using enable_if_not_future_t = std::enable_if_t<!is_future<T>::value, TResult>;

        template<class R>
        struct decay_future_result
        {
            typedef std::decay_t<R> type;
        };

        template<class R>
        struct decay_future_result<std::reference_wrapper<R>>
        {
            typedef R& type;
        };

        template<class R>
        using decay_future_result_t = typename decay_future_result<R>::type;

        template<class...>
        struct has_parameter
        {
            typedef void type;
        };

        template<class, class = void>
        struct is_iterator : std::false_type { };

        template<class T>
        struct is_iterator<T, typename has_parameter<
            typename T::iterator_category,
            typename T::value_type,
            typename T::difference_type,
            typename T::pointer,
            typename T::reference>::type>
            : std::true_type
        { };

        template<class T>
        using enable_if_iterator_t = std::enable_if_t<is_iterator<T>::value, T>;

        template<class T, class U>
        using enable_if_not_same = std::enable_if<!std::is_same<std::decay_t<T>, std::decay_t<U>>::value>;
    }
}