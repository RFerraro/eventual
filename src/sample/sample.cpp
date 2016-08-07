// sample.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <string>
#include <cassert>
#include <vector>
#include <tuple>
#include <exception>
#include "eventual/eventual.h"

using namespace eventual;

template<class T>
void DoSomething(T)
{
   // do nothing
}

template<class T>
class Continuation
{
public:
   void operator()(T) { }
};

void Sample_BasicContinuation()
{
   future<int> future1 = make_ready_future(3);
   future1.then([](future<int> f)
   {
      //immediately calls this lambda
       f.get();
   });

   // or call a pointer
   future<std::string> future2 = make_ready_future(std::string("something"));
   future2.then(&DoSomething<future<std::string>>);

   // or a functor
   future<void> future3 = make_ready_future();
   future3.then(Continuation<future<void>>());

   // promises control when the continuation is called:
   int refValue = 17;
   promise<int&> promise;
   future<int&> future4 = promise.get_future();
   future4.then([](future<int&> f)
   {
      // not invoked until Promise<>::set_value(...) is called.
      f.get();
   });
   // ... do other stuff
   promise.set_value(refValue);
}

void Chaining_Continuations()
{
   // it gets really interesting when you link together continuations:

   future<int> future1 = make_ready_future(14);
   future1
      .then([](future<int> f) { f.get(); return 1.1f; })
      .then([](future<float> f) { f.get(); return std::string(); })
      .then([](future<std::string> f)
         {
             f.get();
             
             // nested futures are implicitly unwrapped
            // so Future<Future<void>> becomes Future<void>
            // when returned from .then()
            return make_ready_future();
         })
      .then([](future<void> f) { f.get(); });

   // of course modern style type deduction is supported

   auto future2 = make_ready_future(13);
   future2
      .then([](auto f) { f.get(); return 1.5; })
      .then([](auto f) { f.get(); return "something..."; })
      .then([](auto f) { f.get(); return std::tuple<>(); })
      .then([](auto f) { f.get(); return 0; })
      .then([](auto& f) { f.get(); return 0; })
      .then([](const auto& f) { f.valid(); return 0; })
      .then([](auto&& f) { f.get(); return 0; });

}

void Examples_When_Any()
{
    promise<int> first;
    promise<int> second;
    promise<int> third;

   auto resultFuture = when_any(first.get_future(), second.get_future(), third.get_future());
   second.set_value(1221);

   auto result = resultFuture.get();

   // the veradic future result returns the index that completed.
   assert(result.index == 1);

   promise<int> fourth;
   promise<int> fifth;
   promise<int> sixth;
   std::vector<future<int>> futures;
   futures.emplace_back(fourth.get_future());
   futures.emplace_back(fifth.get_future());
   futures.emplace_back(sixth.get_future());
   
   auto resultFuture2 = when_any(futures.begin(), futures.end());
   sixth.set_value(1818);

   auto result2 = resultFuture2.get();
   
   // the iterator future result returns the index that completed.
   assert(result2.index == 2);
}

void Examples_When_All()
{
   promise<int> first;
   promise<int> second;
   promise<int> third;

   auto resultFuture = when_all(first.get_future(), second.get_future(), third.get_future());
   first.set_value(123);
   second.set_value(234);
   third.set_value(345);

   // result completes when all futures are complete, returning a tuple of futures.
   auto result = resultFuture.get();
   assert(std::get<0>(result).get() == 123);
   assert(std::get<1>(result).get() == 234);
   assert(std::get<2>(result).get() == 345);

   promise<int> fourth;
   promise<int> fifth;
   promise<int> sixth;
   std::vector<future<int>> futures;
   futures.emplace_back(fourth.get_future());
   futures.emplace_back(fifth.get_future());
   futures.emplace_back(sixth.get_future());

   auto resultFuture2 = when_all(futures.begin(), futures.end());
   fourth.set_value(456);
   fifth.set_value(567);
   sixth.set_value(678);

   // result completes when all futures are complete, returning a vector of futures.
   auto result2 = resultFuture2.get();
   assert(result2.at(0).get() == 456);
   assert(result2.at(1).get() == 567);
   assert(result2.at(2).get() == 678);
}

void Examples_Make_Ready_Future()
{
   auto future = make_ready_future<int>(35219);

   assert(future.get() == 35219);
}

void Examples_Make_Exceptional_Future()
{
   struct Anonymous { };
   auto future = make_exceptional_future<int, Anonymous>(Anonymous());

   try
   {
      future.get();
   }
   catch (const Anonymous&)
   {
      // Exception expected
   }

   auto exPtr = std::make_exception_ptr(Anonymous());
   auto future2 = make_exceptional_future<int>(exPtr);

   try
   {
      future2.get();
   }
   catch (const Anonymous&)
   {
      // Exception expected
   }
}

int main()
{
   try
   {
      Sample_BasicContinuation();
      Chaining_Continuations();
      Examples_When_Any();
      Examples_When_All();
      Examples_Make_Ready_Future();
      Examples_Make_Exceptional_Future();
   }
   catch (...)
   {
      return 1;
   }

   return 0;
}
