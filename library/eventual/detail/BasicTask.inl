#pragma once

#include "BasicTask.h"
#include <utility>
#include <future>

#include "utility.h"

namespace eventual
{
    namespace detail
    {
        template<class TFunctor, class R, class... ArgTypes>
        BasicTask<TFunctor, R, ArgTypes...>::BasicTask() noexcept
            : Base(nullptr)
        { }

        template<class TFunctor, class R, class... ArgTypes>
        BasicTask<TFunctor, R, ArgTypes...>::BasicTask(BasicTask&& other) noexcept
            : Base(std::forward<BasicTask>(other)), _task(std::move(other._task))
        { }

        template<class TFunctor, class R, class... ArgTypes>
        template<class F, class Allocator, class>
        BasicTask<TFunctor, R, ArgTypes...>::BasicTask(std::allocator_arg_t arg, const Allocator& alloc, F&& task)
            : Base(arg, alloc), _task(CreateFunctor<TFunctor>(alloc, std::forward<F>(task)))
        { }

        template<class TFunctor, class R, class... ArgTypes>
        template<class F, class>
        BasicTask<TFunctor, R, ArgTypes...>::BasicTask(F&& task)
            : Base(), _task(std::forward<F>(task))
        { }

        template<class TFunctor, class R, class... ArgTypes>
        BasicTask<TFunctor, R, ArgTypes...>& BasicTask<TFunctor, R, ArgTypes...>::operator=(BasicTask&& other) noexcept
        {
            _task = std::move(other._task);
            Base::operator=(std::forward<BasicTask>(other));
            return *this;
        }

        template<class TFunctor, class R, class... ArgTypes>
        void BasicTask<TFunctor, R, ArgTypes...>::reset()
        {
            Base::Reset();
        }

        template<class TFunctor, class R, class... ArgTypes>
        bool BasicTask<TFunctor, R, ArgTypes...>::valid() const
        {
            return Base::Valid();
        }

        template<class TFunctor, class R, class... ArgTypes>
        void BasicTask<TFunctor, R, ArgTypes...>::operator()(ArgTypes... args)
        {
            try
            {
                Base::SetValue(ExecuteTask<R>(std::forward<ArgTypes>(args)...));
            }
            catch (const std::future_error& error)
            {
                if (error.code() != std::future_errc::promise_already_satisfied)
                    Base::SetException(std::current_exception());
                else
                    throw;
            }
            catch (...)
            {
                Base::SetException(std::current_exception());
            }
        }

        template<class TFunctor, class R, class... ArgTypes>
        void BasicTask<TFunctor, R, ArgTypes...>::make_ready_at_thread_exit(ArgTypes... args)
        {
            try
            {
                Base::SetValueAtThreadExit(ExecuteTask<R>(std::forward<ArgTypes>(args)...));
            }
            catch (const std::future_error& error)
            {
                if (error.code() != std::future_errc::promise_already_satisfied)
                    Base::SetExceptionAtThreadExit(std::current_exception());
                else
                    throw;
            }
            catch (...)
            {
                Base::SetExceptionAtThreadExit(std::current_exception());
            }
        }

        template<class TFunctor, class R, class... ArgTypes>
        void BasicTask<TFunctor, R, ArgTypes...>::swap(BasicTask& other) noexcept
        {
            Base::swap(other);
            std::swap(_task, other._task);
        }

        template<class TFunctor, class R, class... ArgTypes>
        template<class TResult>
        std::enable_if_t<!std::is_void<TResult>::value, TResult>
            BasicTask<TFunctor, R, ArgTypes...>::ExecuteTask(ArgTypes... args)
        {
            return _task(std::forward<ArgTypes>(args)...);
        }

        template<class TFunctor, class R, class... ArgTypes>
        template<class TResult>
        std::enable_if_t<std::is_void<TResult>::value, Unit>
            BasicTask<TFunctor, R, ArgTypes...>::ExecuteTask(ArgTypes... args)
        {
            _task(std::forward<ArgTypes>(args)...);
            return Unit();
        }
    }
}
