#pragma once

#include <utility>
#include <chrono>

#include "traits.h"
#include "State.h"
#include "CompositeState.h"

#include <future>

namespace eventual
{
    template<class TResult> class future;
    template<class TResult> class shared_future;

    namespace detail
    {
        template<class TResult> class CommonPromise;
        template<class TFunctor, class R, class... ArgTypes> class BasicTask;
        
        template<class R>
        class BasicFuture
        {
            template<class>
            friend class BasicFuture;
            friend class FutureHelper;

        protected:
            using SharedState = get_shared_state_t<R>;

        public:
            BasicFuture() noexcept = default;
            BasicFuture(const BasicFuture& other) = default;
            BasicFuture(BasicFuture&& other) noexcept = default;

            BasicFuture(const CommonPromise<R>& promise);

            BasicFuture(SharedState&& state) noexcept;

            BasicFuture& operator=(const BasicFuture& rhs) = default;
            BasicFuture& operator=(BasicFuture&& other) noexcept = default;

            bool valid() const noexcept;
            bool is_ready() const noexcept;

            void wait() const;

            template <class Rep, class Period>
            std::future_status wait_for(const std::chrono::duration<Rep, Period>& rel_time) const;

            template <class Clock, class Duration>
            std::future_status wait_until(const std::chrono::time_point<Clock, Duration>& abs_time) const;

        protected:

            template<class T, template<typename> class NestedFuture>
            BasicFuture(BasicFuture<NestedFuture<T>>&& other) noexcept;

            SharedState ValidateState() const;

            template<class T>
            std::enable_if_t<std::is_void<T>::value>
                GetResult();

            template<class T>
            std::enable_if_t<!std::is_void<T>::value, T>
                GetResult();

            template<class T>
            std::enable_if_t<std::is_void<T>::value>
                GetResultRef() const;

            template<class T>
            std::enable_if_t<!std::is_void<T>::value, std::decay_t<T>&>
                GetResultRef() const;

            SharedState MoveState();

            void CheckState();

            template<class T>
            static shared_future<T> Share(future<T>&& future);

            template<class TContinuation, class TFuture>
            static decltype(auto) ThenMove(TContinuation&& continuation, TFuture& future);

            template<class TContinuation, class TFuture>
            static decltype(auto) ThenShare(TContinuation&& continuation, const TFuture& future);

            template<class TContinuation, class TFuture>
            static decltype(auto) ThenImpl(TContinuation&& continuation, TFuture&& future);

        private:

            SharedState GetState();

            template<class TResult>
            static decltype(auto) GetUnwrappedFuture(const CommonPromise<TResult>& promise);

            template<typename T, template<typename> class TOuter, template<typename> class TInner>
            static enable_if_future_t<TInner<T>> Unwrap(TOuter<TInner<T>>&& future);

            template<typename T, template<typename> class TOuter>
            static enable_if_not_future_t<T, TOuter<T>> Unwrap(TOuter<T>&& future);

            template<class TFunctor, class TResultType, class TArgType>
            static decltype(auto) 
                CreateCallback(BasicTask<TFunctor, TResultType, TArgType&>&& task, TArgType&& argument);

            template<class TFunctor, class TResultType, class TArgType>
            static decltype(auto) 
                CreateCallback(BasicTask<TFunctor, TResultType, TArgType>&& task, TArgType&& argument);

            template<class TFunctor, class TResultType, class TArgType>
            static decltype(auto) CreateCallback(BasicTask<TFunctor, TResultType, TArgType&&>&& task, TArgType&& argument);

            void ResetState();

            SharedState _state;
        };
    }
}
