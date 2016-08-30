#pragma once

#include <type_traits>
#include <memory>

#include "../eventual_export.h"
#include "traits.h"
#include "polymorphic_allocator.h"
#include "utility.h"

namespace eventual
{
    namespace detail
    {        
        class SimpleDelegate
        {
            class Delegate
            {
            public:
                virtual EVENTUAL_API ~Delegate();
                virtual void operator()() = 0;
            };

            template<class T>
            class SmallDelegate : public Delegate
            {
            public:

                SmallDelegate(T&& callable);

                virtual EVENTUAL_API void operator()() override;

            private:
                T _callable;
            };

            template<class T>
            class LargeDelegate : public Delegate
            {
            public:

                LargeDelegate(T&& callable, polymorphic_allocator<T>&& allocator);

                virtual EVENTUAL_API void operator()() override;

            private:

                struct Deleter
                {
                    Deleter(const polymorphic_allocator<T>& resource);

                    void operator()(T* functor);

                    polymorphic_allocator<T> _resource;
                };

                std::unique_ptr<T, Deleter> Create(T&& callable, polymorphic_allocator<T>& resource);

                std::unique_ptr<T, Deleter> _callable;
            };

            // dummy type
            struct Callable
            {
                EVENTUAL_API void operator()();
            };

            //todo: fine-tune this size
            using storage_t = std::aligned_union_t<32, SmallDelegate<Callable>, LargeDelegate<Callable>>;

            using delegate_pointer_t = std::unique_ptr<Delegate, PlacementDestructor<Delegate>>;

        public:

            template<class T, class = enable_if_not_same_t<SimpleDelegate, T>>
            SimpleDelegate(T&& functor, const polymorphic_allocator<SimpleDelegate>& alloc);

            // only contructable in-place. No move/copy.
            SimpleDelegate(SimpleDelegate&&) = delete;
            SimpleDelegate& operator=(const SimpleDelegate&) = delete;
            SimpleDelegate(const SimpleDelegate&) = delete;
            SimpleDelegate& operator=(SimpleDelegate&&) = default;

            EVENTUAL_API void operator()() const;

        private:

            template<class T>
            enable_if_size_is_less_than_or_eq_t<SmallDelegate<T>, storage_t, delegate_pointer_t>
                Create(T&& functor, const polymorphic_allocator<SimpleDelegate>);

            template<class T>
            enable_if_size_is_greater_than_t<SmallDelegate<T>, storage_t, delegate_pointer_t>
                Create(T&& functor, const polymorphic_allocator<SimpleDelegate>& alloc);

            storage_t _data;
            delegate_pointer_t _object;
        };
    }
}
