#include "stdafx.h"
#include <exception>
#include <eventual\eventual.h>
#include "FutureTestPatterns.h"

using namespace eventual;

namespace
{
   class TestException { };
   
   template<class R>
   Shared_Future<R> MakeCompleteSharedFuture()
   {
      struct Anonymous { };
      return Make_Exceptional_Future<R>(std::make_exception_ptr(Anonymous()));
   }

   template<class R>
   Future<R> MakeCompleteFuture()
   {
      struct Anonymous { };
      return Make_Exceptional_Future<R>(std::make_exception_ptr(Anonymous()));
   }
}

// Parameterized Typed Tests
typedef testing::Types<Shared_Future<void>, Shared_Future<int>, Shared_Future<int&>> SharedFutureTypes;
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
   Shared_Future<TypeParam> copy(future);
   
   // Assert
   EXPECT_TRUE(future.Valid()) << "Original shared future did not remain valid.";
   EXPECT_TRUE(copy.Valid()) << "Copied shared future is not valid.";
}

TYPED_TEST(SharedFutureTest, SharedFuture_ConstructorUnwrapsNestedFutures)
{
   // Arrange
   Future<Shared_Future<TypeParam>> nested = MakeCompleteFuture<Shared_Future<TypeParam>>();

   // Act
   Shared_Future<TypeParam> unwrapped(std::move(nested));

   // Assert
   EXPECT_FALSE(nested.Valid()) << "nested future should not remain valid";
   EXPECT_TRUE(unwrapped.Valid()) << "Unwrapped future should be valid";
}

TYPED_TEST(SharedFutureTest, SharedFuture_IsCopyAssignable)
{
   // Arrange
   auto future = MakeCompleteSharedFuture<TypeParam>();

   // Act
   auto copy = future;

   // Assert
   EXPECT_TRUE(future.Valid()) << "Original shared future did not remain valid.";
   EXPECT_TRUE(copy.Valid()) << "Copied shared future is not valid.";
}

TYPED_TEST(SharedFutureTest, Then_DoesNotInvalidateSharedFuture)
{
   // Arrange
   auto future = MakeCompleteSharedFuture<TypeParam>();

   // Act
   future.Then([](auto& f) { });

   // Assert
   EXPECT_TRUE(future.Valid()) << "Shared_Future::Then incorrectly invalidated the future.";
}

TYPED_TEST(SharedFutureTest, Get_DoesNotInvalidateFuture)
{
   // Arrange
   auto future = MakeCompleteSharedFuture<TypeParam>();

   // Act
   try
   {
      future.Get();
   }
   catch (...) { }

   // Assert
   EXPECT_TRUE(future.Valid()) << "Shared_Future::Then incorrectly invalidated the future.";
}

// Standard Tests

TEST(SharedFutureTest_Value, Get_ReturnsCorrectValueWhenComplete)
{
   // Arrange
   int expected = 13;
   auto future = Make_Ready_Future(expected).Share();

   // Act
   int actual = future.Get();

   // Assert
   EXPECT_EQ(expected, actual) << "SharedFuture did not return the expected value.";
}

TEST(SharedFutureTest_Reference, Get_ReturnsCorrectReferenceWhenComplete)
{
   // Arrange
   int expected = 13;
   auto expectedAddress = &expected;
   Promise<int&> promise;
   promise.Set_Value(expected);
   auto future = promise.Get_Future().Share();

   // Act
   int& actual = future.Get();

   // Assert
   auto actualAddress = &actual;

   EXPECT_EQ(expectedAddress, actualAddress) << "SharedFuture did not return the expected reference address.";
}

TEST(SharedFutureTest_Void, Get_ReturnsVoidWhenComplete)
{
   // Arrange
   auto future = Make_Ready_Future().Share();

   // Act
   EXPECT_NO_THROW(future.Get());

   // Assert (-ish, more of a compile-time constant)
   EXPECT_TRUE(std::is_void<std::result_of<decltype(&Shared_Future<void>::Get)(Shared_Future<void>)>::type>::value) << "Shared_Future<void>::Get() does not return void.";
}

TEST(SharedFutureTest_Value, Then_InvokesContinuationWhenComplete)
{
   // Arrange
   Promise<int> promise;
   auto future = promise.Get_Future().Share();
   int expected = 12;
   int actual = -1;

   // Act
   future.Then([&actual](auto& f) { actual = f.Get(); });
   promise.Set_Value(expected);

   // Assert
   EXPECT_EQ(expected, actual) << "SharedFuture::Then failed to register a continuation that was called.";
}

TEST(SharedFutureTest_Reference, Then_InvokesContinuationWhenComplete)
{
   // Arrange
   Promise<int&> promise;
   auto future = promise.Get_Future().Share();
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
   EXPECT_EQ(expectedAddress, actualAddress) << "SharedFuture::Then failed to register a continuation that was called.";
}

TEST(SharedFutureTest_Void, Then_InvokesContinuationWhenComplete)
{
   // Arrange
   Promise<void> promise;
   auto future = promise.Get_Future().Share();
   bool continuationCalled = false;

   // Act
   future.Then([&continuationCalled](auto& f)
   {
      f.Get();
      continuationCalled = true;
   });
   promise.Set_Value();

   // Assert
   EXPECT_TRUE(continuationCalled) << "SharedFuture::Then failed to register a continuation that was called.";
}

TEST(SharedFutureTest_Value, Then_UnwrapsAndReturnNestedResultWhenComplete)
{
   // Arrange
   Promise<int> promise;
   auto future = promise.Get_Future().Share();
   int expected = 13;

   // Act
   auto unwrapped = future.Then([](auto& f)
   {
      Promise<int> inner;
      auto temp = f.Get() + 1;
      inner.Set_Value(temp);
      return inner.Get_Future().Share();
   });
   promise.Set_Value(12);
   auto actual = unwrapped.Get();

   // Assert
   EXPECT_EQ(expected, actual) << "SharedFuture::Then failed to return the expected value of the unwrapped continuation.";
}

TEST(SharedFutureTest_Reference, Then_UnwrapsAndReturnsNestedResultWhenComplete)
{
   // Arrange
   Promise<int&> promise;
   auto future = promise.Get_Future().Share();
   int expected = 12;
   auto expectedAddress = &expected;

   // Act
   auto unwrapped = future.Then([](auto& f)
   {
      Promise<int&> inner;
      auto& temp = f.Get();
      inner.Set_Value(temp);
      return inner.Get_Future().Share();
   });
   promise.Set_Value(expected);
   auto& actual = unwrapped.Get();
   auto actualAddress = &actual;

   // Assert
   EXPECT_EQ(expectedAddress, actualAddress) << "SharedFuture::Then failed to return the expected value of the unwrapped continuation.";
}

TEST(SharedFutureTest_Void, Then_UnwrapsAndReturnsNestedResultWhenComplete)
{
   // Arrange
   Promise<void> promise;
   auto future = promise.Get_Future().Share();
   bool continuationCalled = false;

   // Act
   Shared_Future<void> unwrapped = future.Then([&continuationCalled](auto& f)
   {
      f.Get();
      continuationCalled = true;
      return Make_Ready_Future().Share();
   });
   promise.Set_Value();
   unwrapped.Get();

   // Assert
   EXPECT_TRUE(continuationCalled) << "SharedFuture::Then failed to return the unwrapped continuation..";
}


