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

#include <utility>
#include <type_traits>

#include "detail/implementation.h"

namespace eventual
{
    template <class R> class Future;
    template <class R> class Shared_Future;
    template <class> class Packaged_Task; // undefined
    template <class R, class... ArgTypes>
    class Packaged_Task<R(ArgTypes...)>;

    template<class Sequence>
    struct When_Any_Result;

    using future_errc = std::future_errc;
    using future_error = std::future_error;
    using future_status = std::future_status;

    template<class Sequence>
    struct When_Any_Result
    {
        size_t index;
        Sequence futures;
    };

    template<class RType>
    class Future : public detail::BasicFuture<RType>
    {
        using Base = detail::BasicFuture<RType>;
        friend class detail::FutureHelper;

    public:
        Future() noexcept = default;
        Future(Future&& other) noexcept = default;
        Future(const Future& other) = delete;
        Future(Future<Future>&& other) noexcept : Base(std::forward<Future<Future>>(other)) { }

        Future& operator=(const Future& other) = delete;
        Future& operator=(Future&& other) noexcept = default;

        template<class F>
        decltype(auto) Then(F&& continuation)
        {
            return Base::ThenImpl(detail::decay_copy(std::forward<F>(continuation)), std::move(*this));
        }

        decltype(auto) Get()
        {
            return Base::GetResult(*this);
        }
    };

    template<class RType>
    class Future<RType&> : public detail::BasicFuture<RType&>
    {
        using Base = detail::BasicFuture<RType&>;
        friend class detail::FutureHelper;

    public:
        Future() noexcept = default;
        Future(Future&& other) noexcept = default;
        Future(const Future& other) = delete;
        Future(Future<Future>&& other) noexcept : Base(std::forward<Future<Future>>(other)) { }

        Future& operator=(const Future& other) = delete;
        Future& operator=(Future&& other) noexcept = default;

        template<class F>
        decltype(auto) Then(F&& continuation)
        {
            return Base::ThenImpl(detail::decay_copy(std::forward<F>(continuation)), std::move(*this));
        }

        decltype(auto) Get()
        {
            return Base::GetResult(*this);
        }
    };

    template<>
    class Future<void> : public detail::BasicFuture<void>
    {
        using Base = detail::BasicFuture<void>;
        friend class detail::FutureHelper;

    public:
        Future() noexcept = default;
        Future(Future&& other) noexcept = default;
        Future(const Future& other) = delete;
        Future(Future<Future>&& other) noexcept : Base(std::forward<Future<Future>>(other)) { }

        Future& operator=(const Future& other) = delete;
        Future& operator=(Future&& other) noexcept = default;

        template<class F>
        decltype(auto) Then(F&& continuation)
        {
            return Base::ThenImpl(detail::decay_copy(std::forward<F>(continuation)), std::move(*this));
        }

        decltype(auto) Get()
        {
            return Base::GetResult(*this);
        }
    };

    template<class R>
    class Promise : public detail::BasicPromise<R>
    {
        using Base = detail::BasicPromise<R>;
        friend class detail::FutureHelper;

    public:
        Promise() = default;
        Promise(Promise&& other) noexcept = default;
        Promise(const Promise& other) = delete;

        template<class Alloc>
        Promise(std::allocator_arg_t arg, const Alloc& alloc) : Base(arg, alloc) { }

        Promise& operator=(const Promise& rhs) = delete;
        Promise& operator=(Promise&& other) noexcept = default;

        void Swap(Promise& other) noexcept { Base::Swap(other); }

        void Set_Value(const R& value) { Base::SetValue(value); }
        void Set_Value(R&& value) { Base::SetValue(std::forward<R>(value)); }
        void Set_Value_At_Thread_Exit(const R& value) { Base::SetValueAtThreadExit(value); }
        void Set_Value_At_Thread_Exit(R&& value) { Base::SetValueAtThreadExit(std::forward<R>(value)); }
        void Set_Exception(std::exception_ptr exceptionPtr) { Base::SetException(exceptionPtr); }
        void Set_Exception_At_Thread_Exit(std::exception_ptr exceptionPtr) { Base::SetExceptionAtThreadExit(exceptionPtr); }
    };

    template<class R>
    class Promise<R&> : public detail::BasicPromise<R&>
    {
        using Base = detail::BasicPromise<R&>;
        friend class detail::FutureHelper;

    public:
        Promise() = default;
        Promise(Promise&& other) noexcept = default;
        Promise(const Promise& other) = delete;

        template<class Alloc>
        Promise(std::allocator_arg_t arg, const Alloc& alloc) : Base(arg, alloc) { }

        Promise& operator=(const Promise& rhs) = delete;
        Promise& operator=(Promise&& other) noexcept = default;

        void Swap(Promise& other) noexcept { Base::Swap(other); }

        void Set_Value(R& value) { Base::SetValue(value); }
        void Set_Value_At_Thread_Exit(R& value) { Base::SetValueAtThreadExit(value); }
        void Set_Exception(std::exception_ptr exceptionPtr) { Base::SetException(exceptionPtr); }
        void Set_Exception_At_Thread_Exit(std::exception_ptr exceptionPtr) { Base::SetExceptionAtThreadExit(exceptionPtr); }
    };

    template<>
    class Promise<void> : public detail::BasicPromise<void>
    {
        using Base = detail::BasicPromise<void>;
        friend class detail::FutureHelper;

    public:
        Promise() = default;
        Promise(Promise&& other) noexcept = default;
        Promise(const Promise& other) = delete;

        template<class Alloc>
        Promise(std::allocator_arg_t arg, const Alloc& alloc) : Base(arg, alloc) { }

        Promise& operator=(const Promise& rhs) = delete;
        Promise& operator=(Promise&& other) noexcept = default;

        void Swap(Promise& other) noexcept { Base::Swap(other); }

        void Set_Value() { Base::SetValue(); }
        void Set_Value_At_Thread_Exit() { Base::SetValueAtThreadExit(); }
        void Set_Exception(std::exception_ptr exceptionPtr) { Base::SetException(exceptionPtr); }
        void Set_Exception_At_Thread_Exit(std::exception_ptr exceptionPtr) { Base::SetExceptionAtThreadExit(exceptionPtr); }
    };

    template<class R>
    class Shared_Future : public detail::BasicFuture<R>
    {
        using Base = detail::BasicFuture<R>;
        friend class detail::FutureHelper;

    public:

        Shared_Future() noexcept = default;
        Shared_Future(const Shared_Future& other) = default;
        Shared_Future(Future<R>&& other) noexcept : Base(std::forward<Future<R>>(other)) { }
        Shared_Future(Shared_Future&& other) noexcept = default;
        Shared_Future(Future<Shared_Future>&& other) noexcept : Base(std::forward<Future<Shared_Future>>(other)) { }

        Shared_Future& operator=(const Shared_Future& other) = default;
        Shared_Future& operator=(Shared_Future&& other) noexcept = default;

        Shared_Future Share() = delete;

        template<class F>
        decltype(auto) Then(F&& continuation)
        {
            return Base::ThenImpl(detail::decay_copy(std::forward<F>(continuation)), Shared_Future(*this));
        }

        decltype(auto) Get()
        {
            return Base::GetSharedResult(*this);
        }
    };

    template<class R>
    class Shared_Future<R&> : public detail::BasicFuture<R&>
    {
        using Base = detail::BasicFuture<R&>;
        friend class detail::FutureHelper;

    public:

        Shared_Future() noexcept = default;
        Shared_Future(const Shared_Future& other) = default;
        Shared_Future(Future<R&>&& other) noexcept : Base(std::forward<Future<R&>>(other)) { }
        Shared_Future(Shared_Future&& other) noexcept = default;
        Shared_Future(Future<Shared_Future>&& other) noexcept : Base(std::forward<Future<Shared_Future>>(other)) { }

        Shared_Future& operator=(const Shared_Future& other) = default;
        Shared_Future& operator=(Shared_Future&& other) noexcept = default;

        Shared_Future Share() = delete;

        template<class F>
        decltype(auto) Then(F&& continuation)
        {
            return Base::ThenImpl(detail::decay_copy(std::forward<F>(continuation)), Shared_Future(*this));
        }

        decltype(auto) Get()
        {
            return Base::GetSharedResult(*this);
        }
    };

    template<>
    class Shared_Future<void> : public detail::BasicFuture<void>
    {
        using Base = detail::BasicFuture<void>;
        friend class detail::FutureHelper;

    public:

        Shared_Future() noexcept = default;
        Shared_Future(const Shared_Future& other) = default;
        Shared_Future(Future<void>&& other) noexcept : Base(std::forward<Future<void>>(other)) { }
        Shared_Future(Shared_Future&& other) noexcept = default;

        Shared_Future(Future<Shared_Future>&& other) noexcept : Base(std::forward<Future<Shared_Future>>(other)) { }

        Shared_Future& operator=(const Shared_Future& other) = default;
        Shared_Future& operator=(Shared_Future&& other) noexcept = default;

        Shared_Future Share() = delete;

        template<class F>
        decltype(auto) Then(F&& continuation)
        {
            return Base::ThenImpl(detail::decay_copy(std::forward<F>(continuation)), Shared_Future(*this));
        }

        decltype(auto) Get()
        {
            return Base::GetSharedResult(*this);
        }
    };

    template<class R, class... ArgTypes>
    class Packaged_Task<R(ArgTypes...)> : public detail::BasicTask<R, ArgTypes...>
    {
        using Base = detail::BasicTask<R, ArgTypes...>;

    public:
        Packaged_Task() noexcept = default;
        Packaged_Task(const Packaged_Task&) = delete;
        Packaged_Task(Packaged_Task&& other) noexcept = default;

        template<class F, class = detail::enable_if_not_same<Packaged_Task, F>>
        explicit Packaged_Task(F&& task)
            : Base(std::forward<F>(task))
        { }

        template<class F, class Allocator, class = detail::enable_if_not_same<Packaged_Task, F>>
        Packaged_Task(std::allocator_arg_t arg, const Allocator& alloc, F&& task)
            : Base(arg, alloc, std::forward<F>(task))
        { }

        Packaged_Task& operator=(const Packaged_Task&) = delete;
        Packaged_Task& operator=(Packaged_Task&&) noexcept = default;

        void Swap(Packaged_Task& other) { Base::Swap(other); }
    };

    template<class T>
    Future<detail::decay_future_result_t<T>>
        Make_Ready_Future(T&& value)
    {
        using result_t = detail::decay_future_result_t<T>;
        Promise<result_t> promise;
        promise.Set_Value(std::forward<result_t>(value));
        return promise.Get_Future();
    }

    inline Future<void> Make_Ready_Future()
    {
        Promise<void> promise;
        promise.Set_Value();
        return promise.Get_Future();
    }

    template<class T>
    Future<T> Make_Exceptional_Future(std::exception_ptr ex)
    {
        Promise<T> promise;
        promise.Set_Exception(ex);
        return promise.Get_Future();
    }

    template<class T, class E>
    Future<T> Make_Exceptional_Future(E ex)
    {
        Promise<T> promise;
        promise.Set_Exception(std::make_exception_ptr(ex));
        return promise.Get_Future();
    }

    template<class InputIterator>
    detail::all_futures_vector<InputIterator>
        When_All(InputIterator first, InputIterator last)
    {
        return detail::FutureHelper::When_All(first, last);
    }

    template<class... Futures>
    detail::all_futures_tuple<Futures...>
        When_All(Futures&&... futures)
    {
        return detail::FutureHelper::When_All(std::forward<Futures>(futures)...);
    }

    template<class InputIterator>
    detail::any_future_result_vector<InputIterator>
        When_Any(InputIterator first, InputIterator last)
    {
        return detail::FutureHelper::When_Any(first, last);
    }

    template<class... Futures>
    detail::any_futures_result_tuple<Futures...>
        When_Any(Futures&&... futures)
    {
        return detail::FutureHelper::When_Any(std::forward<Futures>(futures)...);
    }
}

namespace std
{
    template<class RType, class Alloc>
    struct uses_allocator<eventual::Promise<RType>, Alloc> : true_type { };

    template<class RType, class Alloc>
    struct uses_allocator<eventual::Packaged_Task<RType>, Alloc> : true_type { };

    template<class Function, class... Args>
    void swap(eventual::Packaged_Task<Function(Args...)>& lhs, eventual::Packaged_Task<Function(Args...)>& rhs) noexcept
    {
        lhs.Swap(rhs);
    }

    template<class RType>
    void swap(eventual::Promise<RType> &lhs, eventual::Promise<RType> &rhs) noexcept
    {
        lhs.Swap(rhs);
    }
}

namespace eventual
{
    namespace detail
    {
        decltype(auto) FutureHelper::When_All()
        {
            return Make_Ready_Future(std::tuple<>());
        }

        Future<When_Any_Result<std::tuple<>>> FutureHelper::When_Any()
        {
            using result_t = When_Any_Result<std::tuple<>>;

            auto result = result_t();
            result.index = static_cast<size_t>(-1);

            return Make_Ready_Future<result_t>(std::move(result));
        }

        template<class TFuture>
        TFuture FutureHelper::CreateFuture(const future_state<TFuture>& state)
        {
            auto future = TFuture();
            future._state = state;
            return future;
        }

        template<class R>
        Shared_Future<R> BasicFuture<R>::Share()
        {
            auto state = ValidateState();
            _state.reset();

            return FutureHelper::CreateFuture<Shared_Future<R>>(state);
        }

        template<class R>
        Future<R> BasicPromise<R>::Get_Future()
        {
            auto state = ValidateState();

            if (!state->SetRetrieved())
                throw CreateFutureError(future_errc::future_already_retrieved);

            return FutureHelper::CreateFuture<Future<R>>(state);
        }

        template<class InputIterator>
        decltype(auto) FutureHelper::When_Any(enable_if_iterator_t<InputIterator> begin, InputIterator end)
        {
            using namespace std;
            using future_t = typename iterator_traits<InputIterator>::value_type;
            using result_t = When_Any_Result<vector<future_t>>;

            const auto noIdx = static_cast<size_t>(-1);
            auto indexPromise = make_shared<Promise<size_t>>();
            auto indexFuture = indexPromise->Get_Future();
            vector<future_t> futures;

            size_t index = 0;
            for (auto iterator = begin; iterator != end; ++iterator)
            {
                futures.emplace_back(move(*iterator));

                auto state = futures.back()._state;
                state->SetCallback([indexPromise, index]()
                {
                    indexPromise->TrySetValue(index);
                });

                index++;
            }

            if (futures.empty())
                indexPromise->SetValue(noIdx);

            return indexFuture.Then(
                [futures = std::move(futures)](auto f) mutable
            {
                result_t result;
                result.index = f.Get();
                result.futures = move(futures);

                return result;
            });
        }

        template<class... Futures>
        decltype(auto) FutureHelper::When_Any(Futures&&... futures)
        {
            using namespace std;
            using result_t = When_Any_Result<tuple<decay_t<Futures>...>>;
            auto futuresSequence = make_tuple<std::decay_t<Futures>...>(std::forward<Futures>(futures)...);
            auto indexPromise = make_shared<Promise<size_t>>();
            auto indexFuture = indexPromise->Get_Future();

            size_t index = 0;
            detail::for_each(futuresSequence, [indexPromise, &index](auto& future) mutable
            {
                auto state = future._state;
                state->SetCallback([indexPromise, index]()
                {
                    indexPromise->TrySetValue(index);
                });

                index++;
            });

            return indexFuture.Then(
                [t = move(futuresSequence)](auto f) mutable
            {
                result_t result;
                result.index = f.Get();
                result.futures = move(t);

                return move(result);
            });
        }
    }
}
