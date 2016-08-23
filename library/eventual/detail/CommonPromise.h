#pragma once

#include <memory>

#include "traits.h"
#include "State.h"
#include "CompositeState.h"

namespace eventual
{
    template<class TResult> class future;
    
    namespace detail
    {
        template<class TResult> class BasicFuture;
        
        template<class R>
        class CommonPromise
        {
            friend class BasicFuture<R>;

        protected:
            using StateType = get_state_t<R>;
            using SharedState = get_shared_state_t<R>;

        public:

            CommonPromise();
            CommonPromise(const CommonPromise& other) = delete;
            CommonPromise(CommonPromise&& other) noexcept = default;

            CommonPromise(SharedState state);

            template<class Alloc>
            CommonPromise(std::allocator_arg_t, const Alloc& alloc);

            ~CommonPromise() noexcept;

            CommonPromise& operator=(const CommonPromise& rhs) = delete;
            CommonPromise& operator=(CommonPromise&& other) noexcept = default;

            void swap(CommonPromise& other) noexcept;

            future<R> get_future();

        protected:

            template<class TValue>
            void SetValue(TValue&& value);

            template<class TValue>
            void SetValueAtThreadExit(TValue&& value);

            void SetException(std::exception_ptr exceptionPtr);

            void SetExceptionAtThreadExit(std::exception_ptr exceptionPtr);

            template<class TValue>
            bool TrySetValue(TValue&& value);

            void Reset();

            bool Valid() const noexcept;

            SharedState ValidateState() const;

            SharedState CopyState() const;

        private:

            SharedState _state;
        };
    }
}
