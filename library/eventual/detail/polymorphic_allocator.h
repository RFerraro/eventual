#pragma once

#include <cstddef>

namespace eventual
{
    namespace detail
    {
        class memory_resource;
        
        template<class T>
        class polymorphic_allocator
        {
        public:
            using value_type = typename T;
            using pointer = typename value_type*;
            using size_type = typename std::size_t;

            // needed for libstdc++
            using const_pointer = typename const pointer;
            using reference = typename value_type&;
            using const_reference = typename const reference;
            using difference_type = typename std::ptrdiff_t;

            template<class U>
            struct rebind
            {
                typedef polymorphic_allocator<U> other;
            };

            polymorphic_allocator() noexcept;
            polymorphic_allocator(memory_resource* resource) noexcept;
            template<class U>
            polymorphic_allocator(const polymorphic_allocator<U>& other);
            polymorphic_allocator(const polymorphic_allocator& other) = default;
            polymorphic_allocator& operator=(const polymorphic_allocator& rhs) = default;

            memory_resource* resource() const;
            pointer allocate(size_type n);
            void deallocate(pointer p, size_type n);
            template<class U, class... Args>
            void construct(U* p, Args&&... args);
            template<class U>
            void destroy(U* p);
            polymorphic_allocator select_on_container_copy_construction() const;

        private:

            memory_resource* _resource;
        };
    }
}
