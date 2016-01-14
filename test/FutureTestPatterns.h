#pragma once
#include <memory>
#include <exception>
#include <vector>
#include <gtest/gtest.h>
#include <eventual\eventual.h>
#include "BasicAllocator.h"

class FutureTestException { };

// test fixture
template<typename T>
class FutureTestPatterns : public testing::Test
{
   template<class R>
   struct get_inner_type;

   template<class R>
   struct get_inner_type<eventual::Future<R>>
   {
      typedef R InnerType;
   };

   template<class R>
   struct get_inner_type<eventual::Shared_Future<R>>
   {
      typedef R InnerType;
   };

   struct Anonymous { };

protected:
   using future_t = T;
   using InnerType = typename get_inner_type<T>::InnerType;

   template<class Ex>
   static T MakeExceptionalFuture(Ex exception)
   {
      return T(eventual::Make_Exceptional_Future<InnerType>(std::make_exception_ptr(exception)));
   }

   template<class Allocator>
   static T MakeCompleteFuture(const Allocator& alloc)
   {
      auto promise = eventual::Promise<InnerType>(std::allocator_arg_t(), alloc);
      promise.Set_Exception(std::make_exception_ptr(Anonymous()));

      return T(promise.Get_Future());
   }

   T MakeIncompleteFuture()
   {
      _incompletePromises.emplace_back(eventual::Promise<InnerType>());
      return T(_incompletePromises.back().Get_Future());
   }

   static T MakeCompleteFuture()
   {
      return T(MakeExceptionalFuture(Anonymous()));
   }

   static eventual::Promise<InnerType> MakePromise()
   {
      return eventual::Promise<InnerType>();
   }

   static void CompletePromise(eventual::Promise<InnerType>& promise)
   {
      promise.Set_Exception(nullptr);
   }

private:
   std::vector<eventual::Promise<InnerType>> _incompletePromises;

};

TYPED_TEST_CASE_P(FutureTestPatterns);

TYPED_TEST_P(FutureTestPatterns, DefaultConstructorCreatesInvalidFuture)
{
   // Arrange/Act
   future_t future;

   // Assert
   EXPECT_FALSE(future.Valid()) << "Future should not be valid after default construction.";
}

TYPED_TEST_P(FutureTestPatterns, IsMoveConstructable)
{
   // Arrange
   auto future = MakeCompleteFuture();

   // Act
   future_t moved(std::move(future));

   // Assert
   EXPECT_FALSE(future.Valid()) << "Original future should be invalid after move.";
   EXPECT_TRUE(moved.Valid()) << "Moved constructed future should valid";
}

TYPED_TEST_P(FutureTestPatterns, ReleasesSharedStateOnDestruction)
{
   // Arrange
   auto alloc = BasicAllocator<void>();
   {
      auto future = MakeCompleteFuture(alloc);
   } // Act

   // Assert
   EXPECT_EQ(0, alloc.GetCount()) << "Future failed to release shared state on destruction.";
}

TYPED_TEST_P(FutureTestPatterns, IsMoveAssignable)
{
   // Arrange
   auto future = MakeCompleteFuture();

   // Act
   auto moved = std::move(future);

   // Assert
   EXPECT_FALSE(future.Valid()) << "Original future should be invalid after move.";
   EXPECT_TRUE(moved.Valid()) << "Moved future should valid";
}

TYPED_TEST_P(FutureTestPatterns, Get_ThrowsWhenFutureIsExceptional)
{
   // Arrange
   auto future = MakeExceptionalFuture(FutureTestException());

   // Act/Assert
   EXPECT_THROW(future.Get(), FutureTestException) << "Get did not throw the expected exception for the exceptional state";
}

TYPED_TEST_P(FutureTestPatterns, Get_ThrowsWhenFutureIsInvalid)
{
   // Arrange
   future_t future;

   // Act/Assert
   EXPECT_THROW(future.Get(), std::future_error) << "Get did not throw an exception when the future state was invalid";
}

TYPED_TEST_P(FutureTestPatterns, Wait_DoesNotBlockWhenFutureIsComplete)
{
   using namespace std;

   condition_variable waiting;
   mutex m;
   bool waitComplete = false;

   thread waiter([&, this]()
   {
      // Arrange
      auto future = MakeCompleteFuture();

      // Act
      future.Wait();

      lock_guard<mutex> l(m);
      waitComplete = true;
      waiting.notify_all();
   });

   // Assert
   unique_lock<mutex> lock(m);
   auto result = waiting.wait_for(lock, chrono::seconds(30), [&waitComplete]() { return waitComplete; });
   waiter.join();
   EXPECT_TRUE(result) << "Future::Wait blocked on an already complete Future.";
}

TYPED_TEST_P(FutureTestPatterns, Wait_BlocksUntilFutureIsComplete)
{
   using namespace std;

   // Arrange
   auto promise = MakePromise();
   auto future = future_t(promise.Get_Future());
   bool waitStarted = false;
   bool waitComplete = false;
   condition_variable waiting;
   mutex m;

   // Act
   thread waiter([&]()
   {
      unique_lock<mutex> l(m);
      waitStarted = true;
      l.unlock();
      waiting.notify_all();
      future.Wait();

      l.lock();
      waitComplete = true;
   });

   unique_lock<mutex> lock(m);
   waiting.wait(lock, [&waitStarted]() { return waitStarted; });
   lock.unlock();

   // Assert
   EXPECT_FALSE(waitComplete) << "Future::Wait did not wait for Promise completion.";
   CompletePromise(promise);
   waiter.join();
   EXPECT_TRUE(waitComplete);
}

TYPED_TEST_P(FutureTestPatterns, Wait_ThrowsWhenFutureIsInvalid)
{
   // Arrange
   future_t future;

   // Act/Assert
   EXPECT_THROW(future.Wait(), std::future_error) << "Wait did not throw an exception when the future state was invalid";
}

TYPED_TEST_P(FutureTestPatterns, WaitFor_DoesNotBlockWhenFutureIsComplete)
{
   using namespace std;

   condition_variable waiting;
   mutex m;
   bool waitComplete = false;
   eventual::future_status waitStatus = eventual::future_status::timeout;

   thread waiter([&]()
   {
      // Arrange
      auto future = MakeCompleteFuture();

      // Act
      waitStatus = future.Wait_For(chrono::seconds(100));

      lock_guard<mutex> l(m);
      waitComplete = true;
      waiting.notify_all();
   });

   // Assert
   unique_lock<mutex> lock(m);
   auto result = waiting.wait_for(lock, chrono::seconds(30), [&waitComplete]() { return waitComplete; });
   waiter.join();
   EXPECT_TRUE(result) << "Future::Wait_for blocked on an already complete Future.";
   EXPECT_EQ(eventual::future_status::ready, waitStatus);
}

TYPED_TEST_P(FutureTestPatterns, WaitFor_BlocksUntilTimeout)
{
   using namespace std;

   // Arrange
   auto promise = MakePromise();
   auto future = future_t(promise.Get_Future());
   condition_variable waiting;
   mutex m;
   bool waitComplete = false;
   eventual::future_status waitStatus = eventual::future_status::ready;

   thread waiter([&]()
   {
      // Act
      waitStatus = future.Wait_For(chrono::milliseconds(100));

      lock_guard<mutex> l(m);
      waitComplete = true;
      waiting.notify_all();
   });

   // Assert
   unique_lock<mutex> lock(m);
   auto result = waiting.wait_for(lock, chrono::seconds(1), [&waitComplete]() { return waitComplete; });
   waiter.join();
   EXPECT_EQ(eventual::future_status::timeout, waitStatus) << "Future::Wait_for failed to timeout.";
}

TYPED_TEST_P(FutureTestPatterns, WaitFor_ThrowsWhenFutureIsInvalid)
{
   // Arrange
   future_t future;

   // Act/Assert
   EXPECT_THROW(future.Wait_For(std::chrono::milliseconds(100)), std::future_error) << "Wait_For did not throw an exception when the future state was invalid";
}

TYPED_TEST_P(FutureTestPatterns, WaitUntil_DoesNotBlockWhenFutureIsComplete)
{
   using namespace std;

   condition_variable waiting;
   mutex m;
   bool waitComplete = false;
   eventual::future_status waitStatus = eventual::future_status::timeout;

   thread waiter([&]()
   {
      // Arrange
      auto future = MakeCompleteFuture();

      // Act
      waitStatus = future.Wait_Until(chrono::steady_clock::now() + chrono::seconds(100));

      lock_guard<mutex> l(m);
      waitComplete = true;
      waiting.notify_all();
   });

   // Assert
   unique_lock<mutex> lock(m);
   auto result = waiting.wait_for(lock, chrono::seconds(30), [&waitComplete]() { return waitComplete; });
   waiter.join();
   EXPECT_TRUE(result) << "Future::Wait_Until blocked on an already complete Future.";
   EXPECT_EQ(eventual::future_status::ready, waitStatus);
}

TYPED_TEST_P(FutureTestPatterns, WaitUntil_BlocksUntilTimeout)
{
   using namespace std;

   // Arrange
   auto promise = MakePromise();
   auto future = future_t(promise.Get_Future());
   condition_variable waiting;
   mutex m;
   bool waitComplete = false;
   eventual::future_status waitStatus = eventual::future_status::ready;

   thread waiter([&]()
   {
      // Act
      waitStatus = future.Wait_Until(chrono::steady_clock::now() + chrono::milliseconds(100));

      lock_guard<mutex> l(m);
      waitComplete = true;
      waiting.notify_all();
   });

   // Assert
   unique_lock<mutex> lock(m);
   auto result = waiting.wait_for(lock, chrono::seconds(1), [&waitComplete]() { return waitComplete; });
   waiter.join();
   EXPECT_EQ(eventual::future_status::timeout, waitStatus) << "Future::Wait_Until failed to timeout.";
}

TYPED_TEST_P(FutureTestPatterns, WaitUntil_ThrowsWhenFutureIsInvalid)
{
   // Arrange
   future_t future;

   // Act/Assert
   EXPECT_THROW(future.Wait_Until(std::chrono::steady_clock::now() + std::chrono::milliseconds(100)), std::future_error) << "Wait_Until did not throw an exception when the future state was invalid";
}

TYPED_TEST_P(FutureTestPatterns, IsReady_ReturnsTrueWhenFutureIsComplete)
{
   // Arrange
   auto future = MakeCompleteFuture();

   // Act
   auto result = future.Is_Ready();

   // Assert
   EXPECT_TRUE(result) << "Is_Ready returned the wrong result for an complete future.";
}

TYPED_TEST_P(FutureTestPatterns, IsReady_ReturnsFalseWhenFutureIsIncomplete)
{
   // Arrange
   auto future = MakeIncompleteFuture();

   // Act
   auto result = future.Is_Ready();

   // Assert
   EXPECT_FALSE(result) << "Is_Ready returned the wrong result for an incomplete future.";
}

TYPED_TEST_P(FutureTestPatterns, IsReady_ReturnsFalseWhenFutureIsInvalid)
{
   // Arrange
   future_t future;

   // Act
   auto result = future.Is_Ready();

   // Assert
   EXPECT_FALSE(result) << "Is_Ready returned the wrong result for an invalid future.";
}

TYPED_TEST_P(FutureTestPatterns, Then_CallsContinuationWhenExceptionallyComplete)
{
   // Arrange
   auto future = MakeExceptionalFuture(FutureTestException());

   // Act
   future.Then([](auto& f)
   {
      // Assert
      EXPECT_THROW(f.Get(), FutureTestException);
   });
}

// a test for a specific deadlock condition...
TYPED_TEST_P(FutureTestPatterns, Then_CallsContinuationAfterExceptionallyComplete)
{
   // Arrange
   auto promise = MakePromise();
   auto future = future_t(promise.Get_Future());

   // Act
   future.Then([](auto& f)
   {
      // Assert
      EXPECT_THROW(f.Get(), FutureTestException);
   });
   promise.Set_Exception(std::make_exception_ptr(FutureTestException()));
}

TYPED_TEST_P(FutureTestPatterns, Then_ThrowsWhenFutureIsInvalid)
{
   // Arrange
   future_t future;

   // Act/Assert
   EXPECT_THROW(future.Then([](auto& f) { }), std::future_error) << "Future::Then allowed registering a continuation on an invalid Future.";
}

TYPED_TEST_P(FutureTestPatterns, Then_ReturnsFutureWithCapturedExceptionOfContinuation)
{
   // Arrange
   auto future = MakeCompleteFuture();

   // Act
   auto continuation = future.Then([](auto& f)
   {
      throw FutureTestException();
   });

   // Assert
   EXPECT_THROW(continuation.Get(), FutureTestException) << "Continuation did not capture the unhandled exception.";
}

TYPED_TEST_P(FutureTestPatterns, Then_ReturnsFutureWithCompletedNestedFuturesException)
{
   // Arrange
   auto future = MakeExceptionalFuture(FutureTestException());

   // Act
   auto unwrapped = future.Then([this](auto& f)
   {
      return MakeExceptionalFuture(FutureTestException());
   });

   // Assert
   EXPECT_THROW(unwrapped.Get(), FutureTestException) << "Future::Then failed to unwrap a nested future and return its exception.";
}

TYPED_TEST_P(FutureTestPatterns, Then_ReturnsFutureWithNestedFuturesExceptionWhenComplete)
{
   // Arrange
   Promise<void> promise;
   auto outerFuture = promise.Get_Future();

   // Act
   auto unwrapped = outerFuture.Then([](auto& f)
   {
      return MakeExceptionalFuture(FutureTestException());
   });
   promise.Set_Value();

   // Assert
   EXPECT_THROW(unwrapped.Get(), FutureTestException) << "Future::Then failed to unwrap a nested future and return its exception.";
}

// ignore VS intellisense errors...
REGISTER_TYPED_TEST_CASE_P(FutureTestPatterns,
                           DefaultConstructorCreatesInvalidFuture, 
                           IsMoveConstructable,
                           ReleasesSharedStateOnDestruction,
                           IsMoveAssignable,
                           Get_ThrowsWhenFutureIsExceptional,
                           Get_ThrowsWhenFutureIsInvalid,
                           Wait_DoesNotBlockWhenFutureIsComplete,
                           Wait_BlocksUntilFutureIsComplete,
                           Wait_ThrowsWhenFutureIsInvalid,
                           WaitFor_DoesNotBlockWhenFutureIsComplete,
                           WaitFor_BlocksUntilTimeout,
                           WaitFor_ThrowsWhenFutureIsInvalid,
                           WaitUntil_DoesNotBlockWhenFutureIsComplete,
                           WaitUntil_BlocksUntilTimeout,
                           WaitUntil_ThrowsWhenFutureIsInvalid,
                           IsReady_ReturnsTrueWhenFutureIsComplete,
                           IsReady_ReturnsFalseWhenFutureIsIncomplete,
                           IsReady_ReturnsFalseWhenFutureIsInvalid,
                           Then_CallsContinuationWhenExceptionallyComplete,
                           Then_CallsContinuationAfterExceptionallyComplete,
                           Then_ThrowsWhenFutureIsInvalid,
                           Then_ReturnsFutureWithCapturedExceptionOfContinuation,
                           Then_ReturnsFutureWithCompletedNestedFuturesException,
                           Then_ReturnsFutureWithNestedFuturesExceptionWhenComplete);
