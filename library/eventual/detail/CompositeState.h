#pragma once

#include <exception>
#include <memory>

#include "traits.h"

namespace eventual
{
    namespace detail
    {
        template<class TResult> class BasicFuture;
        
        template<typename TPrimaryState, typename TSecondaryState>
        class CompositeState : public TPrimaryState, public TSecondaryState
        {
        public:

            template<class Alloc>
            CompositeState(const StateTag& tag, const Alloc& alloc);

            template<class TCallback>
            void SetCallback(TCallback&& callback);

            const typename TPrimaryState::allocator_t& Get_Allocator() const;

            bool HasResult();

            bool SetRetrieved();

            bool HasException();

            std::exception_ptr GetException();

            bool SetException(std::exception_ptr ex);

            get_state_result_t<TPrimaryState> GetResult();

            const get_state_result_t<TPrimaryState>& GetResult() const;

            template<class TValue>
            bool SetResult(TValue&& value);

            static std::shared_ptr<CompositeState> MakeState();

            template<class Alloc, class = typename enable_if_not_same<CompositeState, Alloc>::type>
            static std::shared_ptr<CompositeState> AllocState(const Alloc& alloc);

        protected:

            void SetNotifier(const std::weak_ptr<CompositeState>& notifier);

            std::shared_ptr<TPrimaryState> GetNotifier();

        private:
            template<class TState, class T>
            static void SetResultFromFuture(TState& state, BasicFuture<T>& future);
        };
    }
}