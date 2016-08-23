#pragma once

#include "ResultBlock.h"
#include <utility>
#include <cassert>

#include "utility.inl"

namespace eventual
{
    namespace detail
    {
        template<typename T>
        template<typename R>
        void ResultBlock<T>::Set(R&& result)
        {
            assert(!_result);

            // placement new
            _result.reset(new(&_data) T(std::forward<R>(result)));
        }

        template<typename T>
        T ResultBlock<T>::get()
        {
            assert(_result);

            return std::move(*_result);
        }

        template<typename T>
        const T& ResultBlock<T>::get() const
        {
            assert(_result != nullptr);

            return *_result;
        }
    }
}
