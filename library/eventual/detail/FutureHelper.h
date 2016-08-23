#pragma once

namespace eventual
{
    namespace detail
    {
        template<class TResult> class BasicFuture;
        template<class TResult> class BasicPromise;
        
        class FutureHelper
        {
        public:

            template<class TResult, class T>
            static void SetCallback(BasicFuture<TResult>& future, T&& callback);

            template<class TResult, class T>
            static bool TrySetValue(BasicPromise<TResult>& promise, const T& value);

            template<class TResult>
            static decltype(auto) GetState(BasicFuture<TResult>& future);

            template<class TResult>
            static decltype(auto) GetResult(BasicFuture<TResult>& future);
        };
    }
}
