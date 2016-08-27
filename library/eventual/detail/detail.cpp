#pragma once

#include "detail.h"

#include <memory>
#include <tuple>

#include "../future.h"
#include "../eventual_methods.h"
#include "memory_resource.h"
#include "resource_adapter.h"

namespace eventual
{
    namespace detail
    {
        class default_resource_singleton
        {
        public:
            static memory_resource* new_delete_resource_singleton() noexcept
            {
                static resource_adapter<std::allocator<char>> instance;

                return &instance;
            }
        };

        memory_resource* get_default_resource() noexcept
        {
            return default_resource_singleton::new_delete_resource_singleton();
        }

        future<std::tuple<>> When_All_()
        {
            return make_ready_future(std::tuple<>());
        }
    }
}
