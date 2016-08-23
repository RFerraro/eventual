#pragma once

#include "polymorphic_allocator.h"

#include <cassert>
#include <utility>

#include "memory_resource.h"

namespace eventual
{
    namespace detail
    {
        template<class T>
        polymorphic_allocator<T>::polymorphic_allocator() noexcept
            : _resource(get_default_resource())
        {
            assert(_resource);
        }

        template<class T>
        polymorphic_allocator<T>::polymorphic_allocator(memory_resource* resource) noexcept
            : _resource(resource)
        {
            assert(_resource);
        }

        template<class T>
        template<class U>
        polymorphic_allocator<T>::polymorphic_allocator(const polymorphic_allocator<U>& other)
            : _resource(other.resource())
        {
            assert(_resource);
        }

        template<class T>
        memory_resource* polymorphic_allocator<T>::resource() const
        {
            return _resource;
        }

        template<class T>
        typename polymorphic_allocator<T>::pointer polymorphic_allocator<T>::allocate(size_type n)
        {
            return static_cast<pointer>(_resource->allocate(n * sizeof(value_type), alignof(value_type)));
        }

        template<class T>
        void polymorphic_allocator<T>::deallocate(pointer p, size_type n)
        {
            _resource->deallocate(p, n * sizeof(value_type), alignof(value_type));
        }

        template<class T>
        template<class U, class... Args>
        void polymorphic_allocator<T>::construct(U* p, Args&&... args)
        {
            ::new(p) U(std::forward<Args>(args)...);
        }

        template<class T>
        template<class U>
        void polymorphic_allocator<T>::destroy(U* p)
        {
            assert(p);
            p->~U();
            (void)p; // suppresses C4100 in VS2015
        }

        template<class T>
        polymorphic_allocator<T> polymorphic_allocator<T>::select_on_container_copy_construction() const
        {
            return polymorphic_allocator();
        }
    }
}
