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
        template <class T> class State;
        template <class T> class StateNotificationShim;

        template<class TFunctor, class R, class... ArgTypes> class BasicTask;

        template<typename TState1, typename TState2> class CompositeState;

        struct Unit { };
        
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

        template<class T, class Alloc, class U = T>
        using enable_if_uses_allocator_t = std::enable_if_t<std::uses_allocator<T, Alloc>::value, U>;

        template<class T, class Alloc, class U = T>
        using enable_if_doesnt_use_allocator_t = std::enable_if_t<!std::uses_allocator<T, Alloc>::value, U>;

        template<class T>
        struct unit_from_type
        {
            typedef T type;
        };

        template<class T>
        struct unit_from_type<T&>
        {
            typedef std::reference_wrapper<T> type;
        };

        template<>
        struct unit_from_type<void>
        {
            typedef Unit type;
        };

        template<class T>
        struct type_from_unit
        {
            typedef T type;
        };

        template<class T>
        struct type_from_unit<std::reference_wrapper<T>>
        {
            typedef T& type;
        };

        template<>
        struct type_from_unit<Unit>
        {
            typedef void type;
        };

        template<class T>
        using unit_from_type_t = typename unit_from_type<T>::type;

        template<class T>
        using type_from_unit_t = typename type_from_unit<T>::type;

        template<class>
        struct get_future_unit;

        template<template<typename> class TFuture, class TUnit>
        struct get_future_unit<TFuture<TUnit>>
        {
            typedef unit_from_type_t<TUnit> type;
        };

        template<class T, typename = void>
        struct get_state
        {
            typedef State<unit_from_type_t<T>> state_type;
            typedef std::shared_ptr<state_type> shared_type;
        };

        template<template<typename> class TFuture, class T>
        struct get_state<TFuture<T>, std::enable_if_t<is_future<TFuture<T>>::value>>
        {
            typedef State<TFuture<T>> parent_state_type;
            typedef typename get_state<T>::state_type child_state_type;

            typedef CompositeState<parent_state_type, child_state_type> state_type;
            typedef std::shared_ptr<state_type> shared_type;
        };

        template<class T>
        using get_state_t = typename get_state<T>::state_type;

        template<class T>
        using get_composite_t = typename get_state<T>::composite_type;

        template<class T>
        using get_shared_state_t = typename get_state<T>::shared_type;

        template<class T, class U, class TResult>
        using enable_if_size_is_greater_than_t = typename std::enable_if<(sizeof(T) > sizeof(U)), TResult>::type;

        template<class T, class U, class TResult>
        using enable_if_size_is_less_than_or_eq_t = typename std::enable_if<(sizeof(T) <= sizeof(U)), TResult>::type;

        template<class TFuture, class TContinuation>
        struct get_continuation_task
        {
            typedef std::result_of_t<std::decay_t<TContinuation>(std::decay_t<TFuture>)> result_type;
            typedef BasicTask<TContinuation, result_type, TFuture> task_type;
        };

        template<class TFuture, class TContinuation>
        using get_continuation_task_t = typename get_continuation_task<TFuture, TContinuation>::task_type;
    }
}
