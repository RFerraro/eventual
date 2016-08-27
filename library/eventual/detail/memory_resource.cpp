#pragma once

#include "memory_resource.h"

namespace eventual
{
    namespace detail
    {
        void* memory_resource::allocate(size_t bytes, size_t alignment)
        {
            return do_allocate(bytes, alignment);
        }

        void memory_resource::deallocate(void* p, size_t bytes, size_t alignment)
        {
            do_deallocate(p, bytes, alignment);
        }

        bool memory_resource::is_equal(const memory_resource& other) const
        {
            return do_is_equal(other);
        }

        bool eventual::detail::operator==(const memory_resource& a, const memory_resource& b)
        {
            return (&a == &b || a.is_equal(b));
        }

        bool eventual::detail::operator!=(const memory_resource& a, const memory_resource& b)
        {
            return !(a == b);
        }
    }
}