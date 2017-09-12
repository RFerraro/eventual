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
    using future_errc = std::future_errc;
    using future_error = std::future_error;
    using future_status = std::future_status;

    template<class R>
    class shared_future;

    template<class R>
    class promise;

    template <class> 
    class packaged_task; // undefined

    template<class Sequence>
    struct when_any_result
    {
        size_t index = size_t(-1);
        Sequence futures;
    };

    template<class RType>
    class future : public detail::BasicFuture<RType>
    {
        using Base = detail::BasicFuture<RType>;
        using SharedState = typename Base::SharedState;
        
        friend class detail::FutureFactory;

    public:
        future() noexcept = default;
        future(future&& other) noexcept = default;
        future(const future& other) = delete;
        future(future<future>&& other) noexcept : Base(std::forward<future<future>>(other)) { }

        future& operator=(const future& other) = delete;
        future& operator=(future&& other) noexcept = default;

        template<class F>
        decltype(auto) then(F&& continuation)
        {
            return Base::ThenMove(std::forward<F>(continuation), *this);
        }

        RType get()
        {
            return Base::GetResult();
        }

        shared_future<RType> share();

    private:

        template<class TPromise>
        future(const TPromise& promise, detail::enable_if_not_same_t<future, TPromise, int> = 0)
            : Base(promise) { }
    };

    template<class RType>
    class future<RType&> : public detail::BasicFuture<RType&>
    {
        using Base = detail::BasicFuture<RType&>;
        using SharedState = typename Base::SharedState;
        
        friend class detail::FutureFactory;

    public:
        future() noexcept = default;
        future(future&& other) noexcept = default;
        future(const future& other) = delete;
        future(future<future>&& other) noexcept : Base(std::forward<future<future>>(other)) { }

        future& operator=(const future& other) = delete;
        future& operator=(future&& other) noexcept = default;

        template<class F>
        decltype(auto) then(F&& continuation)
        {
            return Base::ThenMove(std::forward<F>(continuation), *this);
        }

        RType& get()
        {
            return Base::GetResult();
        }

        shared_future<RType&> share();

    private:

        template<class TPromise>
        future(const TPromise& promise, detail::enable_if_not_same_t<future, TPromise, int> = 0)
            : Base(promise) { }
    };

    template<>
    class future<void> : public detail::BasicFuture<void>
    {
        using Base = detail::BasicFuture<void>;
        using SharedState = typename Base::SharedState;
        
        friend class detail::FutureFactory;

    public:
        future() noexcept = default;
        future(future&& other) noexcept = default;
        future(const future& other) = delete;
        future(future<future>&& other) noexcept : Base(std::forward<future<future>>(other)) { }

        future& operator=(const future& other) = delete;
        future& operator=(future&& other) noexcept = default;

        template<class F>
        decltype(auto) then(F&& continuation)
        {
            return Base::ThenMove(std::forward<F>(continuation), *this);
        }

        void get()
        {
            Base::GetResult();
        }

        shared_future<void> share();

    private:

        template<class TPromise>
        future(const TPromise& promise, detail::enable_if_not_same_t<future, TPromise, int> = 0)
            : Base(promise) { }
    };

    template<class R>
    class promise : public detail::BasicPromise<R>
    {
        using Base = detail::BasicPromise<R>;
        using Factory = detail::FutureFactory;

    public:
        promise() = default;
        promise(promise&& other) noexcept = default;
        promise(const promise& other) = delete;

        template<class Alloc>
        promise(std::allocator_arg_t arg, const Alloc& alloc) : Base(arg, alloc) { }

        promise& operator=(const promise& rhs) = delete;
        promise& operator=(promise&& other) noexcept = default;

        void swap(promise& other) noexcept { Base::swap(other); }

        void set_value(const R& value) { Base::SetValue(value); }
        void set_value(R&& value) { Base::SetValue(std::forward<R>(value)); }
        void set_value_at_thread_exit(const R& value) { Base::SetValueAtThreadExit(value); }
        void set_value_at_thread_exit(R&& value) { Base::SetValueAtThreadExit(std::forward<R>(value)); }
    };

    template<class R>
    class promise<R&> : public detail::BasicPromise<R&>
    {
        using Base = detail::BasicPromise<R&>;
        using Factory = detail::FutureFactory;

    public:
        promise() = default;
        promise(promise&& other) noexcept = default;
        promise(const promise& other) = delete;

        template<class Alloc>
        promise(std::allocator_arg_t arg, const Alloc& alloc) : Base(arg, alloc) { }

        promise& operator=(const promise& rhs) = delete;
        promise& operator=(promise&& other) noexcept = default;

        void swap(promise& other) noexcept { Base::swap(other); }

        void set_value(R& value) { Base::SetValue(value); }
        void set_value_at_thread_exit(R& value) { Base::SetValueAtThreadExit(value); }
    };

    template<>
    class promise<void> : public detail::BasicPromise<void>
    {
        using Base = detail::BasicPromise<void>;
        using Factory = detail::FutureFactory;

    public:
        promise() = default;
        promise(promise&& other) noexcept = default;
        promise(const promise& other) = delete;

        template<class Alloc>
        promise(std::allocator_arg_t arg, const Alloc& alloc) : Base(arg, alloc) { }

        promise& operator=(const promise& rhs) = delete;
        promise& operator=(promise&& other) noexcept = default;

        void swap(promise& other) noexcept { Base::swap(other); }

        void set_value() { Base::SetValue(detail::Unit()); }
        void set_value_at_thread_exit() { Base::SetValueAtThreadExit(detail::Unit()); }
    };

    template<class R>
    class shared_future : public detail::BasicFuture<R>
    {
        using Base = detail::BasicFuture<R>;
        using SharedState = typename Base::SharedState;

    public:

        shared_future() noexcept = default;
        shared_future(const shared_future& other) = default;
        shared_future(future<R>&& other) noexcept : Base(std::forward<future<R>>(other)) { }
        shared_future(shared_future&& other) noexcept = default;
        shared_future(future<shared_future>&& other) noexcept : Base(std::forward<future<shared_future>>(other)) { }

        shared_future& operator=(const shared_future& other) = default;
        shared_future& operator=(shared_future&& other) noexcept = default;

        template<class F>
        decltype(auto) then(F&& continuation)
        {
            return Base::ThenShare(std::forward<F>(continuation), *this);
        }

        const R& get() const
        {
            return Base::GetResult();
        }
    };

    template<class R>
    class shared_future<R&> : public detail::BasicFuture<R&>
    {
        using Base = detail::BasicFuture<R&>;
        using SharedState = typename Base::SharedState;

    public:

        shared_future() noexcept = default;
        shared_future(const shared_future& other) = default;
        shared_future(future<R&>&& other) noexcept : Base(std::forward<future<R&>>(other)) { }
        shared_future(shared_future&& other) noexcept = default;
        shared_future(future<shared_future>&& other) noexcept : Base(std::forward<future<shared_future>>(other)) { }

        shared_future& operator=(const shared_future& other) = default;
        shared_future& operator=(shared_future&& other) noexcept = default;

        template<class F>
        decltype(auto) then(F&& continuation)
        {
            return Base::ThenShare(std::forward<F>(continuation), *this);
        }

        R& get() const
        {
            return Base::GetResult();
        }
    };

    template<>
    class shared_future<void> : public detail::BasicFuture<void>
    {
        using Base = detail::BasicFuture<void>;
        using SharedState = typename Base::SharedState;

    public:

        shared_future() noexcept = default;
        shared_future(const shared_future& other) = default;
        shared_future(future<void>&& other) noexcept : Base(std::forward<future<void>>(other)) { }
        shared_future(shared_future&& other) noexcept = default;

        shared_future(future<shared_future>&& other) noexcept : Base(std::forward<future<shared_future>>(other)) { }

        shared_future& operator=(const shared_future& other) = default;
        shared_future& operator=(shared_future&& other) noexcept = default;

        template<class F>
        decltype(auto) then(F&& continuation)
        {
            return Base::ThenShare(std::forward<F>(continuation), *this);
        }

        void get() const
        {
            Base::GetResult();
        }
    };

    template<class R, class... ArgTypes>
    class packaged_task<R(ArgTypes...)> : public detail::BasicTask<std::function<R(ArgTypes...)>, R, ArgTypes...>
    {
        using Base = detail::BasicTask<std::function<R(ArgTypes...)>, R, ArgTypes...>;
        using Factory = detail::FutureFactory;

    public:
        packaged_task() noexcept = default;
        packaged_task(const packaged_task&) = delete;
        packaged_task(packaged_task&& other) noexcept = default;

        template<class F, class = detail::enable_if_not_same<packaged_task, F>>
        explicit packaged_task(F&& task)
            : Base(detail::decay_copy(std::forward<F>(task)))
        { }

        template<class F, class Allocator, class = detail::enable_if_not_same<packaged_task, F>>
        packaged_task(std::allocator_arg_t arg, const Allocator& alloc, F&& task)
            : Base(arg, alloc, detail::decay_copy(std::forward<F>(task)))
        { }

        packaged_task& operator=(const packaged_task&) = delete;
        packaged_task& operator=(packaged_task&&) noexcept = default;

        void swap(packaged_task& other) { Base::swap(other); }
    };

    template<class R>
    shared_future<R> future<R>::share()
    {
        return Base::Share(std::move(*this));
    }

    template<class R>
    shared_future<R&> future<R&>::share()
    {
        return Base::Share(std::move(*this));
    }

    inline shared_future<void> future<void>::share()
    {
        return Base::Share(std::move(*this));
    }

    template<class T>
    future<detail::decay_future_result_t<T>>
        make_ready_future(T&& value)
    {
        using result_t = detail::decay_future_result_t<T>;
        promise<result_t> promise;
        promise.set_value(std::forward<result_t>(value));
        return promise.get_future();
    }

    inline future<void> make_ready_future()
    {
        promise<void> promise;
        promise.set_value();
        return promise.get_future();
    }

    template<class T>
    future<T> make_exceptional_future(std::exception_ptr ex)
    {
        promise<T> promise;
        promise.set_exception(ex);
        return promise.get_future();
    }

    template<class T, class E>
    future<T> make_exceptional_future(E ex)
    {
        promise<T> promise;
        promise.set_exception(std::make_exception_ptr(ex));
        return promise.get_future();
    }

    template<class InputIterator>
    detail::all_futures_vector<InputIterator>
        when_all(InputIterator first, InputIterator last)
    {
        using namespace std;
        using future_vector_t = vector<typename iterator_traits<InputIterator>::value_type>;

        auto vectorfuture = make_ready_future(future_vector_t());

        for (auto iterator = first; iterator != last; ++iterator)
        {
            auto& future = *iterator;
            vectorfuture = vectorfuture.then(
                [future = std::move(future)](auto& vf) mutable
            {
                return future.then(
                    [vf = std::move(vf)](auto& future) mutable
                {
                    auto resultVector = vf.get();
                    resultVector.emplace_back(std::move(future));
                    return resultVector;
                });
            });
        }

        return vectorfuture;
    }

    template<class... Futures>
    detail::all_futures_tuple<Futures...>
        when_all(Futures&&... futures)
    {
        return detail::When_All_(std::forward<Futures>(futures)...);
    }

    template<class InputIterator>
    detail::any_future_result_vector<InputIterator>
        when_any(InputIterator first, InputIterator last)
    {
        using namespace std;
        using namespace eventual::detail;
        using future_t = typename iterator_traits<InputIterator>::value_type;
        using result_t = when_any_result<vector<future_t>>;

        constexpr auto noIdx = size_t(-1);
        auto indexPromise = make_shared<promise<size_t>>();
        auto indexfuture = indexPromise->get_future();
        vector<future_t> futures;

        size_t index = 0;
        for (auto iterator = first; iterator != last; ++iterator)
        {
            futures.emplace_back(std::move(*iterator));

            FutureHelper::SetCallback(futures.back(),
                        [indexPromise, index]()
            {
                FutureHelper::TrySetValue(*indexPromise, index);
            });

            index++;
        }

        if (futures.empty())
            indexPromise->set_value(noIdx);

        return indexfuture.then(
            [sequence = std::move(futures)](auto& firstIndex) mutable
        {
            result_t result;
            result.index = firstIndex.get();
            result.futures = std::move(sequence);

            return result;
        });
    }

    template<class... Futures>
    detail::any_futures_result_tuple<Futures...>
        when_any(Futures&&... futures)
    {
        using namespace std;
        using namespace eventual::detail;
        using result_t = when_any_result<tuple<decay_t<Futures>...>>;

        auto futuresSequence = make_tuple<decay_t<Futures>...>(std::forward<Futures>(futures)...);
        auto indexPromise = make_shared<promise<size_t>>();
        auto indexfuture = indexPromise->get_future();

        size_t index = 0;
        detail::for_each(futuresSequence, [indexPromise, &index](auto& future) mutable
        {
            FutureHelper::SetCallback(future,
                        [indexPromise, index]()
            {
                FutureHelper::TrySetValue(*indexPromise, index);
            });

            index++;
        });

        return indexfuture.then(
            [sequence = std::move(futuresSequence)](auto& firstIndex) mutable
        {
            result_t result;
            result.index = firstIndex.get();
            result.futures = std::move(sequence);

            return result;
        });
    }

    inline future<when_any_result<std::tuple<>>> when_any()
    {
        return make_ready_future(when_any_result<std::tuple<>>());
    }
}

namespace std
{
    template<class RType, class Alloc>
    struct uses_allocator<eventual::promise<RType>, Alloc> : true_type { };

    template<class RType, class Alloc>
    struct uses_allocator<eventual::packaged_task<RType>, Alloc> : true_type { };

    template<class Function, class... Args>
    void swap(eventual::packaged_task<Function(Args...)>& lhs, eventual::packaged_task<Function(Args...)>& rhs) noexcept
    {
        lhs.swap(rhs);
    }

    template<class RType>
    void swap(eventual::promise<RType> &lhs, eventual::promise<RType> &rhs) noexcept
    {
        lhs.swap(rhs);
    }
}

namespace eventual
{
    namespace detail
    {
        template<class TResult>
        future<TResult> FutureFactory::Create(const CommonPromise<TResult>& promise)
        {
            return future<TResult>(promise);
        }

        template<class R>
        shared_future<R> detail::BasicFuture<R>::Share(future<R>&& future)
        {
            future.CheckState();
            return shared_future<R>(std::forward<eventual::future<R>>(future));
        }

        template<class R>
        template<class TContinuation, class TFuture>
        decltype(auto) detail::BasicFuture<R>::ThenImpl(TContinuation&& continuation, TFuture&& future)
        {
            using namespace std;
            using task_t = get_continuation_task_t<TFuture, TContinuation>;

            auto current = std::forward<TFuture>(future);
            auto state = current.ValidateState();
            auto allocator = state->Get_Allocator();

            task_t task(allocator_arg_t(), allocator, std::forward<TContinuation>(continuation));
            auto taskFuture = GetUnwrappedFuture(task);

            state->SetCallback(CreateCallback(std::move(task), std::move(current)));

            return taskFuture;
        }

        template<class R>
        future<R> detail::CommonPromise<R>::get_future() { return FutureFactory::Create(*this); }
        
        template<class Future, class... Futures>
        decltype(auto) When_All_(Future&& head, Futures&&... others)
        {
            using namespace std;

            auto tailfuture = When_All_(std::forward<Futures>(others)...);
            return tailfuture.then([head = std::move(head)](auto& tf) mutable
            {
                return head.then([tf = std::move(tf)](auto& h) mutable
                {
                    return tuple_cat(make_tuple(std::move(h)), tf.get());
                });
            });
        }

        inline future<std::tuple<>> When_All_()
        {
            return make_ready_future(std::tuple<>());
        }
    }
}
