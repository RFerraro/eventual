#pragma once

#include <type_traits>
#include <memory>

#include "utility.h"

namespace eventual
{
    namespace detail
    {
        template<typename T>
        class ResultBlock
        {
            using storage_t = std::aligned_storage_t<sizeof(T), alignof(T)>;
            using result_pointer_t = std::unique_ptr<T, PlacementDestructor<T>>;

        public:

            ResultBlock() = default;

            ResultBlock(ResultBlock&&) = delete;
            ResultBlock(const ResultBlock&) = delete;
            ResultBlock& operator=(ResultBlock&&) = delete;
            ResultBlock& operator=(const ResultBlock&) = delete;

            template<typename R>
            void Set(R&& result);

            T get();
            const T& get() const;

        private:
            storage_t _data;
            result_pointer_t _result;
        };
    }
}
