#pragma once

#include "CompositeState.h"

#include <memory>

#include "FutureHelper.h"
#include "State.h"

namespace eventual
{
    namespace detail
    {
        template<typename TPrimaryState, typename TSecondaryState>
        template<class Alloc>
        CompositeState<TPrimaryState, TSecondaryState>::CompositeState(const StateTag& tag, const Alloc& alloc)
            : TPrimaryState(tag, alloc), TSecondaryState(tag, alloc)
        {
            SetCallback([this]() mutable
            {
                if (HasException())
                {
                    TSecondaryState::SetException(GetException());
                    return;
                }

                auto innerFuture = GetResult();
                if (!innerFuture.valid())
                {
                    TSecondaryState::SetException(CreateFutureExceptionPtr(std::future_errc::broken_promise));
                    return;
                }

                auto futureState = TSecondaryState::GetNotifier();
                innerFuture.then([futureState = std::move(futureState)](auto& future)
                {
                    CompositeState::SetResultFromFuture(*futureState, future);
                });
            });
        }

        template<typename TPrimaryState, typename TSecondaryState>
        template<class TCallback>
        void CompositeState<TPrimaryState, TSecondaryState>::SetCallback(TCallback&& callback)
        {
            TPrimaryState::SetCallback(std::forward<TCallback>(callback));
        }

        template<typename TPrimaryState, typename TSecondaryState>
        const typename TPrimaryState::allocator_t& CompositeState<TPrimaryState, TSecondaryState>::Get_Allocator() const
        {
            return TPrimaryState::Get_Allocator();
        }

        template<typename TPrimaryState, typename TSecondaryState>
        bool CompositeState<TPrimaryState, TSecondaryState>::HasResult()
        {
            return TPrimaryState::HasResult();
        }

        template<typename TPrimaryState, typename TSecondaryState>
        bool CompositeState<TPrimaryState, TSecondaryState>::SetRetrieved()
        {
            return TPrimaryState::SetRetrieved();
        }

        template<typename TPrimaryState, typename TSecondaryState>
        bool CompositeState<TPrimaryState, TSecondaryState>::HasException()
        {
            return TPrimaryState::HasException();
        }

        template<typename TPrimaryState, typename TSecondaryState>
        std::exception_ptr CompositeState<TPrimaryState, TSecondaryState>::GetException()
        {
            return TPrimaryState::GetException();
        }

        template<typename TPrimaryState, typename TSecondaryState>
        bool CompositeState<TPrimaryState, TSecondaryState>::SetException(std::exception_ptr ex)
        {
            return TPrimaryState::SetException(ex);
        }

        template<typename TPrimaryState, typename TSecondaryState>
        get_state_result_t<TPrimaryState> CompositeState<TPrimaryState, TSecondaryState>::GetResult()
        {
            return TPrimaryState::GetResult();
        }

        template<typename TPrimaryState, typename TSecondaryState>
        const get_state_result_t<TPrimaryState>& CompositeState<TPrimaryState, TSecondaryState>::GetResult() const
        {
            return TPrimaryState::GetResult();
        }

        template<typename TPrimaryState, typename TSecondaryState>
        template<class TValue>
        bool CompositeState<TPrimaryState, TSecondaryState>::SetResult(TValue&& value)
        {
            return TPrimaryState::SetResult(std::forward<TValue>(value));
        }

        template<typename TPrimaryState, typename TSecondaryState>
        std::shared_ptr<CompositeState<TPrimaryState, TSecondaryState>> CompositeState<TPrimaryState, TSecondaryState>::MakeState()
        {
            return AllocState(std::allocator<CompositeState>());
        }

        template<typename TPrimaryState, typename TSecondaryState>
        template<class Alloc, class>
        std::shared_ptr<CompositeState<TPrimaryState, TSecondaryState>> CompositeState<TPrimaryState, TSecondaryState>::AllocState(const Alloc& alloc)
        {
            auto state = std::allocate_shared<CompositeState>(alloc, StateTag(0), alloc);

            state->SetNotifier(state);

            return state;
        }

        template<typename TPrimaryState, typename TSecondaryState>
        void CompositeState<TPrimaryState, TSecondaryState>::SetNotifier(const std::weak_ptr<CompositeState>& notifier)
        {
            TPrimaryState::SetNotifier(notifier);
            TSecondaryState::SetNotifier(notifier);
        }

        template<typename TPrimaryState, typename TSecondaryState>
        std::shared_ptr<TPrimaryState> CompositeState<TPrimaryState, TSecondaryState>::GetNotifier()
        {
            return TPrimaryState::GetNotifier();
        }

        template<class TPrimaryState, class TSecondaryState>
        template<class TState, class T>
        void CompositeState<TPrimaryState, TSecondaryState>::SetResultFromFuture(TState& state, BasicFuture<T>& future)
        {
            auto innerState = FutureHelper::GetState(future);
            if (innerState->HasException())
            {
                state.SetException(innerState->GetException());
                return;
            }

            state.SetResult(innerState->GetResult());
        }
    }
}