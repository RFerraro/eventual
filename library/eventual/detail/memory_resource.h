#pragma once

#include <cstddef>

namespace eventual
{
    namespace detail
    {
        class memory_resource
        {
            using size_t = std::size_t;
            using max_align_t = std::max_align_t;

        public:

            virtual ~memory_resource() { }

            inline void* allocate(size_t bytes, size_t alignment = alignof(max_align_t));

            inline void deallocate(void* p, size_t bytes, size_t alignment = alignof(max_align_t));

            inline bool is_equal(const memory_resource& other) const;

        protected:
            virtual void* do_allocate(size_t bytes, size_t alignment) = 0;
            virtual void do_deallocate(void* p, size_t bytes, size_t alignment) = 0;
            virtual bool do_is_equal(const memory_resource& other) const = 0;
        };

        inline bool operator==(const memory_resource& a, const memory_resource& b);

        inline bool operator!=(const memory_resource& a, const memory_resource& b);
    }
}

