#pragma once

namespace eventual
{
    template<class TResult> class future;
    
    namespace detail
    {
        template<class TResult> class CommonPromise;
        
        class FutureFactory
        {
        public:
            template<class TResult>
            static future<TResult> Create(const CommonPromise<TResult>& promise);
        };
    }
}