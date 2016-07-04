#include "stdafx.h"
#include <type_traits>
#include <memory>
#include <thread>
#include <chrono>
#include <atomic>
#include <eventual/eventual.h>
#include "FutureTestPatterns.h"
#include "NonCopyable.h"

using namespace eventual;

namespace
{
   class TestException { };

   template<class R>
   future<R> MakeCompleteFuture()
   {
      struct Anonymous { };
      return eventual::make_exceptional_future<R>(std::make_exception_ptr(Anonymous()));
   }
}

// Parameterized Typed Tests
typedef testing::Types<future<void>, future<int>, future<int&>> FutureTypes;
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
   EXPECT_FALSE(std::is_copy_constructible<future<TypeParam>>::value) << "Future should not be copy constructable.";
}

TYPED_TEST(FutureTest, Future_ConstructorUnwrapsNestedFutures)
{
   // Arrange
   future<future<TypeParam>> nested = MakeCompleteFuture<future<TypeParam>>();

   // Act
   future<TypeParam> unwrapped(std::move(nested));

   // Assert
   EXPECT_FALSE(nested.valid()) << "nested future should not remain valid";
   EXPECT_TRUE(unwrapped.valid()) << "Unwrapped future should be valid";
}

// questionable value...
TYPED_TEST(FutureTest, Future_IsNotCopyAssignable)
{
   // Assert
   EXPECT_FALSE(std::is_copy_assignable<future<TypeParam>>::value) << "Future should not be copy assignable.";
}

TYPED_TEST(FutureTest, Share_InvalidatesFuture)
{
   // Arrange
   auto future = MakeCompleteFuture<TypeParam>();

   // Act
   shared_future<TypeParam> shared = future.share();

   // Assert
   EXPECT_FALSE(future.valid()) << "Original future should be invalid after transfer to shared.";
   EXPECT_TRUE(shared.valid()) << "Shared future should valid";
}

TYPED_TEST(FutureTest, Share_ThrowsWhenInvalid)
{
   // Arrange
   future<TypeParam> future;

   // Act/Assert
   EXPECT_THROW(future.share(), std::future_error) << "share did not throw an exception when the future state was invalid";
}

TYPED_TEST(FutureTest, Then_InvalidatesFuture)
{
   // Arrange
   auto future = MakeCompleteFuture<TypeParam>();

   // Act
   future.then([](auto) { });

   // Assert
   EXPECT_FALSE(future.valid()) << "Future::then failed to invalidate the future.";
}

TYPED_TEST(FutureTest, Get_InvalidatesFuture)
{
   // Arrange
   auto future = MakeCompleteFuture<TypeParam>();

   // Act
   try
   {
      future.get();
   }
   catch (...) { }

   // Assert
   EXPECT_FALSE(future.valid()) << "Future::get failed to invalidate the future.";
}

// Standard tests

TEST(FutureTest_Value, Get_ReturnsCorrectValueWhenReady)
{
   // Arrange
   int expected = 13;
   auto future = make_ready_future(expected);

   // Act
   int actual = future.get();

   // Assert
   EXPECT_EQ(expected, actual) << "The future did not return the expected value.";
}

TEST(FutureTest_RValue, Get_ReturnsCorrectRValueWhenReady)
{
   // Arrange
   int expectedMarker = 761;
   auto np = NonCopyable();
   np._markerValue = expectedMarker;
   future<NonCopyable> future;
   
   {
      promise<NonCopyable> promise;
      promise.set_value(std::move(np));
      future = promise.get_future();
   }

   // Act
   auto result = future.get();

   // Assert
   EXPECT_EQ(expectedMarker, result._markerValue) << "The future did not return the expected value.";
}

TEST(FutureTest_Reference, Get_ReturnsCorrectReferenceWhenReady)
{
   // Arrange
   int expected = 13;
   auto expectedAddress = &expected;
   promise<int&> promise;
   promise.set_value(expected);
   auto future = promise.get_future();

   // Act
   int& actual = future.get();

   // Assert
   auto actualAddress = &actual;

   EXPECT_EQ(expectedAddress, actualAddress) << "The future did not return the expected reference address.";
}

TEST(FutureTest_Reference, Get_ReturnsCorrectReferenceAfterPromiseDeleted)
{
   // Arrange
   int expected = 18;
   auto expectedAddress = &expected;
   future<int&> future;

   {
      promise<int&> promise;
      promise.set_value(expected);
      future = promise.get_future();
   }

   // Act
   int& actual = future.get();

   // Assert
   auto actualAddress = &actual;
   EXPECT_EQ(expectedAddress, actualAddress) << "The future did not return the expected reference address.";
}

TEST(FutureTest_Void, Get_ReturnsVoidWhenReady)
{
   // Arrange
   auto future = make_ready_future();

   // Act
   EXPECT_NO_THROW(future.get());

   // Assert (-ish, more of a compile-time constant)
   EXPECT_TRUE(std::is_void<std::result_of<decltype(&eventual::future<void>::get)(eventual::future<void>)>::type>::value) << "future<void>::get() does not return void.";
}

TEST(FutureTest_Value, Get_BlocksUntilReady)
{
   // Arrange
   promise<int> promise;
   auto future = promise.get_future();
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
      
      future.get();
      actual.store(true);
   });

   std::unique_lock<std::mutex> l(m);
   cv.wait(l, [&]() { return ready; });
   std::this_thread::sleep_for(std::chrono::milliseconds(500));

   EXPECT_FALSE(future.is_ready());
   EXPECT_FALSE(actual.load());
   promise.set_value(4);

   // Assert
   worker.join();
   EXPECT_TRUE(actual.load());
}

TEST(FutureTest_Reference, Get_BlocksUntilReady)
{
   // Arrange
   int expectedValue = 5;
   promise<int&> promise;
   auto future = promise.get_future();
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
      
      future.get();
      actual.store(true);
   });

   std::unique_lock<std::mutex> l(m);
   cv.wait(l, [&]() { return ready; });
   std::this_thread::sleep_for(std::chrono::milliseconds(500));

   EXPECT_FALSE(future.is_ready());
   EXPECT_FALSE(actual.load());
   promise.set_value(expectedValue);

   // Assert
   worker.join();
   EXPECT_TRUE(actual.load());
}

TEST(FutureTest_Void, Get_BlocksUntilReady)
{
   // Arrange
   promise<void> promise;
   auto future = promise.get_future();
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
      
      future.get();
      actual.store(true);
   });

   std::unique_lock<std::mutex> l(m);
   cv.wait(l, [&]() { return ready; });
   std::this_thread::sleep_for(std::chrono::milliseconds(500));

   EXPECT_FALSE(future.is_ready());
   EXPECT_FALSE(actual.load());
   promise.set_value();

   // Assert
   worker.join();
   EXPECT_TRUE(actual.load());
}

TEST(FutureTest_Value, Then_InvokesContinuation_ByValue_WhenComplete)
{
   // Arrange
   promise<int> promise;
   auto future = promise.get_future();
   int expected = 12;
   int actual = -1;
   
   // Act
   future.then([&actual](auto f) { actual = f.get(); });
   promise.set_value(expected);

   // Assert
   EXPECT_EQ(expected, actual) << "Future::then failed to register a continuation that was called.";
}

TEST(FutureTest_Value, Then_InvokesContinuation_ByLValueReference_WhenComplete)
{
    // Arrange
    promise<int> promise;
    auto future = promise.get_future();
    int expected = 12;
    int actual = -1;

    // Act
    future.then([&actual](auto& f) { actual = f.get(); });
    promise.set_value(expected);

    // Assert
    EXPECT_EQ(expected, actual) << "Future::then failed to register a continuation that was called.";
}

TEST(FutureTest_Value, Then_InvokesContinuation_ByConstLValueReference_WhenComplete)
{
    // Arrange
    promise<int> promise;
    auto future = promise.get_future();
    bool actualIsReady = false;

    // Act
    future.then([&actualIsReady](const auto& f) { actualIsReady = f.is_ready(); });
    promise.set_value(0);

    // Assert
    EXPECT_TRUE(actualIsReady) << "Future::then failed to register a continuation that was called.";
}

TEST(FutureTest_Value, Then_InvokesContinuation_ByRValueReference_WhenComplete)
{
    // Arrange
    promise<int> promise;
    auto future = promise.get_future();
    int expected = 12;
    int actual = -1;

    // Act
    future.then([&actual](auto&& f) { actual = f.get(); });
    promise.set_value(expected);

    // Assert
    EXPECT_EQ(expected, actual) << "Future::then failed to register a continuation that was called.";
}

TEST(FutureTest_Reference, Then_InvokesContinuationWhenComplete)
{
   // Arrange
   promise<int&> promise;
   auto future = promise.get_future();
   int expected = 12;
   auto expectedAddress = &expected;
   int* actualAddress = nullptr;

   // Act
   future.then([&actualAddress](auto f)
   { 
      auto& temp = f.get();
      actualAddress = &temp;
   });
   promise.set_value(expected);

   // Assert
   EXPECT_EQ(expectedAddress, actualAddress) << "Future::then failed to register a continuation that was called.";
}

TEST(FutureTest_Void, Then_InvokesContinuationWhenComplete)
{
   // Arrange
   promise<void> promise;
   auto future = promise.get_future();
   bool continuationCalled = false;

   // Act
   future.then([&continuationCalled](auto f)
   {
      f.get();
      continuationCalled = true;
   });
   promise.set_value();

   // Assert
   EXPECT_TRUE(continuationCalled) << "Future::then failed to register a continuation that was called.";
}

TEST(FutureTest_Value, Then_UnwrapsAndReturnsNestedResultWhenComplete)
{
   // Arrange
   eventual::promise<int> outer;
   auto future = outer.get_future();
   int expected = 13;

   // Act
   auto unwrapped = future.then([](auto f)
   {
      eventual::promise<int> inner;
      auto temp = f.get() + 1;
      inner.set_value(temp);
      return inner.get_future();
   });
   outer.set_value(12);
   auto actual = unwrapped.get();

   // Assert
   EXPECT_EQ(expected, actual) << "Future::then failed to return the expected value of the unwrapped continuation.";
}

TEST(FutureTest_Reference, Then_UnwrapsAndReturnsNestedResultWhenComplete)
{
   // Arrange
   eventual::promise<int&> promise;
   auto future = promise.get_future();
   int expected = 12;
   auto expectedAddress = &expected;

   // Act
   auto unwrapped = future.then([](auto f)
   {
      eventual::promise<int&> inner;
      auto& temp = f.get();
      inner.set_value(temp);
      return inner.get_future();
   });
   promise.set_value(expected);
   auto& actual = unwrapped.get();
   auto actualAddress = &actual;

   // Assert
   EXPECT_EQ(expectedAddress, actualAddress) << "Future::then failed to return the expected value of the unwrapped continuation.";
}

TEST(FutureTest_Void, Then_UnwrapsAndReturnsNestedVoidWhenComplete)
{
   // Arrange
   promise<void> promise;
   auto future = promise.get_future();
   bool continuationCalled = false;

   // Act
   eventual::future<void> unwrapped = future.then([&continuationCalled](auto f)
   {
      f.get();
      continuationCalled = true;
      return make_ready_future();
   });
   promise.set_value();
   unwrapped.get();

   // Assert
   EXPECT_TRUE(continuationCalled) << "Future::then failed to return the unwrapped continuation..";
}
