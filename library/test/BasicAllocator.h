#pragma once
#include <memory>

template<class T>
struct BasicAllocator
{
   typedef T value_type;
   typedef value_type* pointer;
   BasicAllocator() : _allocatedObjects(new int()) { }
   BasicAllocator(const BasicAllocator&) = default;
   BasicAllocator(BasicAllocator&&) = default;

   template<class U>
   BasicAllocator(const BasicAllocator<U>& other)
      : _allocatedObjects(other._allocatedObjects)
   { }

   BasicAllocator& operator=(const BasicAllocator&) = default;
   BasicAllocator& operator=(BasicAllocator&&) = default;

   pointer allocate(std::size_t n)
   {
      auto ptr = reinterpret_cast<pointer>(::operator new(n * sizeof(T)));
      (*_allocatedObjects)++;
      return ptr;
   }

   void deallocate(pointer p, std::size_t)
   {
      ::operator delete(p);
      (*_allocatedObjects)--;
   }

   int GetCount()
   {
      return *_allocatedObjects;
   }

   std::shared_ptr<int> _allocatedObjects;
};

template <class T, class U>
bool operator==(const BasicAllocator<T>&, const BasicAllocator<U>&) { return true; }
template <class T, class U>
bool operator!=(const BasicAllocator<T>&, const BasicAllocator<U>&) { return false; }

