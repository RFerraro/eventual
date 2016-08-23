#pragma once

#include <memory>
#include <exception>

#include "CommonPromise.h"

namespace eventual
{
    namespace detail
    {
        class FutureHelper;

        template<class TResult> class BasicFuture;
        
        template<class R>
        class BasicPromise : public CommonPromise<R>
        {
            using Base = CommonPromise<R>;
            using SharedState = typename Base::SharedState;

            friend class FutureHelper;
            friend class BasicFuture<R>;

        public:
            BasicPromise() = default;
            BasicPromise(const BasicPromise& other) = delete;
            BasicPromise(BasicPromise&& other) noexcept = default;

            BasicPromise(SharedState state);

            template<class Alloc>
            BasicPromise(std::allocator_arg_t arg, const Alloc& alloc);

            BasicPromise& operator=(const BasicPromise& rhs) = delete;
            BasicPromise& operator=(BasicPromise&& other) noexcept = default;

            void set_exception(std::exception_ptr exceptionPtr);
            void set_exception_at_thread_exit(std::exception_ptr exceptionPtr);

        protected:

            template<class TValue>
            void SetValue(TValue&& value);

            template<class TValue>
            void SetValueAtThreadExit(TValue&& value);

            void SetException(std::exception_ptr exceptionPtr);

            void SetExceptionAtThreadExit(std::exception_ptr exceptionPtr);
        };
    }
}
