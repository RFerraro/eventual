#pragma once

#include <cstddef>

#include "../eventual_export.h"

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

            EVENTUAL_API void* allocate(size_t bytes, size_t alignment = alignof(max_align_t));

            EVENTUAL_API void deallocate(void* p, size_t bytes, size_t alignment = alignof(max_align_t));

            EVENTUAL_API bool is_equal(const memory_resource& other) const;

        protected:
            virtual void* do_allocate(size_t bytes, size_t alignment) = 0;
            virtual void do_deallocate(void* p, size_t bytes, size_t alignment) = 0;
            virtual bool do_is_equal(const memory_resource& other) const = 0;
        };

        EVENTUAL_API bool operator==(const memory_resource& a, const memory_resource& b);

        EVENTUAL_API bool operator!=(const memory_resource& a, const memory_resource& b);
    }
}

