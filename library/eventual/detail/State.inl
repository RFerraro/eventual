#pragma once

#include "State.h"

#include <memory>
#include <cassert>
#include <queue>

#include "traits.h"
#include "strong_polymorphic_allocator.h"
#include "SimpleDelegate.h"
#include "ResultBlock.h"

namespace eventual
{
    namespace detail
    {
        template<class T>
        template<class Alloc>
        State<T>::State(const StateTag&, const Alloc& alloc) :
            _retrieved(false),
            _hasResult(false),
            _result(),
            _exception(nullptr),
            _self(),
            _ready(0),
            _mutex(),
            _condition(),
            _allocator(allocator_t(alloc)),
            _callbacks(delegate_allocator_t(_allocator))
        { }

        template<class T>
        std::shared_ptr<State<T>> State<T>::MakeState()
        {
            return AllocState(std::allocator<State>());
        }

        template<class T>
        template<class Alloc, class>
        std::shared_ptr<State<T>> State<T>::AllocState(const Alloc& alloc)
        {
            auto state = std::allocate_shared<State>(alloc, StateTag(0), alloc);

            state->SetNotifier(state);

            return state;
        }

        template<class T>
        const typename State<T>::allocator_t& State<T>::Get_Allocator() const
        {
            return _allocator;
        }

        template<class T>
        bool State<T>::Is_Ready() const
        {
            //todo: atomic read?
            return _ready != 0;
        }

        template<class T>
        void State<T>::Wait() const
        {
            auto lock = AquireLock();
            Wait(lock);
        }

        template<class T>
        template <class TDuration>
        bool State<T>::Wait_For(const TDuration& rel_time)
        {
            auto lock = AquireLock();
            return _condition.wait_for(lock, rel_time, [this]() { return Is_Ready(); });
        }

        template<class T>
        template <class TTime>
        bool State<T>::Wait_Until(const TTime& abs_time)
        {
            auto lock = AquireLock();
            return _condition.wait_until(lock, abs_time, [this]() { return Is_Ready(); });
        }

        template<class T>
        template<class TCallback>
        void State<T>::SetCallback(TCallback&& callback)
        {
            //todo: quicker way?
            {
                auto lock = AquireLock();

                if (!Is_Ready())
                {
                    _callbacks.emplace_back(std::forward<TCallback>(callback), _callbacks.get_allocator());
                    return;
                }
            }

            //promise is ready, invoke immediately
            callback();
        }

        template<class T>
        void State<T>::NotifyCompletion()
        {
            {
                auto lock = AquireLock();
                _ready = true;
                _condition.notify_all();
            }

            InvokeContinuations();
        }

        template<class T>
        bool State<T>::HasException()
        {
            return _exception ? true : false;
        }

        template<class T>
        std::exception_ptr State<T>::GetException()
        {
            return _exception;
        }

        template<class T>
        bool State<T>::HasResult()
        {
            return _hasResult;
        }

        template<class T>
        bool State<T>::SetRetrieved()
        {
            auto lock = AquireLock();
            if (_retrieved)
                return false;

            _retrieved = true;
            return _retrieved;
        }

        template<class T>
        bool State<T>::SetException(std::exception_ptr ex)
        {
            if (!SetExceptionImpl(ex))
                return false;

            NotifyPromiseFullfilled();
            return true;
        }

        template<class T>
        bool State<T>::SetExceptionAtThreadExit(std::exception_ptr ex)
        {
            if (!SetExceptionImpl(ex))
                return false;

            NotifyPromiseFullfilledAtThreadExit();
            return true;
        }

        template<class T>
        T State<T>::GetResult()
        {
            auto lock = AquireLock();

            Wait(lock);
            CheckException();
            return _result.get();
        }

        template<class T>
        const T& State<T>::GetResult() const
        {
            auto lock = AquireLock();

            Wait(lock);
            CheckException();
            return _result.get();
        }

        template<class T>
        template<class TValue>
        bool State<T>::SetResult(TValue&& value)
        {
            {
                auto lock = AquireLock();

                if (!SetHasResult())
                    return false;

                _result.Set(std::forward<TValue>(value));
            }

            NotifyPromiseFullfilled();
            return true;
        }

        template<class T>
        template<class TValue>
        bool State<T>::SetResultAtThreadExit(TValue&& value)
        {
            {
                auto lock = AquireLock();

                if (!SetHasResult())
                    return false;

                _result.Set(std::forward<TValue>(value));
            }

            NotifyPromiseFullfilledAtThreadExit();
            return true;
        }

        template<class T>
        typename State<T>::strong_reference State<T>::GetNotifier()
        {
            return _self.lock();
        }

        template<class T>
        void State<T>::SetNotifier(const weak_reference& notifier)
        {
            _self = notifier;
        }

        template<class T>
        typename State<T>::unique_lock State<T>::AquireLock() const
        {
            return unique_lock(_mutex);
        }

        template<class T>
        void State<T>::InvokeContinuations()
        {
            for (auto& cb : _callbacks)
                cb(); // todo: exceptions?
        }

        template<class T>
        void State<T>::Wait(unique_lock& lock) const
        {
            _condition.wait(lock, [this]() { return Is_Ready(); });
        }

        template<class T>
        void State<T>::NotifyPromiseFullfilled()
        {
            NotifyCompletion();
        }

        template<class T>
        void State<T>::NotifyPromiseFullfilledAtThreadExit()
        {
            auto block = _self.lock();
            assert(block);

            using namespace std;
            class ExitNotifier
            {
            public:
                ExitNotifier() = default;
                ExitNotifier(const ExitNotifier&) = delete;
                ExitNotifier& operator=(const ExitNotifier&) = delete;

                ~ExitNotifier() noexcept
                {
                    while (!_exitFunctions.empty())
                    {
                        _exitFunctions.front()->NotifyCompletion();
                        _exitFunctions.pop();
                    }
                }

                void Add(strong_reference&& block)
                {
                    _exitFunctions.emplace(forward<strong_reference>(block));
                }

            private:
                queue<strong_reference> _exitFunctions;
            };

            thread_local ExitNotifier notifier;
            notifier.Add(move(block));
        }

        template<class T>
        bool State<T>::SetHasResult()
        {
            if (_hasResult)
                return false;

            _hasResult = true;
            return _hasResult;
        }

        template<class T>
        void State<T>::CheckException() const
        {
            if (_exception)
                std::rethrow_exception(_exception);
        }

        template<class T>
        bool State<T>::SetExceptionImpl(std::exception_ptr ex)
        {
            auto lock = AquireLock();
            if (!SetHasResult())
                return false;

            _exception = ex;
            return true;
        }
    }
}
