#pragma once

#include "SimpleDelegate.h"

#include <memory>
#include <utility>

#include "polymorphic_allocator.h"
#include "utility.h"

#include "SimpleDelegate.cpp"

namespace eventual
{
    namespace detail
    {
        template<class T>
        SimpleDelegate::SmallDelegate<T>::SmallDelegate(T&& callable)
            : _callable(std::forward<T>(callable))
        { }

        template<class T>
        void SimpleDelegate::SmallDelegate<T>::operator()()
        {
            _callable();
        }

        template<class T>
        SimpleDelegate::LargeDelegate<T>::LargeDelegate(T&& callable, polymorphic_allocator<T>&& allocator)
            : _callable(Create(std::forward<T>(callable), allocator))
        { }

        template<class T>
        void SimpleDelegate::LargeDelegate<T>::operator()()
        {
            (*_callable)();
        }

        template<class T>
        std::unique_ptr<T, typename SimpleDelegate::LargeDelegate<T>::Deleter> 
            SimpleDelegate::LargeDelegate<T>::Create(T&& callable, polymorphic_allocator<T>& resource)
        {
            using traits = std::allocator_traits<polymorphic_allocator<T>>;

            auto ptr = traits::allocate(resource, 1);
            traits::construct(resource, ptr, std::forward<T>(callable));

            return std::unique_ptr<T, Deleter>(ptr, Deleter(resource));
        }

        template<class T>
        SimpleDelegate::LargeDelegate<T>::Deleter::Deleter(const polymorphic_allocator<T>& resource)
            : _resource(resource)
        { }

        template<class T>
        void SimpleDelegate::LargeDelegate<T>::Deleter::operator()(T* functor)
        {
            using traits = std::allocator_traits<polymorphic_allocator<T>>;

            traits::destroy(_resource, functor);
            traits::deallocate(_resource, functor, 1);
        }

        template<class T, class>
        SimpleDelegate::SimpleDelegate(T&& functor, const polymorphic_allocator<SimpleDelegate>& alloc)
            : _data(),
            _object(Create(std::forward<T>(functor), alloc))
        { }

        template<class T>
        enable_if_size_is_less_than_or_eq_t<SimpleDelegate::SmallDelegate<T>, SimpleDelegate::storage_t, SimpleDelegate::delegate_pointer_t>
            SimpleDelegate::Create(T&& functor, const polymorphic_allocator<SimpleDelegate>)
        {
            static_assert(sizeof(SmallDelegate<T>) <= sizeof(storage_t), "Delegate does not fit in storage buffer.");

            return delegate_pointer_t(new(&_data) SmallDelegate<T>(std::forward<T>(functor)));
        }

        template<class T>
        enable_if_size_is_greater_than_t<SimpleDelegate::SmallDelegate<T>, SimpleDelegate::storage_t, SimpleDelegate::delegate_pointer_t>
            SimpleDelegate::Create(T&& functor, const polymorphic_allocator<SimpleDelegate>& alloc)
        {
            static_assert(sizeof(SmallDelegate<T>) > sizeof(storage_t), "Incorrect delegate choosen.");
            static_assert(sizeof(LargeDelegate<T>) <= sizeof(storage_t), "Pointer delegate does not fit in storage buffer.");

            return delegate_pointer_t(new(&_data) LargeDelegate<T>(std::forward<T>(functor), polymorphic_allocator<T>(alloc)));
        }
    }
}
