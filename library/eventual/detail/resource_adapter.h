#pragma once

#include <cstddef>
#include <memory>

#include "memory_resource.h"

namespace eventual
{
    namespace detail
    {
        template<class Allocator>
        class resource_adapter_impl
            : public memory_resource
        {
            using size_t = std::size_t;
            using shared_resource = std::shared_ptr<memory_resource>;
            using allocator_type = Allocator;

        public:

            resource_adapter_impl() = default;
            resource_adapter_impl(const resource_adapter_impl&) = default;
            resource_adapter_impl(const Allocator& allocator);
            resource_adapter_impl(Allocator&& allocator);

            static shared_resource create_shared(allocator_type allocator);

        protected:

            virtual void* do_allocate(size_t bytes, size_t alignment) override;
            virtual void do_deallocate(void* p, size_t bytes, size_t alignment) override;
            virtual bool do_is_equal(const memory_resource& other) const override;

        private:

            template<size_t Alignment>
            void* do_allocate(size_t bytes);
            void* do_allocate_n(size_t bytes, size_t alignment);

            template<size_t Alignment>
            void do_deallocate(void* p, size_t bytes);
            void do_deallocate_n(void* p, size_t bytes, size_t alignment);

            allocator_type _allocator;
        };

        template<class Allocator>
        using resource_adapter = resource_adapter_impl<typename std::allocator_traits<Allocator>::template rebind_alloc<char>>;
    }
}
