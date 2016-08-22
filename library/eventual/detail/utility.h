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
#include <tuple>
#include <future>
#include <exception>

#include "traits.h"

namespace eventual
{
    namespace detail
    {
        template<class T>
        decltype(auto) decay_copy(T&& v)
        {
            return std::forward<T>(v);
        }

        template<std::size_t I = 0, typename TTuple, typename Func>
        std::enable_if_t<I != std::tuple_size<TTuple>::value>
            for_each(TTuple& tuple, Func&& func)
        {
            func(std::get<I>(tuple));
            for_each<I + 1>(tuple, std::forward<Func>(func));
        }

        template<std::size_t I = 0, typename TTuple, typename Func>
        std::enable_if_t<I == std::tuple_size<TTuple>::value>
            for_each(TTuple&, Func&&)
        {
            //do nothing
        }

        inline std::future_error CreateFutureError(std::future_errc error)
        {
            auto code = std::make_error_code(error);
            return std::future_error(code);
        }

        inline std::exception_ptr CreateFutureExceptionPtr(std::future_errc error)
        {
            std::exception_ptr ptr;
            try
            {
                throw CreateFutureError(error);
            }
            catch (const std::future_error&)
            {
                ptr = std::current_exception();
            }
            return ptr;
        }

        template<class TFunctor, class TCallable, class Allocator>
        static detail::enable_if_uses_allocator_t<TFunctor, Allocator>
            CreateFunctor(const Allocator& alloc, TCallable&& function)
        {
            //todo: C++ 17 removes erased allocators for std::function...
            return TFunctor(std::allocator_arg_t(), alloc, std::forward<TCallable>(function));
        }

        template<class TFunctor, class TCallable, class Allocator>
        static detail::enable_if_doesnt_use_allocator_t<TFunctor, Allocator>
            CreateFunctor(const Allocator&, TCallable&& function)
        {
            //todo: C++ 17 removes erased allocators for std::function...
            // because libstdc++'s std::function doesn't support custom allocation...
            return TFunctor(std::forward<TCallable>(function));
        }
    }
}