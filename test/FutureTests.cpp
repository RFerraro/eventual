#include "stdafx.h"
#include <type_traits>
#include <memory>
#include <thread>
#include <chrono>
#include <atomic>
#include <eventual\eventual.h>
#include "FutureTestPatterns.h"
#include "NonCopyable.h"

using namespace eventual;

namespace
{
   class TestException { };

   template<class R>
   Future<R> MakeCompleteFuture()
   {
      struct Anonymous { };
      return eventual::Make_Exceptional_Future<R>(std::make_exception_ptr(Anonymous()));
   }
}

// Parameterized Typed Tests
typedef testing::Types<Future<void>, Future<int>, Future<int&>> FutureTypes;
INSTANTIATE_TYPED_TEST_CASE_P(Future, FutureTestPatterns, FutureTypes);

// Typed Tests
template<typename T>
class FutureTest : public testing::Test { };
typedef testing::Types<void, int, int&> FutureReturnTypes;
TYPED_TEST_CASE(FutureTest, FutureReturnTypes); // ignore intellisense warning

// questionable value...
TYPED_TEST(FutureTest, Future_IsNotCopyConstructable)
{
   // Assert
   EXPECT_FALSE(std::is_copy_constructible<Future<TypeParam>>::value) << "Future should not be copy constructable.";
}

TYPED_TEST(FutureTest, Future_ConstructorUnwrapsNestedFutures)
{
   // Arrange
   Future<Future<TypeParam>> nested = MakeCompleteFuture<Future<TypeParam>>();

   // Act
   Future<TypeParam> unwrapped(std::move(nested));

   // Assert
   EXPECT_FALSE(nested.Valid()) << "nested future should not remain valid";
   EXPECT_TRUE(unwrapped.Valid()) << "Unwrapped future should be valid";
}

// questionable value...
TYPED_TEST(FutureTest, Future_IsNotCopyAssignable)
{
   // Assert
   EXPECT_FALSE(std::is_copy_assignable<Future<TypeParam>>::value) << "Future should not be copy assignable.";
}

TYPED_TEST(FutureTest, Share_InvalidatesFuture)
{
   // Arrange
   auto future = MakeCompleteFuture<TypeParam>();

   // Act
   Shared_Future<TypeParam> shared = future.Share();

   // Assert
   EXPECT_FALSE(future.Valid()) << "Original future should be invalid after transfer to shared.";
   EXPECT_TRUE(shared.Valid()) << "Shared future should valid";
}

TYPED_TEST(FutureTest, Share_ThrowsWhenInvalid)
{
   // Arrange
   Future<TypeParam> future;

   // Act/Assert
   EXPECT_THROW(future.Share(), std::future_error) << "Share did not throw an exception when the future state was invalid";
}

TYPED_TEST(FutureTest, Then_InvalidatesFuture)
{
   // Arrange
   auto future = MakeCompleteFuture<TypeParam>();

   // Act
   future.Then([](auto& f) { });

   // Assert
   EXPECT_FALSE(future.Valid()) << "Future::Then failed to invalidate the future.";
}

TYPED_TEST(FutureTest, Get_InvalidatesFuture)
{
   // Arrange
   auto future = MakeCompleteFuture<TypeParam>();

   // Act
   try
   {
      future.Get();
   }
   catch (...) { }

   // Assert
   EXPECT_FALSE(future.Valid()) << "Future::Get failed to invalidate the future.";
}

// Standard tests

TEST(FutureTest_Value, Get_ReturnsCorrectValueWhenReady)
{
   // Arrange
   int expected = 13;
   auto future = Make_Ready_Future(expected);

   // Act
   int actual = future.Get();

   // Assert
   EXPECT_EQ(expected, actual) << "The future did not return the expected value.";
}

TEST(FutureTest_RValue, Get_ReturnsCorrectRValueWhenReady)
{
   // Arrange
   int expectedMarker = 761;
   auto np = NonCopyable();
   np._markerValue = expectedMarker;
   Future<NonCopyable> future;
   
   {
      Promise<NonCopyable> promise;
      promise.Set_Value(std::move(np));
      future = promise.Get_Future();
   }

   // Act
   auto result = future.Get();

   // Assert
   EXPECT_EQ(expectedMarker, result._markerValue) << "The future did not return the expected value.";
}

TEST(FutureTest_Reference, Get_ReturnsCorrectReferenceWhenReady)
{
   // Arrange
   int expected = 13;
   auto expectedAddress = &expected;
   Promise<int&> promise;
   promise.Set_Value(expected);
   auto future = promise.Get_Future();

   // Act
   int& actual = future.Get();

   // Assert
   auto actualAddress = &actual;

   EXPECT_EQ(expectedAddress, actualAddress) << "The future did not return the expected reference address.";
}

TEST(FutureTest_Reference, Get_ReturnsCorrectReferenceAfterPromiseDeleted)
{
   // Arrange
   int expected = 18;
   auto expectedAddress = &expected;
   Future<int&> future;

   {
      Promise<int&> promise;
      promise.Set_Value(expected);
      future = promise.Get_Future();
   }

   // Act
   int& actual = future.Get();

   // Assert
   auto actualAddress = &actual;
   EXPECT_EQ(expectedAddress, actualAddress) << "The future did not return the expected reference address.";
}

TEST(FutureTest_Void, Get_ReturnsVoidWhenReady)
{
   // Arrange
   auto future = Make_Ready_Future();

   // Act
   EXPECT_NO_THROW(future.Get());

   // Assert (-ish, more of a compile-time constant)
   EXPECT_TRUE(std::is_void<std::result_of<decltype(&Future<void>::Get)(Future<void>)>::type>::value) << "Future<void>::Get() does not return void.";
}

TEST(FutureTest_Value, Get_BlocksUntilReady)
{
   // Arrange
   auto promise = Promise<int>();
   auto future = promise.Get_Future();
   std::atomic_bool actual(false);

   std::mutex m;
   std::condition_variable cv;
   bool ready = false;

   // Act
   std::thread worker([&]()
   {
      {
         std::unique_lock<std::mutex> l(m);
         ready = true;
      }
      cv.notify_all();
      
      future.Get();
      actual.store(true);
   });

   std::unique_lock<std::mutex> l(m);
   cv.wait(l, [&]() { return ready; });
   std::this_thread::sleep_for(std::chrono::milliseconds(500));

   EXPECT_FALSE(future.Is_Ready());
   EXPECT_FALSE(actual.load());
   promise.Set_Value(4);

   // Assert
   worker.join();
   EXPECT_TRUE(actual.load());
}

TEST(FutureTest_Reference, Get_BlocksUntilReady)
{
   // Arrange
   int expectedValue = 5;
   auto promise = Promise<int&>();
   auto future = promise.Get_Future();
   std::atomic_bool actual(false);

   std::mutex m;
   std::condition_variable cv;
   bool ready = false;

   // Act
   std::thread worker([&]()
   {
      {
         std::unique_lock<std::mutex> l(m);
         ready = true;
      }
      cv.notify_all();
      
      future.Get();
      actual.store(true);
   });

   std::unique_lock<std::mutex> l(m);
   cv.wait(l, [&]() { return ready; });
   std::this_thread::sleep_for(std::chrono::milliseconds(500));

   EXPECT_FALSE(future.Is_Ready());
   EXPECT_FALSE(actual.load());
   promise.Set_Value(expectedValue);

   // Assert
   worker.join();
   EXPECT_TRUE(actual.load());
}

TEST(FutureTest_Void, Get_BlocksUntilReady)
{
   // Arrange
   auto promise = Promise<void>();
   auto future = promise.Get_Future();
   std::atomic_bool actual(false);

   std::mutex m;
   std::condition_variable cv;
   bool ready = false;

   // Act
   std::thread worker([&]()
   {
      {
         std::unique_lock<std::mutex> l(m);
         ready = true;
      }
      cv.notify_all();
      
      future.Get();
      actual.store(true);
   });

   std::unique_lock<std::mutex> l(m);
   cv.wait(l, [&]() { return ready; });
   std::this_thread::sleep_for(std::chrono::milliseconds(500));

   EXPECT_FALSE(future.Is_Ready());
   EXPECT_FALSE(actual.load());
   promise.Set_Value();

   // Assert
   worker.join();
   EXPECT_TRUE(actual.load());
}

TEST(FutureTest_Value, Then_InvokesContinuationWhenComplete)
{
   // Arrange
   Promise<int> promise;
   auto future = promise.Get_Future();
   int expected = 12;
   int actual = -1;
   
   // Act
   future.Then([&actual](auto& f) { actual = f.Get(); });
   promise.Set_Value(expected);

   // Assert
   EXPECT_EQ(expected, actual) << "Future::Then failed to register a continuation that was called.";
}

TEST(FutureTest_Reference, Then_InvokesContinuationWhenComplete)
{
   // Arrange
   Promise<int&> promise;
   auto future = promise.Get_Future();
   int expected = 12;
   auto expectedAddress = &expected;
   int* actualAddress = nullptr;

   // Act
   future.Then([&actualAddress](auto& f)
   { 
      auto& temp = f.Get();
      actualAddress = &temp;
   });
   promise.Set_Value(expected);

   // Assert
   EXPECT_EQ(expectedAddress, actualAddress) << "Future::Then failed to register a continuation that was called.";
}

TEST(FutureTest_Void, Then_InvokesContinuationWhenComplete)
{
   // Arrange
   Promise<void> promise;
   auto future = promise.Get_Future();
   bool continuationCalled = false;

   // Act
   future.Then([&continuationCalled](auto& f)
   {
      f.Get();
      continuationCalled = true;
   });
   promise.Set_Value();

   // Assert
   EXPECT_TRUE(continuationCalled) << "Future::Then failed to register a continuation that was called.";
}

TEST(FutureTest_Value, Then_UnwrapsAndReturnsNestedResultWhenComplete)
{
   // Arrange
   Promise<int> promise;
   auto future = promise.Get_Future();
   int expected = 13;

   // Act
   auto unwrapped = future.Then([](auto& f)
   {
      Promise<int> inner;
      auto temp = f.Get() + 1;
      inner.Set_Value(temp);
      return inner.Get_Future();
   });
   promise.Set_Value(12);
   auto actual = unwrapped.Get();

   // Assert
   EXPECT_EQ(expected, actual) << "Future::Then failed to return the expected value of the unwrapped continuation.";
}

TEST(FutureTest_Reference, Then_UnwrapsAndReturnsNestedResultWhenComplete)
{
   // Arrange
   Promise<int&> promise;
   auto future = promise.Get_Future();
   int expected = 12;
   auto expectedAddress = &expected;

   // Act
   auto unwrapped = future.Then([](auto& f)
   {
      Promise<int&> inner;
      auto& temp = f.Get();
      inner.Set_Value(temp);
      return inner.Get_Future();
   });
   promise.Set_Value(expected);
   auto& actual = unwrapped.Get();
   auto actualAddress = &actual;

   // Assert
   EXPECT_EQ(expectedAddress, actualAddress) << "Future::Then failed to return the expected value of the unwrapped continuation.";
}

TEST(FutureTest_Void, Then_UnwrapsAndReturnsNestedVoidWhenComplete)
{
   // Arrange
   Promise<void> promise;
   auto future = promise.Get_Future();
   bool continuationCalled = false;

   // Act
   Future<void> unwrapped = future.Then([&continuationCalled](auto& f)
   {
      f.Get();
      continuationCalled = true;
      return Make_Ready_Future();
   });
   promise.Set_Value();
   unwrapped.Get();

   // Assert
   EXPECT_TRUE(continuationCalled) << "Future::Then failed to return the unwrapped continuation..";
}
