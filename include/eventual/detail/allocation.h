#pragma once

#include <cstddef>
#include <memory>
#include <type_traits>
#include <cassert>
#include <atomic>

namespace eventual
{
    namespace detail
    {
        // a simplified implementation of a c++17 memory_resource

        class memory_resource;

        using shared_resource = std::shared_ptr<memory_resource>;

        class memory_resource
        {
            using size_t = std::size_t;
            using max_align_t = std::max_align_t;

        public:

            typedef void(*cleanup_t)(memory_resource*);

            //memory_resource() : _destroyed(false) { }

            //virtual ~memory_resource() { _destroyed = true; }

            void* allocate(size_t bytes, size_t alignment = alignof(max_align_t))
            {
                //assert(!_destroyed);
                return do_allocate(bytes, alignment);
            }

            void deallocate(void* p, size_t bytes, size_t alignment = alignof(max_align_t))
            {
                //assert(!_destroyed);
                do_deallocate(p, bytes, alignment);
            }

            bool is_equal(const memory_resource& other) const
            {
                //assert(!_destroyed);
                return do_is_equal(other);
            }

            shared_resource clone() const
            {
                //assert(!_destroyed);
                return do_clone();
            }

        protected:
            virtual void* do_allocate(size_t bytes, size_t alignment) = 0;
            virtual void do_deallocate(void* p, size_t bytes, size_t alignment) = 0;
            virtual bool do_is_equal(const memory_resource& other) const = 0;
            virtual shared_resource do_clone() const = 0;

        private:
            bool _destroyed;
        };

        inline bool operator==(const memory_resource& a, const memory_resource& b)
        {
            return (&a == &b || a.is_equal(b));
        }

        inline bool operator!=(const memory_resource& a, const memory_resource& b)
        {
            return !(a == b);
        }

        template <size_t Alignment> struct aligned_block;
        template <> struct aligned_block<1>  { alignas(1)  char _x; };
        template <> struct aligned_block<2>  { alignas(2)  char _x; };
        template <> struct aligned_block<4>  { alignas(4)  char _x; };
        template <> struct aligned_block<8>  { alignas(8)  char _x; };
        template <> struct aligned_block<16> { alignas(16) char _x; };
        template <> struct aligned_block<32> { alignas(32) char _x; };
        template <> struct aligned_block<64> { alignas(64) char _x; };

        template<class Allocator>
        class resource_allocator_impl 
            : public memory_resource
        {
            using size_t = std::size_t;

        public:
            typedef Allocator allocator_type;

            resource_allocator_impl() = default;

            resource_allocator_impl(const resource_allocator_impl&) = default;

            resource_allocator_impl(const Allocator& allocator)
                : _allocator(allocator)
            { }

            resource_allocator_impl(Allocator&& allocator)
                : _allocator(std::forward<Allocator>(allocator))
            { }

            static shared_resource create_shared(allocator_type allocator)
            {
                return std::allocate_shared<resource_allocator_impl>(allocator, allocator);
            }

        protected:

            virtual void* do_allocate(size_t bytes, size_t alignment) override
            {
                switch (alignment)
                {
                    case 1: return do_allocate<1>(bytes);
                    case 2: return do_allocate<2>(bytes);
                    case 4: return do_allocate<4>(bytes);
                    case 8: return do_allocate<8>(bytes);
                    case 16: return do_allocate<16>(bytes);
                    case 32: return do_allocate<32>(bytes);
                    case 64: return do_allocate<64>(bytes);
                    default:
                    {
                        auto blocks = (bytes + sizeof(void*) + alignment - 1) / 64;
                        auto blockBytes = blocks * 64;
                        auto beginning = do_allocate<64>(blockBytes);

                        //save room for memory address
                        auto p = static_cast<char*>(beginning) + sizeof(void*);

                        //round up to the alignment boundary
                        p += alignment - 1;
                        p -= (size_t(p)) & (alignment - 1);

                        //store the memory address
                        reinterpret_cast<void**>(p)[-1] = beginning;

                        return p;
                    }
                }
            }

            virtual void do_deallocate(void* p, size_t bytes, size_t alignment) override
            {
                switch (alignment)
                {
                    case 1: do_deallocate<1>(p, bytes); break;
                    case 2: do_deallocate<2>(p, bytes); break;
                    case 4: do_deallocate<4>(p, bytes); break;
                    case 8: do_deallocate<8>(p, bytes); break;
                    case 16: do_deallocate<16>(p, bytes); break;
                    case 32: do_deallocate<32>(p, bytes); break;
                    case 64: do_deallocate<64>(p, bytes); break;
                    default:
                    {
                        auto blocks = (bytes + sizeof(void*) + alignment - 1) / 64;
                        auto blockBytes = blocks * 64;

                        auto beginning = reinterpret_cast<void**>(p)[-1];
                        do_deallocate<64>(beginning, blockBytes);
                    }
                }
            }

            virtual bool do_is_equal(const memory_resource& other) const override
            {
                const auto otherPtr = dynamic_cast<const resource_allocator_impl*>(std::addressof(other));
                if (!otherPtr)
                    return false;

                return _allocator == otherPtr->_allocator;
            }

            virtual shared_resource do_clone() const override
            {
                return create_shared(_allocator);
            }

        private:

            template<size_t Alignment>
            void* do_allocate(size_t bytes)
            {
                //using block = std::aligned_storage<1, Alignment>;
                using block_t = aligned_block<Alignment>;
                using block_traits = std::allocator_traits<allocator_type>::template
                    rebind_traits<block_t>;
                using block_alloc = block_traits::allocator_type;

                auto blocks = (bytes + Alignment - 1) / Alignment;
                auto allocator = block_alloc(_allocator);

                auto blockSize = sizeof(block_t);
                assert((blocks * blockSize) >= bytes);

                return block_traits::allocate(allocator, blocks);
            }

            template<size_t Alignment>
            void do_deallocate(void* p, size_t bytes)
            {
                //using block = std::aligned_storage<1, Alignment>;
                using block_t = aligned_block<Alignment>;
                using block_traits = std::allocator_traits<allocator_type>::template
                    rebind_traits<block_t>;
                using block_alloc = block_traits::allocator_type;

                auto blocks = (bytes + Alignment - 1) / Alignment;
                auto allocator = block_alloc(_allocator);

                auto blockSize = sizeof(block_t);
                assert((blocks * blockSize) >= bytes);

                block_traits::deallocate(allocator, static_cast<block_t*>(p), blocks);
            }

            allocator_type _allocator;
        };

        template<class Allocator>
        using resource_adapter = resource_allocator_impl<std::allocator_traits<Allocator>::template rebind_alloc<char>>;

        class default_resource_singleton
        {
        public:

            static memory_resource* new_delete_resource_singleton() noexcept
            {
                static resource_adapter<std::allocator<char>> instance;

                return &instance;
            }
        };

        inline memory_resource* get_default_resource() noexcept
        {
            return default_resource_singleton::new_delete_resource_singleton();
        }

        template<class T>
        class polymorphic_allocator
        {
        public:
            typedef T value_type;
            typedef value_type* pointer;
            typedef std::size_t size_type;

            polymorphic_allocator() noexcept
                : _resource(get_default_resource())
            {
                assert(_resource);
            }

            polymorphic_allocator(memory_resource* resource) noexcept
                : _resource(resource)
            {
                assert(_resource);
            }

            template<class U>
            polymorphic_allocator(const polymorphic_allocator<U>& other)
                : _resource(other.resource())
            {
                assert(_resource);
            }

            polymorphic_allocator(const polymorphic_allocator& other) = default;

            polymorphic_allocator& operator=(const polymorphic_allocator& rhs) = default;

            memory_resource* resource() const
            {
                return _resource;
            }

            pointer allocate(size_type n)
            {
                return static_cast<pointer>(_resource->allocate(n * sizeof(value_type), alignof(value_type)));
            }

            void deallocate(pointer p, size_type n)
            {
                _resource->deallocate(p, n * sizeof(value_type), alignof(value_type));
            }

            template<class U, class... Args>
            void construct(U* p, Args&&... args)
            {
                ::new(p) U(std::forward<Args>(args)...);
            }

            template<class U>
            void destroy(U* p)
            {
                assert(p);
                p->~U();
            }

            polymorphic_allocator select_on_container_copy_construction() const
            {
                return polymorphic_allocator();
            }

        private:

            memory_resource* _resource;
        };

        template<class T>
        class strong_polymorphic_allocator 
            : public polymorphic_allocator<T>
        {

        public:
            template<class Alloc, class = std::enable_if_t<!std::is_convertible<Alloc, strong_polymorphic_allocator>::value>>
            strong_polymorphic_allocator(const Alloc& alloc)
                : strong_polymorphic_allocator(resource_adapter<Alloc>::create_shared(alloc))
            {
                assert(_resource);
            }

            template<class U>
            strong_polymorphic_allocator(const strong_polymorphic_allocator<U>& other)
                : strong_polymorphic_allocator(other.share())
            {
                assert(_resource);
            }

            strong_polymorphic_allocator(shared_resource&& resource)
                : polymorphic_allocator<T>(resource.get()), 
                  _resource(std::forward<shared_resource>(resource))
            {
                assert(_resource);
            }

            strong_polymorphic_allocator(const strong_polymorphic_allocator& other) = default;
            strong_polymorphic_allocator& operator=(const strong_polymorphic_allocator& rhs) = default;

            shared_resource share() const
            {
                return _resource;
            }

        private:
            shared_resource _resource;
        };

        template<class T1, class T2>
        inline bool operator==(const polymorphic_allocator<T1>& a, const polymorphic_allocator<T2>& b)
        {
            return (&a == &b || a.resource() == b.resource());
        }

        template<class T1, class T2>
        inline bool operator!=(const polymorphic_allocator<T1>& a, const polymorphic_allocator<T2>& b)
        {
            return !(a == b);
        }
    }
}