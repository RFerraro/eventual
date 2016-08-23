#pragma once

#include "strong_polymorphic_allocator.h"

#include <type_traits>
#include <utility>

#include "polymorphic_allocator.inl"
#include "resource_adapter.inl"

namespace eventual
{
    namespace detail
    {
        template<class T>
        template<class Alloc, class>
        strong_polymorphic_allocator<T>::strong_polymorphic_allocator(const Alloc& alloc)
            : strong_polymorphic_allocator(resource_adapter<Alloc>::create_shared(alloc))
        {
            assert(_resource);
        }

        template<class T>
        template<class U>
        strong_polymorphic_allocator<T>::strong_polymorphic_allocator(const strong_polymorphic_allocator<U>& other)
            : strong_polymorphic_allocator(other.share())
        {
            assert(_resource);
        }

        template<class T>
        strong_polymorphic_allocator<T>::strong_polymorphic_allocator(shared_resource&& resource)
            : polymorphic_allocator<T>(resource.get()),
            _resource(std::forward<shared_resource>(resource))
        {
            assert(_resource);
        }

        template<class T>
        typename strong_polymorphic_allocator<T>::shared_resource strong_polymorphic_allocator<T>::share() const
        {
            return _resource;
        }

        template<class T1, class T2>
        bool operator==(const polymorphic_allocator<T1>& a, const polymorphic_allocator<T2>& b)
        {
            return (&a == &b || a.resource() == b.resource());
        }

        template<class T1, class T2>
        bool operator!=(const polymorphic_allocator<T1>& a, const polymorphic_allocator<T2>& b)
        {
            return !(a == b);
        }
    }
}