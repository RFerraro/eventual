#if defined(EVENTUAL_HEADER_ONLY)
#pragma once
#endif

#include "utility.h"

#include <future>
#include <exception>

namespace eventual
{
    namespace detail
    {
        std::future_error CreateFutureError(std::future_errc error)
        {
            auto code = make_error_code(error);
            return std::future_error(code);
        }

        std::exception_ptr CreateFutureExceptionPtr(std::future_errc error)
        {
            std::exception_ptr ptr;
            try
            {
                throw CreateFutureError(error);
            }
            catch (const std::future_error&)
            {
                ptr = std::current_exception();
            }
            return ptr;
        }
    }
}
