#include "stdafx.h"
#include <exception>
#include <eventual/eventual.h>
#include "FutureTestPatterns.h"

using namespace eventual;

namespace
{
   class TestException { };
   
   template<class R>
   shared_future<R> MakeCompleteSharedFuture()
   {
      struct Anonymous { };
      return make_exceptional_future<R>(std::make_exception_ptr(Anonymous()));
   }

   template<class R>
   future<R> MakeCompleteFuture()
   {
      struct Anonymous { };
      return make_exceptional_future<R>(std::make_exception_ptr(Anonymous()));
   }
}

// Parameterized Typed Tests
typedef testing::Types<shared_future<void>, shared_future<int>, shared_future<int&>> SharedFutureTypes;
INSTANTIATE_TYPED_TEST_CASE_P(SharedFuture, FutureTestPatterns, SharedFutureTypes);

// Typed Tests
template<typename T>
class SharedFutureTest : public testing::Test { };
typedef testing::Types<void, int, int&> SharedFutureReturnTypes;
TYPED_TEST_CASE(SharedFutureTest, SharedFutureReturnTypes); // ignore intellisense warning

TYPED_TEST(SharedFutureTest, SharedFuture_IsCopyConstructable)
{
   // Arrange
   auto future = MakeCompleteSharedFuture<TypeParam>();

   // Act
   shared_future<TypeParam> copy(future);
   
   // Assert
   EXPECT_TRUE(future.valid()) << "Original shared future did not remain valid.";
   EXPECT_TRUE(copy.valid()) << "Copied shared future is not valid.";
}

TYPED_TEST(SharedFutureTest, SharedFuture_ConstructorUnwrapsNestedFutures)
{
   // Arrange
   future<shared_future<TypeParam>> nested = MakeCompleteFuture<shared_future<TypeParam>>();

   // Act
   shared_future<TypeParam> unwrapped(std::move(nested));

   // Assert
   EXPECT_FALSE(nested.valid()) << "nested future should not remain valid";
   EXPECT_TRUE(unwrapped.valid()) << "Unwrapped future should be valid";
}

TYPED_TEST(SharedFutureTest, SharedFuture_IsCopyAssignable)
{
   // Arrange
   auto future = MakeCompleteSharedFuture<TypeParam>();

   // Act
   auto copy = future;

   // Assert
   EXPECT_TRUE(future.valid()) << "Original shared future did not remain valid.";
   EXPECT_TRUE(copy.valid()) << "Copied shared future is not valid.";
}

TYPED_TEST(SharedFutureTest, Then_DoesNotInvalidateSharedFuture)
{
   // Arrange
   auto future = MakeCompleteSharedFuture<TypeParam>();

   // Act
   future.then([](auto) { });

   // Assert
   EXPECT_TRUE(future.valid()) << "shared_future::then incorrectly invalidated the future.";
}

TYPED_TEST(SharedFutureTest, Get_DoesNotInvalidateFuture)
{
   // Arrange
   auto future = MakeCompleteSharedFuture<TypeParam>();

   // Act
   try
   {
      future.get();
   }
   catch (...) { }

   // Assert
   EXPECT_TRUE(future.valid()) << "shared_future::then incorrectly invalidated the future.";
}

// Standard Tests

TEST(SharedFutureTest_Value, Get_ReturnsCorrectValueWhenComplete)
{
   // Arrange
   int expected = 13;
   auto future = make_ready_future(expected).share();

   // Act
   int actual = future.get();

   // Assert
   EXPECT_EQ(expected, actual) << "SharedFuture did not return the expected value.";
}

TEST(SharedFutureTest_Reference, Get_ReturnsCorrectReferenceWhenComplete)
{
   // Arrange
   int expected = 13;
   auto expectedAddress = &expected;
   promise<int&> promise;
   promise.set_value(expected);
   auto future = promise.get_future().share();

   // Act
   int& actual = future.get();

   // Assert
   auto actualAddress = &actual;

   EXPECT_EQ(expectedAddress, actualAddress) << "SharedFuture did not return the expected reference address.";
}

TEST(SharedFutureTest_Void, Get_ReturnsVoidWhenComplete)
{
   // Arrange
   auto future = make_ready_future().share();

   // Act
   EXPECT_NO_THROW(future.get());

   // Assert (-ish, more of a compile-time constant)
   EXPECT_TRUE(std::is_void<std::result_of<decltype(&shared_future<void>::get)(shared_future<void>)>::type>::value) << "shared_future<void>::get() does not return void.";
}

TEST(SharedFutureTest_Value, Then_InvokesContinuationWhenComplete)
{
   // Arrange
   promise<int> promise;
   auto future = promise.get_future().share();
   int expected = 12;
   int actual = -1;

   // Act
   future.then([&actual](auto f) { actual = f.get(); });
   promise.set_value(expected);

   // Assert
   EXPECT_EQ(expected, actual) << "SharedFuture::then failed to register a continuation that was called.";
}

TEST(SharedFutureTest_Reference, Then_InvokesContinuationWhenComplete)
{
   // Arrange
   promise<int&> promise;
   auto future = promise.get_future().share();
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
   EXPECT_EQ(expectedAddress, actualAddress) << "SharedFuture::then failed to register a continuation that was called.";
}

TEST(SharedFutureTest_Void, Then_InvokesContinuationWhenComplete)
{
   // Arrange
   promise<void> promise;
   auto future = promise.get_future().share();
   bool continuationCalled = false;

   // Act
   future.then([&continuationCalled](auto f)
   {
      f.get();
      continuationCalled = true;
   });
   promise.set_value();

   // Assert
   EXPECT_TRUE(continuationCalled) << "SharedFuture::then failed to register a continuation that was called.";
}

TEST(SharedFutureTest_Value, Then_UnwrapsAndReturnNestedResultWhenComplete)
{
   // Arrange
   promise<int> outer;
   auto future = outer.get_future().share();
   int expected = 13;

   // Act
   auto unwrapped = future.then([](auto f)
   {
      eventual::promise<int> inner;
      auto temp = f.get() + 1;
      inner.set_value(temp);
      return inner.get_future().share();
   });
   outer.set_value(12);
   auto actual = unwrapped.get();

   // Assert
   EXPECT_EQ(expected, actual) << "SharedFuture::then failed to return the expected value of the unwrapped continuation.";
}

TEST(SharedFutureTest_Reference, Then_UnwrapsAndReturnsNestedResultWhenComplete)
{
   // Arrange
   promise<int&> outer;
   auto future = outer.get_future().share();
   int expected = 12;
   auto expectedAddress = &expected;

   // Act
   auto unwrapped = future.then([](auto f)
   {
      eventual::promise<int&> inner;
      auto& temp = f.get();
      inner.set_value(temp);
      return inner.get_future().share();
   });
   outer.set_value(expected);
   auto& actual = unwrapped.get();
   auto actualAddress = &actual;

   // Assert
   EXPECT_EQ(expectedAddress, actualAddress) << "SharedFuture::then failed to return the expected value of the unwrapped continuation.";
}

TEST(SharedFutureTest_Void, Then_UnwrapsAndReturnsNestedResultWhenComplete)
{
   // Arrange
   promise<void> promise;
   auto future = promise.get_future().share();
   bool continuationCalled = false;

   // Act
   shared_future<void> unwrapped = future.then([&continuationCalled](auto f)
   {
      f.get();
      continuationCalled = true;
      return make_ready_future().share();
   });
   promise.set_value();
   unwrapped.get();

   // Assert
   EXPECT_TRUE(continuationCalled) << "SharedFuture::then failed to return the unwrapped continuation..";
}


