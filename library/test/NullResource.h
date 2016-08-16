#pragma once

#include <cstdlib>
#include <cstddef>
#include <memory>

#include <eventual/eventual.h>

class NullResource : public eventual::detail::memory_resource
{
    using size_t = std::size_t;
    using max_align_t = std::max_align_t;

public:

    NullResource() { }

protected:

    virtual void* do_allocate(size_t, size_t) override
    {
        return nullptr;
    }

    virtual void do_deallocate(void* p, size_t, size_t) override
    {
        assert(!p);
        (void)p;
    }

    virtual bool do_is_equal(const memory_resource& other) const override
    {
        return this == std::addressof(other);
    }
};