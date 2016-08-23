#pragma once

#include "resource_adapter.h"

#include <utility>
#include <memory>
#include <type_traits>
#include <cassert>
#include <cstddef>

#include "memory_resource.inl"

namespace eventual
{
    namespace detail
    {
        template<class Allocator>
        resource_adapter_impl<Allocator>::resource_adapter_impl(const Allocator& allocator)
            : _allocator(allocator)
        { }

        template<class Allocator>
        resource_adapter_impl<Allocator>::resource_adapter_impl(Allocator&& allocator)
            : _allocator(std::forward<Allocator>(allocator))
        { }

        template<class Allocator>
        typename resource_adapter_impl<Allocator>::shared_resource resource_adapter_impl<Allocator>::create_shared(allocator_type allocator)
        {
            return std::allocate_shared<resource_adapter_impl>(allocator, allocator);
        }

        template<class Allocator>
        void* resource_adapter_impl<Allocator>::do_allocate(size_t bytes, size_t alignment)
        {
            switch (alignment)
            {
                case 1: return do_allocate<1>(bytes);
                case 2: return do_allocate<2>(bytes);
                case 4: return do_allocate<4>(bytes);
                case 8: return do_allocate<8>(bytes);
                default: return do_allocate_n(bytes, alignment);
            }
        }

        template<class Allocator>
        void resource_adapter_impl<Allocator>::do_deallocate(void* p, size_t bytes, size_t alignment)
        {
            switch (alignment)
            {
                case 1: do_deallocate<1>(p, bytes); break;
                case 2: do_deallocate<2>(p, bytes); break;
                case 4: do_deallocate<4>(p, bytes); break;
                case 8: do_deallocate<8>(p, bytes); break;
                default: do_deallocate_n(p, bytes, alignment); break;
            }
        }

        template<class Allocator>
        bool resource_adapter_impl<Allocator>::do_is_equal(const memory_resource& other) const
        {
            const auto otherPtr = dynamic_cast<const resource_adapter_impl*>(std::addressof(other));
            if (!otherPtr)
                return false;

            return _allocator == otherPtr->_allocator;
        }

        template<class Allocator>
        template<size_t Alignment>
        void* resource_adapter_impl<Allocator>::do_allocate(size_t bytes)
        {
            using block_t = std::aligned_storage_t<1, Alignment>;
            using block_traits = typename std::allocator_traits<allocator_type>::template
                rebind_traits<block_t>;
            using block_alloc = typename block_traits::allocator_type;

            auto blocks = (bytes + Alignment - 1) / Alignment;
            auto allocator = block_alloc(_allocator);

            assert((blocks * sizeof(block_t)) >= bytes);

            return block_traits::allocate(allocator, blocks);
        }

        template<class Allocator>
        void* resource_adapter_impl<Allocator>::do_allocate_n(size_t bytes, size_t alignment)
        {
            constexpr auto maxAlignment = alignof(std::max_align_t);

            // check if alignment is a power of 2...
            assert(alignment != 0);
            assert((alignment & (alignment - 1)) == 0);

            auto blocks = (bytes + sizeof(void*) + alignment - 1) / maxAlignment;
            auto blockBytes = blocks * maxAlignment;
            auto beginning = do_allocate<maxAlignment>(blockBytes);

            //save room for memory address
            auto p = static_cast<char*>(beginning) + sizeof(void*);

            //round up to the alignment boundary
            auto unaligned = static_cast<void*>(p);
            auto aligned = std::align(alignment, sizeof(char), unaligned, blockBytes);

            assert(aligned != nullptr);

            //store the memory address
            reinterpret_cast<void**>(aligned)[-1] = beginning;

            return aligned;
        }

        template<class Allocator>
        template<size_t Alignment>
        void resource_adapter_impl<Allocator>::do_deallocate(void* p, size_t bytes)
        {
            using block_t = std::aligned_storage_t<1, Alignment>;
            using block_traits = typename std::allocator_traits<allocator_type>::template
                rebind_traits<block_t>;
            using block_alloc = typename block_traits::allocator_type;

            auto blocks = (bytes + Alignment - 1) / Alignment;
            auto allocator = block_alloc(_allocator);

            assert((blocks * sizeof(block_t)) >= bytes);

            block_traits::deallocate(allocator, static_cast<block_t*>(p), blocks);
        }

        template<class Allocator>
        void resource_adapter_impl<Allocator>::do_deallocate_n(void* p, size_t bytes, size_t alignment)
        {
            constexpr auto maxAlignment = alignof(std::max_align_t);

            // check if alignment is a power of 2...
            assert(alignment != 0);
            assert((alignment & (alignment - 1)) == 0);

            auto blocks = (bytes + sizeof(void*) + alignment - 1) / maxAlignment;
            auto blockBytes = blocks * maxAlignment;

            auto beginning = reinterpret_cast<void**>(p)[-1];
            do_deallocate<maxAlignment>(beginning, blockBytes);
        }
    }
}
