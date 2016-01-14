// sample.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <string>
#include <cassert>
#include <vector>
#include <tuple>
#include <exception>
#include "eventual\eventual.h"

using namespace eventual;

template<class T>
void DoSomething(T& future)
{
   // do nothing
}

template<class T>
class Continuation
{
public:
   void operator()(T& future) { }
};

void Sample_BasicContinuation()
{
   Future<int> future1 = Make_Ready_Future(3);
   future1.Then([](Future<int>& f)
   {
      //immediately calls this lambda
   });

   // or call a pointer
   Future<std::string> future2 = Make_Ready_Future(std::string("something"));
   future2.Then(&DoSomething<Future<std::string>>);

   // or a functor
   Future<void> future3 = Make_Ready_Future();
   future3.Then(Continuation<Future<void>>());

   // promises control when the continuation is called:
   int refValue = 17;
   Promise<int&> promise;
   Future<int&> future4 = promise.Get_Future();
   future4.Then([](Future<int&> f)
   {
      // not invoked until Promise<>::Set_Value(...) is called.
   });
   // ... do other stuff
   promise.Set_Value(refValue);

}

void Chaining_Continuations()
{
   // it gets really interesting when you link together continuations:

   Future<int> future = Make_Ready_Future(14);
   future
      .Then([](Future<int>& f) { return 1.1f; })
      .Then([](Future<float>& f) { return std::string(); })
      .Then([](Future<std::string>& f)
         {
            // nested futures are implicitly unwrapped
            // so Future<Future<void>> becomes Future<void>
            // when returned from .Then()
            return Make_Ready_Future();
         })
      .Then([](Future<void>& f) { });

   // of course modern style type deduction is supported

   auto future2 = Make_Ready_Future(13);
   future2
      .Then([](auto& f) { return 1.5; })
      .Then([](auto& f) { return "something..."; })
      .Then([](auto& f) { return std::tuple<>(); })
      .Then([](auto& f) { return 0; });

}

void Examples_When_Any()
{
   Promise<int> first;
   Promise<int> second;
   Promise<int> third;

   auto resultFuture = When_Any(first.Get_Future(), second.Get_Future(), third.Get_Future());
   second.Set_Value(1221);

   auto result = resultFuture.Get();

   // the veradic future result returns the index that completed.
   assert(result.index == 1);

   Promise<int> fourth;
   Promise<int> fifth;
   Promise<int> sixth;
   std::vector<Future<int>> futures;
   futures.emplace_back(fourth.Get_Future());
   futures.emplace_back(fifth.Get_Future());
   futures.emplace_back(sixth.Get_Future());
   
   auto resultFuture2 = When_Any(futures.begin(), futures.end());
   sixth.Set_Value(1818);

   auto result2 = resultFuture2.Get();
   
   // the iterator future result returns the index that completed.
   assert(result2.index == 2);
}

void Examples_When_All()
{
   Promise<int> first;
   Promise<int> second;
   Promise<int> third;

   auto resultFuture = When_All(first.Get_Future(), second.Get_Future(), third.Get_Future());
   first.Set_Value(123);
   second.Set_Value(234);
   third.Set_Value(345);

   // result completes when all futures are complete, returning a tuple of futures.
   auto result = resultFuture.Get();
   assert(std::get<0>(result).Get() == 123);
   assert(std::get<1>(result).Get() == 234);
   assert(std::get<2>(result).Get() == 345);

   Promise<int> fourth;
   Promise<int> fifth;
   Promise<int> sixth;
   std::vector<Future<int>> futures;
   futures.emplace_back(fourth.Get_Future());
   futures.emplace_back(fifth.Get_Future());
   futures.emplace_back(sixth.Get_Future());

   auto resultFuture2 = When_All(futures.begin(), futures.end());
   fourth.Set_Value(456);
   fifth.Set_Value(567);
   sixth.Set_Value(678);

   // result completes when all futures are complete, returning a vector of futures.
   auto result2 = resultFuture2.Get();
   assert(result2.at(0).Get() == 456);
   assert(result2.at(1).Get() == 567);
   assert(result2.at(2).Get() == 678);
}

void Examples_Make_Ready_Future()
{
   auto future = Make_Ready_Future<int>(35219);

   assert(future.Get() == 35219);
}

void Examples_Make_Exceptional_Future()
{
   struct Anonymous { };
   auto future = Make_Exceptional_Future<int, Anonymous>(Anonymous());

   try
   {
      future.Get();
   }
   catch (const Anonymous&)
   {
      // Exception expected
   }

   auto exPtr = std::make_exception_ptr(Anonymous());
   auto future2 = Make_Exceptional_Future<int>(exPtr);

   try
   {
      future2.Get();
   }
   catch (const Anonymous&)
   {
      // Exception expected
   }
}

int _tmain(int argc, _TCHAR* argv[])
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
