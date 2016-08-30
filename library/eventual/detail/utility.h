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

#include <cstddef>
#include <type_traits>
#include <tuple>
#include <future>
#include <exception>

#include "../eventual_export.h"
#include "traits.h"

namespace eventual
{
    namespace detail
    {        
        template<class T>
        struct PlacementDestructor
        {
            void operator()(T* item) const;
        };
        
        template<class T>
        decltype(auto) decay_copy(T&& v);

        template<std::size_t I = 0, typename TTuple, typename Func>
        std::enable_if_t<I != std::tuple_size<TTuple>::value>
            for_each(TTuple& tuple, Func&& func);

        template<std::size_t I = 0, typename TTuple, typename Func>
        std::enable_if_t<I == std::tuple_size<TTuple>::value>
            for_each(TTuple&, Func&&);

        EVENTUAL_API std::future_error CreateFutureError(std::future_errc error);

        EVENTUAL_API std::exception_ptr CreateFutureExceptionPtr(std::future_errc error);

        template<class TFunctor, class TCallable, class Allocator>
        static typename detail::enable_if_uses_allocator_t<TFunctor, Allocator>
            CreateFunctor(const Allocator& alloc, TCallable&& function);

        template<class TFunctor, class TCallable, class Allocator>
        static typename detail::enable_if_doesnt_use_allocator_t<TFunctor, Allocator>
            CreateFunctor(const Allocator&, TCallable&& function);
    }
}
