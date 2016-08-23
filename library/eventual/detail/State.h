#pragma once

#include <mutex>
#include <memory>
#include <list>
#include <exception>

#include "strong_polymorphic_allocator.h"
#include "SimpleDelegate.h"
#include "ResultBlock.h"
#include "traits.h"

namespace eventual
{
    namespace detail
    {
        struct StateTag { explicit StateTag(int) { } };

        template<typename T>
        class State
        {
        public:
            
            using unique_lock = std::unique_lock<std::mutex>;
            using strong_reference = std::shared_ptr<State>;
            using weak_reference = std::weak_ptr<State>;
            using allocator_t = strong_polymorphic_allocator<State>;
            using delegate_allocator_t = strong_polymorphic_allocator<SimpleDelegate>;
            using callbacks_t = std::list<SimpleDelegate, delegate_allocator_t>;

            template<class Alloc>
            State(const StateTag&, const Alloc& alloc);

            static std::shared_ptr<State> MakeState();

            template<class Alloc, class = typename enable_if_not_same<State, Alloc>::type>
            static std::shared_ptr<State> AllocState(const Alloc& alloc);

            State(State&&) = delete;
            State(const State&) = delete;
            State& operator=(State&&) = delete;
            State& operator=(const State&) = delete;

            const allocator_t& Get_Allocator() const;

            bool Is_Ready() const;

            void Wait() const;

            template <class TDuration>
            bool Wait_For(const TDuration& rel_time);

            template <class TTime>
            bool Wait_Until(const TTime& abs_time);

            template<class TCallback>
            void SetCallback(TCallback&& callback);

            void NotifyCompletion();

            bool HasException();
            std::exception_ptr GetException();
            bool HasResult();

            // future retrieved
            bool SetRetrieved();

            bool SetException(std::exception_ptr ex);

            bool SetExceptionAtThreadExit(std::exception_ptr ex);

            T GetResult();

            const T& GetResult() const;

            template<class TValue>
            bool SetResult(TValue&& value);

            template<class TValue>
            bool SetResultAtThreadExit(TValue&& value);

        protected:

            strong_reference GetNotifier();

            void SetNotifier(const weak_reference& notifier);

            unique_lock AquireLock() const;

            void InvokeContinuations();

            void Wait(unique_lock& lock) const;

        private:

            void NotifyPromiseFullfilled();

            void NotifyPromiseFullfilledAtThreadExit();

            bool SetHasResult();

            void CheckException() const;

            bool SetExceptionImpl(std::exception_ptr ex);

            bool _retrieved;
            bool _hasResult;
            ResultBlock<T> _result;
            std::exception_ptr _exception;
            weak_reference _self;

            int _ready;
            mutable std::mutex _mutex;
            mutable std::condition_variable _condition;
            allocator_t _allocator;
            callbacks_t _callbacks;
        };
    }
}
