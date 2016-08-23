#pragma once

#include "BasicPromise.h"

#include "FutureHelper.inl"
#include "BasicFuture.inl"

namespace eventual
{
    namespace detail
    {
        template<class R>
        BasicPromise<R>::BasicPromise(SharedState state)
            : Base(state)
        { }

        template<class R>
        template<class Alloc>
        BasicPromise<R>::BasicPromise(std::allocator_arg_t arg, const Alloc& alloc)
            : Base(arg, alloc)
        { }

        template<class R>
        void BasicPromise<R>::set_exception(std::exception_ptr exceptionPtr)
        {
            Base::SetException(exceptionPtr);
        }

        template<class R>
        void BasicPromise<R>::set_exception_at_thread_exit(std::exception_ptr exceptionPtr)
        {
            Base::SetExceptionAtThreadExit(exceptionPtr);
        }

        template<class R>
        template<class TValue>
        void BasicPromise<R>::SetValue(TValue&& value)
        {
            Base::SetValue(std::forward<TValue>(value));
        }

        template<class R>
        template<class TValue>
        void BasicPromise<R>::SetValueAtThreadExit(TValue&& value)
        {
            Base::SetValueAtThreadExit(std::forward<TValue>(value));
        }

        template<class R>
        void BasicPromise<R>::SetException(std::exception_ptr exceptionPtr)
        {
            Base::SetException(exceptionPtr);
        }

        template<class R>
        void BasicPromise<R>::SetExceptionAtThreadExit(std::exception_ptr exceptionPtr)
        {
            Base::SetExceptionAtThreadExit(exceptionPtr);
        }
    }
}
