#pragma once

#include "utility.h"

#include <utility>
#include <type_traits>
#include <tuple>

#include "traits.h"

#include "utility.cpp"

namespace eventual
{
    namespace detail
    {
        template<class T>
        void PlacementDestructor<T>::operator()(T* item) const
        {
            item->~T();
            (void)item; // suppresses C4100 in VS2015
        }

        template<class T>
        decltype(auto) decay_copy(T&& v)
        {
            return std::forward<T>(v);
        }

        template<std::size_t I, typename TTuple, typename Func>
        std::enable_if_t<I != std::tuple_size<TTuple>::value>
            for_each(TTuple& tuple, Func&& func)
        {
            func(std::get<I>(tuple));
            for_each<I + 1>(tuple, std::forward<Func>(func));
        }

        template<std::size_t I, typename TTuple, typename Func>
        std::enable_if_t<I == std::tuple_size<TTuple>::value>
            for_each(TTuple&, Func&&)
        {
            //do nothing
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
