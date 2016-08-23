#pragma once

#include <memory>
#include <type_traits>

#include "CommonPromise.h"
#include "traits.h"

namespace eventual
{
    namespace detail
    {
        template<class TFunctor, class R, class... ArgTypes>
        class BasicTask : public CommonPromise<R>
        {
            using Base = CommonPromise<R>;

        public:

            BasicTask() noexcept;
            BasicTask(const BasicTask&) = delete;
            BasicTask(BasicTask&& other) noexcept;

            template<class F, class Allocator, class = typename enable_if_not_same<BasicTask, F>::type>
            BasicTask(std::allocator_arg_t arg, const Allocator& alloc, F&& task);

            template<class F, class = typename enable_if_not_same<BasicTask, F>::type>
            BasicTask(F&& task);

            BasicTask& operator=(BasicTask&& other) noexcept;

            void reset();

            bool valid() const;

            void operator()(ArgTypes... args);

            void make_ready_at_thread_exit(ArgTypes... args);

            void swap(BasicTask& other) noexcept;

        private:

            template<class TResult>
            std::enable_if_t<!std::is_void<TResult>::value, TResult>
                ExecuteTask(ArgTypes... args);

            template<class TResult>
            std::enable_if_t<std::is_void<TResult>::value, Unit>
                ExecuteTask(ArgTypes... args);

            TFunctor _task;
        };
    }
}
