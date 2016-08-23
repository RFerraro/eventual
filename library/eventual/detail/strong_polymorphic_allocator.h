#pragma once

#include <type_traits>
#include <memory>

#include "polymorphic_allocator.h"

namespace eventual
{
    namespace detail
    {
        class memory_resource;
        
        template<class T>
        class strong_polymorphic_allocator
            : public polymorphic_allocator<T>
        {
            using Base = polymorphic_allocator<T>;
            using shared_resource = std::shared_ptr<memory_resource>;

        public:

            // needed for libstdc++
            using value_type = typename Base::value_type;
            using pointer = typename Base::pointer;
            using size_type = typename Base::size_type;
            using const_pointer = typename Base::const_pointer;
            using reference = typename Base::reference;
            using const_reference = typename Base::const_reference;
            using difference_type = typename Base::difference_type;

            template<class U>
            struct rebind
            {
                typedef strong_polymorphic_allocator<U> other;
            };

            template<class Alloc, class = std::enable_if_t<!std::is_convertible<Alloc, strong_polymorphic_allocator>::value>>
            strong_polymorphic_allocator(const Alloc& alloc);
            template<class U>
            strong_polymorphic_allocator(const strong_polymorphic_allocator<U>& other);
            strong_polymorphic_allocator(shared_resource&& resource);
            strong_polymorphic_allocator(const strong_polymorphic_allocator& other) = default;
            strong_polymorphic_allocator& operator=(const strong_polymorphic_allocator& rhs) = default;

            shared_resource share() const;

        private:
            shared_resource _resource;
        };

        template<class T1, class T2>
        bool operator==(const polymorphic_allocator<T1>& a, const polymorphic_allocator<T2>& b);

        template<class T1, class T2>
        bool operator!=(const polymorphic_allocator<T1>& a, const polymorphic_allocator<T2>& b);
    }
}
