#pragma once
#include <memory>
#include <exception>
#include <vector>
#include <gtest/gtest.h>
#include <eventual/eventual.h>
#include "BasicAllocator.h"

class FutureTestException { };

// test fixture
template<typename T>
class FutureTestPatterns : public testing::Test
{
   template<class R>
   struct get_inner_type;

   template<class R>
   struct get_inner_type<eventual::future<R>>
   {
      typedef R InnerType;
   };

   template<class R>
   struct get_inner_type<eventual::shared_future<R>>
   {
      typedef R InnerType;
   };

   struct Anonymous { };

protected:
   using InnerType = typename get_inner_type<T>::InnerType;

   template<class Ex>
   static T MakeExceptionalFuture(Ex exception)
   {
      return T(eventual::make_exceptional_future<InnerType>(std::make_exception_ptr(exception)));
   }

   template<class Allocator>
   static T MakeCompleteFuture(const Allocator& alloc)
   {
      auto promise = eventual::promise<InnerType>(std::allocator_arg_t(), alloc);
      promise.set_exception(std::make_exception_ptr(Anonymous()));

      return T(promise.get_future());
   }

   T MakeIncompleteFuture()
   {
      _incompletePromises.emplace_back(eventual::promise<InnerType>());
      return T(_incompletePromises.back().get_future());
   }

   static T MakeCompleteFuture()
   {
      return T(MakeExceptionalFuture(Anonymous()));
   }

   static eventual::promise<InnerType> MakePromise()
   {
      return eventual::promise<InnerType>();
   }

   static void CompletePromise(eventual::promise<InnerType>& promise)
   {
      promise.set_exception(nullptr);
   }

private:
   std::vector<eventual::promise<InnerType>> _incompletePromises;

};

TYPED_TEST_CASE_P(FutureTestPatterns);

TYPED_TEST_P(FutureTestPatterns, DefaultConstructorCreatesInvalidFuture)
{
   // Arrange/Act
   TypeParam future;

   // Assert
   EXPECT_FALSE(future.valid()) << "Future should not be valid after default construction.";
}

TYPED_TEST_P(FutureTestPatterns, IsMoveConstructable)
{
   // Arrange
   auto future = this->MakeCompleteFuture();

   // Act
   TypeParam moved(std::move(future));

   // Assert
   EXPECT_FALSE(future.valid()) << "Original future should be invalid after move.";
   EXPECT_TRUE(moved.valid()) << "Moved constructed future should valid";
}

TYPED_TEST_P(FutureTestPatterns, ReleasesSharedStateOnDestruction)
{
   // Arrange
   auto alloc = BasicAllocator<void>();
   {
      auto future = this->MakeCompleteFuture(alloc);
   } // Act

   // Assert
   EXPECT_EQ(0, alloc.GetCount()) << "Future failed to release shared state on destruction.";
}

TYPED_TEST_P(FutureTestPatterns, IsMoveAssignable)
{
   // Arrange
   auto future = this->MakeCompleteFuture();

   // Act
   auto moved = std::move(future);

   // Assert
   EXPECT_FALSE(future.valid()) << "Original future should be invalid after move.";
   EXPECT_TRUE(moved.valid()) << "Moved future should valid";
}

TYPED_TEST_P(FutureTestPatterns, Get_ThrowsWhenFutureIsExceptional)
{
   // Arrange
   auto future = this->MakeExceptionalFuture(FutureTestException());

   // Act/Assert
   EXPECT_THROW(future.get(), FutureTestException) << "get did not throw the expected exception for the exceptional state";
}

TYPED_TEST_P(FutureTestPatterns, Get_ThrowsWhenFutureIsInvalid)
{
   // Arrange
   TypeParam future;

   // Act/Assert
   EXPECT_THROW(future.get(), std::future_error) << "get did not throw an exception when the future state was invalid";
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
      auto future = this->MakeCompleteFuture();

      // Act
      future.wait();

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
   auto promise = this->MakePromise();
   TypeParam future(promise.get_future());
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
      future.wait();

      l.lock();
      waitComplete = true;
   });

   unique_lock<mutex> lock(m);
   waiting.wait(lock, [&waitStarted]() { return waitStarted; });
   lock.unlock();

   // Assert
   EXPECT_FALSE(waitComplete) << "Future::Wait did not wait for Promise completion.";
   this->CompletePromise(promise);
   waiter.join();
   EXPECT_TRUE(waitComplete);
}

TYPED_TEST_P(FutureTestPatterns, Wait_ThrowsWhenFutureIsInvalid)
{
   // Arrange
   TypeParam future;

   // Act/Assert
   EXPECT_THROW(future.wait(), std::future_error) << "Wait did not throw an exception when the future state was invalid";
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
      auto future = this->MakeCompleteFuture();

      // Act
      waitStatus = future.wait_for(chrono::seconds(100));

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
   auto promise = this->MakePromise();
   TypeParam future(promise.get_future());
   condition_variable waiting;
   mutex m;
   bool waitComplete = false;
   eventual::future_status waitStatus = eventual::future_status::ready;

   thread waiter([&]()
   {
      // Act
      waitStatus = future.wait_for(chrono::milliseconds(100));

      lock_guard<mutex> l(m);
      waitComplete = true;
      waiting.notify_all();
   });

   // Assert
   unique_lock<mutex> lock(m);
   waiting.wait_for(lock, chrono::seconds(1), [&waitComplete]() { return waitComplete; });
   waiter.join();
   EXPECT_EQ(eventual::future_status::timeout, waitStatus) << "Future::Wait_for failed to timeout.";
}

TYPED_TEST_P(FutureTestPatterns, WaitFor_ThrowsWhenFutureIsInvalid)
{
   // Arrange
   TypeParam future;

   // Act/Assert
   EXPECT_THROW(future.wait_for(std::chrono::milliseconds(100)), std::future_error) << "Wait_For did not throw an exception when the future state was invalid";
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
      auto future = this->MakeCompleteFuture();

      // Act
      waitStatus = future.wait_until(chrono::steady_clock::now() + chrono::seconds(100));

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
   auto promise = this->MakePromise();
   TypeParam future(promise.get_future());
   condition_variable waiting;
   mutex m;
   bool waitComplete = false;
   eventual::future_status waitStatus = eventual::future_status::ready;

   thread waiter([&]()
   {
      // Act
      waitStatus = future.wait_until(chrono::steady_clock::now() + chrono::milliseconds(100));

      lock_guard<mutex> l(m);
      waitComplete = true;
      waiting.notify_all();
   });

   // Assert
   unique_lock<mutex> lock(m);
   waiting.wait_for(lock, chrono::seconds(1), [&waitComplete]() { return waitComplete; });
   waiter.join();
   EXPECT_EQ(eventual::future_status::timeout, waitStatus) << "Future::Wait_Until failed to timeout.";
}

TYPED_TEST_P(FutureTestPatterns, WaitUntil_ThrowsWhenFutureIsInvalid)
{
   // Arrange
   TypeParam future;

   // Act/Assert
   EXPECT_THROW(future.wait_until(std::chrono::steady_clock::now() + std::chrono::milliseconds(100)), std::future_error) << "Wait_Until did not throw an exception when the future state was invalid";
}

TYPED_TEST_P(FutureTestPatterns, IsReady_ReturnsTrueWhenFutureIsComplete)
{
   // Arrange
   auto future = this->MakeCompleteFuture();

   // Act
   auto result = future.is_ready();

   // Assert
   EXPECT_TRUE(result) << "Is_Ready returned the wrong result for an complete future.";
}

TYPED_TEST_P(FutureTestPatterns, IsReady_ReturnsFalseWhenFutureIsIncomplete)
{
   // Arrange
   auto future = this->MakeIncompleteFuture();

   // Act
   auto result = future.is_ready();

   // Assert
   EXPECT_FALSE(result) << "Is_Ready returned the wrong result for an incomplete future.";
}

TYPED_TEST_P(FutureTestPatterns, IsReady_ReturnsFalseWhenFutureIsInvalid)
{
   // Arrange
   TypeParam future;

   // Act
   auto result = future.is_ready();

   // Assert
   EXPECT_FALSE(result) << "Is_Ready returned the wrong result for an invalid future.";
}

TYPED_TEST_P(FutureTestPatterns, Then_CallsContinuationWhenExceptionallyComplete)
{
   // Arrange
   auto future = this->MakeExceptionalFuture(FutureTestException());

   // Act
   future.then([](auto f)
   {
      // Assert
      EXPECT_THROW(f.get(), FutureTestException);
   });
}

// a test for a specific deadlock condition...
TYPED_TEST_P(FutureTestPatterns, Then_CallsContinuationAfterExceptionallyComplete)
{
   // Arrange
   auto promise = this->MakePromise();
   TypeParam future(promise.get_future());

   // Act
   future.then([](auto f)
   {
      // Assert
      EXPECT_THROW(f.get(), FutureTestException);
   });
   promise.set_exception(std::make_exception_ptr(FutureTestException()));
}

TYPED_TEST_P(FutureTestPatterns, Then_ThrowsWhenFutureIsInvalid)
{
   // Arrange
   TypeParam future;

   // Act/Assert
   EXPECT_THROW(future.then([](auto) { }), std::future_error) << "Future::then allowed registering a continuation on an invalid Future.";
}

TYPED_TEST_P(FutureTestPatterns, Then_ReturnsFutureWithCapturedExceptionOfContinuation)
{
   // Arrange
   auto future = this->MakeCompleteFuture();

   // Act
   auto continuation = future.then([](auto)
   {
      throw FutureTestException();
   });

   // Assert
   EXPECT_THROW(continuation.get(), FutureTestException) << "Continuation did not capture the unhandled exception.";
}

TYPED_TEST_P(FutureTestPatterns, Then_ReturnsFutureWithCompletedNestedFuturesException)
{
   // Arrange
   auto future = this->MakeExceptionalFuture(FutureTestException());

   // Act
   auto unwrapped = future.then([this](auto)
   {
      return this->MakeExceptionalFuture(FutureTestException());
   });

   // Assert
   EXPECT_THROW(unwrapped.get(), FutureTestException) << "Future::then failed to unwrap a nested future and return its exception.";
}

TYPED_TEST_P(FutureTestPatterns, Then_ReturnsFutureWithNestedFuturesExceptionWhenComplete)
{
   using namespace eventual;
   
   // Arrange
   promise<void> promise;
   auto outerFuture = promise.get_future();

   // Act
   auto unwrapped = outerFuture.then([this](auto)
   {
      return this->MakeExceptionalFuture(FutureTestException());
   });
   promise.set_value();

   // Assert
   EXPECT_THROW(unwrapped.get(), FutureTestException) << "Future::then failed to unwrap a nested future and return its exception.";
}

TYPED_TEST_P(FutureTestPatterns, Then_ReturnsFutureWithBrokenPromiseExceptionWhenInnerFutureHasNoState)
{
   using namespace eventual;
   
   // Arrange
   promise<void> promise;
   auto outerFuture = promise.get_future();

   // Act
   auto unwrapped = outerFuture.then([this](auto)
   {
      auto future = this->MakeCompleteFuture();
      TypeParam moved(std::move(future));
      
      return future;
   });
   promise.set_value();

   // Assert

   try
   {
      unwrapped.get();
      FAIL() << "Future::then failed to throw an exception";
   }
   catch (const future_error& e)
   {
      
      EXPECT_EQ(future_errc::broken_promise, e.code()) << "Future::then failed to unwrap a nested future and return a broken-promise exception.";
   }
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
                           Then_ReturnsFutureWithNestedFuturesExceptionWhenComplete,
                           Then_ReturnsFutureWithBrokenPromiseExceptionWhenInnerFutureHasNoState);
