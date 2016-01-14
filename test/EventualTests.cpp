#include "stdafx.h"
#include <exception>
#include <memory>
#include <vector>
#include <eventual\eventual.h>
#include "NonCopyable.h"

using namespace eventual;

namespace
{
   class EventualTestException { };

   template<class R>
   Future<R> MakeCompleteFuture()
   {
      struct Anonymous { };
      return Make_Exceptional_Future<R, Anonymous>(Anonymous());
   }

   template<class R>
   void CompletePromise(Promise<R>& promise)
   {
      struct Anonymous { };
      promise.Set_Exception(std::make_exception_ptr(Anonymous()));
   }

   template<class P, class... POther>
   void CompletePromise(P& promise, POther&... otherPromises)
   {
      CompletePromise(promise);
      CompletePromise(otherPromises...);
   }
}

// Typed Tests
template<typename T>
class EventualTest : public testing::Test { };
typedef testing::Types<void, int, int&> EventualReturnTypes;
TYPED_TEST_CASE(EventualTest, EventualReturnTypes); // ignore intellisense warning

TYPED_TEST(EventualTest, MakeExceptionFuturePtr_ReturnsExceptionalFuture)
{
   // Arrange
   auto exPtr = std::make_exception_ptr(EventualTestException());
   
   // Act
   auto future = Make_Exceptional_Future<TypeParam>(exPtr);

   // Assert
   EXPECT_TRUE(future.Valid());
   EXPECT_TRUE(future.Is_Ready());
   EXPECT_THROW(future.Get(), EventualTestException);
}

TYPED_TEST(EventualTest, MakeExceptionFuture_ReturnsExceptionalFuture)
{
   // Act
   auto future = Make_Exceptional_Future<TypeParam, EventualTestException>(EventualTestException());

   // Assert
   EXPECT_TRUE(future.Valid());
   EXPECT_TRUE(future.Is_Ready());
   EXPECT_THROW(future.Get(), EventualTestException);
}

TYPED_TEST(EventualTest, WhenAllIterator_ReturnsCompleteFuture_WhenNoFuturesAreProvided)
{
   // Arrange
   std::vector<Future<TypeParam>> futures;

   // Act
   auto allFuture = When_All(futures.begin(), futures.end());

   // Assert
   EXPECT_TRUE(allFuture.Valid());
   EXPECT_TRUE(allFuture.Is_Ready());
   EXPECT_TRUE(allFuture.Get().empty());
}

TYPED_TEST(EventualTest, WhenAllIterator_ReturnsIncompleteFuture_WhenNoneAreComplete)
{
   // Arrange
   Promise<TypeParam> promise1;
   Promise<TypeParam> promise2;
   Promise<TypeParam> promise3;
   Promise<TypeParam> promise4;
   Promise<TypeParam> promise5;
   std::vector<Future<TypeParam>> futures;
   futures.emplace_back(promise1.Get_Future());
   futures.emplace_back(promise2.Get_Future());
   futures.emplace_back(promise3.Get_Future());
   futures.emplace_back(promise4.Get_Future());
   futures.emplace_back(promise5.Get_Future());

   // Act
   auto allFuture = When_All(futures.begin(), futures.end());

   // Assert
   EXPECT_TRUE(allFuture.Valid());
   EXPECT_FALSE(allFuture.Is_Ready());

   CompletePromise(promise1, promise2, promise3, promise4, promise5);
}

TYPED_TEST(EventualTest, WhenAllIterator_ReturnsCompleteFuture_WhenAllAreComplete)
{
   // Arrange
   std::vector<Future<TypeParam>> futures;
   futures.emplace_back(MakeCompleteFuture<TypeParam>());
   futures.emplace_back(MakeCompleteFuture<TypeParam>());
   futures.emplace_back(MakeCompleteFuture<TypeParam>());
   futures.emplace_back(MakeCompleteFuture<TypeParam>());
   futures.emplace_back(MakeCompleteFuture<TypeParam>());

   // Act
   auto allFuture = When_All(futures.begin(), futures.end());

   // Assert
   EXPECT_TRUE(allFuture.Valid());
   EXPECT_TRUE(allFuture.Is_Ready());
}

TYPED_TEST(EventualTest, WhenAllIterator_ReturnsIncompleteFuture_WhenPartiallyComplete)
{
   // Arrange
   Promise<TypeParam> promise;
   std::vector<Future<TypeParam>> futures;
   futures.emplace_back(MakeCompleteFuture<TypeParam>());
   futures.emplace_back(MakeCompleteFuture<TypeParam>());
   futures.emplace_back(MakeCompleteFuture<TypeParam>());
   futures.emplace_back(promise.Get_Future());
   futures.emplace_back(MakeCompleteFuture<TypeParam>());

   // Act
   auto allFuture = When_All(futures.begin(), futures.end());

   // Assert
   EXPECT_TRUE(allFuture.Valid());
   EXPECT_FALSE(allFuture.Is_Ready());

   CompletePromise(promise);
}

TYPED_TEST(EventualTest, WhenAllIterator_CompletesAfterReturning)
{
   // Arrange
   Promise<TypeParam> promise;
   std::vector<Future<TypeParam>> futures;
   futures.emplace_back(MakeCompleteFuture<TypeParam>());
   futures.emplace_back(MakeCompleteFuture<TypeParam>());
   futures.emplace_back(MakeCompleteFuture<TypeParam>());
   futures.emplace_back(promise.Get_Future());
   futures.emplace_back(MakeCompleteFuture<TypeParam>());
   auto allFuture = When_All(futures.begin(), futures.end());
   EXPECT_TRUE(allFuture.Valid());
   EXPECT_FALSE(allFuture.Is_Ready());

   // Act
   CompletePromise(promise);

   // Assert
   EXPECT_TRUE(allFuture.Is_Ready());
}



TYPED_TEST(EventualTest, WhenAllVeradic_ReturnsIncompleteFuture_WhenNoneAreComplete)
{
   // Arrange
   Promise<TypeParam> promise1;
   Promise<TypeParam> promise2;
   Promise<TypeParam> promise3;
   Promise<TypeParam> promise4;
   Promise<TypeParam> promise5;

   // Act
   auto allFuture = When_All(
      promise1.Get_Future(), 
      promise2.Get_Future(), 
      promise3.Get_Future(), 
      promise4.Get_Future(), 
      promise5.Get_Future());

   // Assert
   EXPECT_TRUE(allFuture.Valid());
   EXPECT_FALSE(allFuture.Is_Ready());

   CompletePromise(promise1, promise2, promise3, promise4, promise5);
   EXPECT_TRUE(allFuture.Is_Ready());
}

TYPED_TEST(EventualTest, WhenAllVeradic_ReturnsCompleteFuture_WhenAllAreComplete)
{
   // Act
   auto allFuture = When_All(
      MakeCompleteFuture<TypeParam>(), 
      MakeCompleteFuture<TypeParam>(), 
      MakeCompleteFuture<TypeParam>(), 
      MakeCompleteFuture<TypeParam>(), 
      MakeCompleteFuture<TypeParam>());

   // Assert
   EXPECT_TRUE(allFuture.Valid());
   EXPECT_TRUE(allFuture.Is_Ready());
}

TYPED_TEST(EventualTest, WhenAllVeradic_ReturnsIncompleteFuture_WhenPartiallyComplete)
{
   // Arrange
   Promise<TypeParam> promise;
   
   // Act
   auto allFuture = When_All(
      MakeCompleteFuture<TypeParam>(),
      MakeCompleteFuture<TypeParam>(),
      MakeCompleteFuture<TypeParam>(),
      promise.Get_Future(),
      MakeCompleteFuture<TypeParam>());

   // Assert
   EXPECT_TRUE(allFuture.Valid());
   EXPECT_FALSE(allFuture.Is_Ready());
   CompletePromise(promise);
   EXPECT_TRUE(allFuture.Is_Ready());
}

TYPED_TEST(EventualTest, WhenAllVeradic_CompletesAfterReturning)
{
   // Arrange
   Promise<TypeParam> promise;
   auto allFuture = When_All(
      MakeCompleteFuture<TypeParam>(),
      MakeCompleteFuture<TypeParam>(),
      MakeCompleteFuture<TypeParam>(),
      promise.Get_Future(),
      MakeCompleteFuture<TypeParam>());
   EXPECT_TRUE(allFuture.Valid());
   EXPECT_FALSE(allFuture.Is_Ready());

   // Act
   CompletePromise(promise);

   // Assert
   EXPECT_TRUE(allFuture.Is_Ready());
}

TYPED_TEST(EventualTest, WhenAnyIterator_ReturnsCompleteFuture_WhenNoFuturesAreProvided)
{
   // Arrange
   auto expectedIndex = static_cast<size_t>(-1);
   std::vector<Future<TypeParam>> futures;

   // Act
   auto allFuture = When_Any(futures.begin(), futures.end());

   // Assert
   EXPECT_TRUE(allFuture.Valid());
   EXPECT_TRUE(allFuture.Is_Ready());
   auto result = allFuture.Get();
   EXPECT_TRUE(result.futures.empty());
   EXPECT_EQ(expectedIndex, result.index);
}

TYPED_TEST(EventualTest, WhenAnyIterator_ReturnsIncompleteFuture_WhenNoneAreComplete)
{
   // Arrange
   Promise<TypeParam> promise1;
   Promise<TypeParam> promise2;
   Promise<TypeParam> promise3;
   Promise<TypeParam> promise4;
   Promise<TypeParam> promise5;
   auto futures = std::vector<Future<TypeParam>>();
   futures.emplace_back(promise1.Get_Future());
   futures.emplace_back(promise2.Get_Future());
   futures.emplace_back(promise3.Get_Future());
   futures.emplace_back(promise4.Get_Future());
   futures.emplace_back(promise5.Get_Future());

   // Act
   auto anyFuture = When_Any(futures.begin(), futures.end());

   // Assert
   EXPECT_TRUE(anyFuture.Valid());
   EXPECT_FALSE(anyFuture.Is_Ready());

   CompletePromise(promise1, promise2, promise3, promise4, promise5);
   EXPECT_TRUE(anyFuture.Is_Ready());
}

TYPED_TEST(EventualTest, WhenAnyIterator_ReturnsCompleteFuture_WhenAllAreComplete)
{
   // Arrange
   auto futures = std::vector<Future<TypeParam>>();
   futures.emplace_back(MakeCompleteFuture<TypeParam>());
   futures.emplace_back(MakeCompleteFuture<TypeParam>());
   futures.emplace_back(MakeCompleteFuture<TypeParam>());
   futures.emplace_back(MakeCompleteFuture<TypeParam>());
   futures.emplace_back(MakeCompleteFuture<TypeParam>());
   
   // Act
   auto anyFuture = When_Any(futures.begin(), futures.end());

   // Assert
   EXPECT_TRUE(anyFuture.Valid());
   EXPECT_TRUE(anyFuture.Is_Ready());
}

TYPED_TEST(EventualTest, WhenAnyIterator_ReturnsCompleteFuture_WhenPartiallyComplete)
{
   // Arrange
   Promise<TypeParam> promise;
   std::vector<Future<TypeParam>> futures;
   futures.emplace_back(MakeCompleteFuture<TypeParam>());
   futures.emplace_back(MakeCompleteFuture<TypeParam>());
   futures.emplace_back(MakeCompleteFuture<TypeParam>());
   futures.emplace_back(promise.Get_Future());
   futures.emplace_back(MakeCompleteFuture<TypeParam>());

   // Act
   auto anyFuture = When_Any(futures.begin(), futures.end());

   // Assert
   EXPECT_TRUE(anyFuture.Valid());
   EXPECT_TRUE(anyFuture.Is_Ready());

   CompletePromise(promise);
}

TYPED_TEST(EventualTest, WhenAnyIterator_CompletesAfterReturning)
{
   // Arrange
   Promise<TypeParam> promise1;
   Promise<TypeParam> promise2;
   std::vector<Future<TypeParam>> futures;
   futures.emplace_back(promise1.Get_Future());
   futures.emplace_back(promise2.Get_Future());
   auto anyFuture = When_Any(futures.begin(), futures.end());
   EXPECT_TRUE(anyFuture.Valid());
   EXPECT_FALSE(anyFuture.Is_Ready());

   // Act
   CompletePromise(promise1);

   // Assert
   EXPECT_TRUE(anyFuture.Is_Ready());

   CompletePromise(promise2);
}

TYPED_TEST(EventualTest, WhenAnyIterator_ReturnsIndexOfCompleteFuture)
{
   // Arrange
   Promise<TypeParam> promise0;
   Promise<TypeParam> promise1;
   Promise<TypeParam> promise2;
   Promise<TypeParam> promise3;
   Promise<TypeParam> promise4;
   std::vector<Future<TypeParam>> futures;
   futures.emplace_back(promise0.Get_Future());
   futures.emplace_back(promise1.Get_Future());
   futures.emplace_back(promise2.Get_Future());
   futures.emplace_back(promise3.Get_Future());
   futures.emplace_back(promise4.Get_Future());

   // Act
   auto anyFuture = When_Any(futures.begin(), futures.end());
   CompletePromise(promise3);

   // Assert
   auto results = anyFuture.Get();
   EXPECT_EQ(3, results.index);

   CompletePromise(promise0, promise1, promise2, promise4);
}

TYPED_TEST(EventualTest, WhenAnyVeradic_ReturnsIncompleteFuture_WhenNoneAreComplete)
{
   // Arrange
   Promise<TypeParam> promise1;
   Promise<TypeParam> promise2;
   Promise<TypeParam> promise3;
   Promise<TypeParam> promise4;
   Promise<TypeParam> promise5;

   // Act
   auto anyFuture = When_Any(
      promise1.Get_Future(),
      promise2.Get_Future(),
      promise3.Get_Future(),
      promise4.Get_Future(),
      promise5.Get_Future());

   // Assert
   EXPECT_TRUE(anyFuture.Valid());
   EXPECT_FALSE(anyFuture.Is_Ready());

   CompletePromise(promise1, promise2, promise3, promise4, promise5);
}

TYPED_TEST(EventualTest, WhenAnyVeradic_ReturnsCompleteFuture_WhenAllAreComplete)
{
   // Act
   auto anyFuture = When_Any(
      MakeCompleteFuture<TypeParam>(),
      MakeCompleteFuture<TypeParam>(),
      MakeCompleteFuture<TypeParam>(),
      MakeCompleteFuture<TypeParam>(),
      MakeCompleteFuture<TypeParam>());

   // Assert
   EXPECT_TRUE(anyFuture.Valid());
   EXPECT_TRUE(anyFuture.Is_Ready());
}

TYPED_TEST(EventualTest, WhenAnyVeradic_ReturnsCompleteFuture_WhenPartiallyComplete)
{
   // Arrange
   Promise<TypeParam> promise;

   // Act
   auto anyFuture = When_Any(
      MakeCompleteFuture<TypeParam>(),
      MakeCompleteFuture<TypeParam>(),
      MakeCompleteFuture<TypeParam>(),
      promise.Get_Future(),
      MakeCompleteFuture<TypeParam>());

   // Assert
   EXPECT_TRUE(anyFuture.Valid());
   EXPECT_TRUE(anyFuture.Is_Ready());
   
   CompletePromise(promise);
}

TYPED_TEST(EventualTest, WhenAnyVeradic_CompletesAfterReturning)
{
   // Arrange
   Promise<TypeParam> promise1;
   Promise<TypeParam> promise2;
   auto anyFuture = When_Any(promise1.Get_Future(), promise2.Get_Future());
   EXPECT_TRUE(anyFuture.Valid());
   EXPECT_FALSE(anyFuture.Is_Ready());

   // Act
   CompletePromise(promise1);

   // Assert
   EXPECT_TRUE(anyFuture.Is_Ready());

   CompletePromise(promise2);
}

TYPED_TEST(EventualTest, WhenAnyVeradic_ReturnsIndexOfCompleteFuture)
{
   // Arrange
   Promise<TypeParam> promise0;
   Promise<TypeParam> promise1;
   Promise<TypeParam> promise2;
   Promise<TypeParam> promise3;
   Promise<TypeParam> promise4;

   // Act
   auto anyFuture = When_Any(
      promise0.Get_Future(),
      promise1.Get_Future(),
      promise2.Get_Future(),
      promise3.Get_Future(),
      promise4.Get_Future());
   CompletePromise(promise3);

   // Assert
   auto results = anyFuture.Get();
   EXPECT_EQ(3, results.index);

   CompletePromise(promise0, promise1, promise2, promise4);
}

TEST(EventualTest_WhenAny, WhenAllVeradic_ReturnsCompleteFuture_WhenNoFuturesAreProvided)
{
   // Act
   Future<std::tuple<>> allFuture = When_All();

   // Assert
   EXPECT_TRUE(allFuture.Valid());
   EXPECT_TRUE(allFuture.Is_Ready());
}

TEST(EventualTest_WhenAny, WhenAnyVeradic_ReturnsCompleteFuture_WhenNoFuturesAreProvided)
{
   // Arrange
   auto expectedIndex = static_cast<size_t>(-1);
   
   // Act
   Future<When_Any_Result<std::tuple<>>> allFuture = When_Any();

   // Assert
   EXPECT_TRUE(allFuture.Valid());
   EXPECT_TRUE(allFuture.Is_Ready());
   EXPECT_EQ(expectedIndex, allFuture.Get().index);
}

TEST(EventualTest_WhenAll, WhenAllIterator_MaintainsResultOrderInVector)
{
   // Arrange
   Promise<int> promise;
   std::vector<Future<int>> futures;
   futures.emplace_back(Make_Ready_Future(0));
   futures.emplace_back(Make_Ready_Future(1));
   futures.emplace_back(Make_Ready_Future(2));
   futures.emplace_back(promise.Get_Future());
   futures.emplace_back(Make_Ready_Future(4));

   // Act
   auto allFuture = When_All(futures.begin(), futures.end());
   promise.Set_Value(3);

   // Assert
   auto results = allFuture.Get();

   int idx = 0;
   for (auto& future : results)
   {
      EXPECT_EQ(idx, future.Get());
      idx++;
   }
}

TEST(EventualTest_WhenAll, WhenAllVeradic_MaintainsResultOrderInTuple)
{
   // Arrange
   Promise<int> promise;

   // Act
   auto allFuture = When_All(
      Make_Ready_Future(0),
      Make_Ready_Future(1),
      Make_Ready_Future(2),
      promise.Get_Future(),
      Make_Ready_Future(4));
   promise.Set_Value(3);

   // Assert
   auto results = allFuture.Get();

   EXPECT_EQ(0, std::get<0>(results).Get());
   EXPECT_EQ(1, std::get<1>(results).Get());
   EXPECT_EQ(2, std::get<2>(results).Get());
   EXPECT_EQ(3, std::get<3>(results).Get());
   EXPECT_EQ(4, std::get<4>(results).Get());
}

TEST(EventualTest_WhenAny, WhenAnyIterator_MaintainsResultOrderInVector)
{
   // Arrange
   Promise<int> promise;
   std::vector<Future<int>> futures;
   futures.emplace_back(Make_Ready_Future(0));
   futures.emplace_back(Make_Ready_Future(1));
   futures.emplace_back(Make_Ready_Future(2));
   futures.emplace_back(promise.Get_Future());
   futures.emplace_back(Make_Ready_Future(4));

   // Act
   auto anyFuture = When_Any(futures.begin(), futures.end());
   promise.Set_Value(3);

   // Assert
   auto results = anyFuture.Get();

   int idx = 0;
   for (auto& future : results.futures)
   {
      EXPECT_EQ(idx, future.Get());
      idx++;
   }
}

TEST(EventualTest_WhenAny, WhenAnyVeradic_MaintainsResultOrderInTuple)
{
   // Arrange
   Promise<int> promise;

   // Act
   auto anyFuture = When_Any(
      Make_Ready_Future(0),
      Make_Ready_Future(1),
      Make_Ready_Future(2),
      promise.Get_Future(),
      Make_Ready_Future(4));
   promise.Set_Value(3);

   // Assert
   auto results = anyFuture.Get();

   EXPECT_EQ(0, std::get<0>(results.futures).Get());
   EXPECT_EQ(1, std::get<1>(results.futures).Get());
   EXPECT_EQ(2, std::get<2>(results.futures).Get());
   EXPECT_EQ(3, std::get<3>(results.futures).Get());
   EXPECT_EQ(4, std::get<4>(results.futures).Get());
}

TEST(EventualTest_Value, MakeReadyFuture_ReturnsACompleteFuture)
{
   // Act
   auto future = Make_Ready_Future<int>(3);

   // Assert
   EXPECT_TRUE(future.Valid());
   EXPECT_TRUE(future.Is_Ready());
   EXPECT_EQ(3, future.Get());
}

TEST(EventualTest_Value, MakeReadyFuture_MoveValue_ReturnsACompleteFuture)
{
   // Arrange
   auto nc = NonCopyable();
   nc._markerValue = 1313;
   
   // Act
   auto future = Make_Ready_Future(std::move(nc));

   // Assert
   EXPECT_TRUE(future.Valid());
   EXPECT_TRUE(future.Is_Ready());
   EXPECT_EQ(1313, future.Get()._markerValue);
}

TEST(EventualTest_Reference, MakeReadyFuture_ReturnsACompleteFuture)
{
   // Arrange
   int expected = 489;
   auto expectedAddress = &expected;

   // Act
   auto future = Make_Ready_Future(std::reference_wrapper<int>(expected));

   // Assert
   EXPECT_TRUE(future.Valid());
   EXPECT_TRUE(future.Is_Ready());
   auto& result = future.Get();
   auto actualAddress = &result;
   EXPECT_EQ(expectedAddress, actualAddress);
}

TEST(EventualTest_Void, MakeReadyFuture_ReturnsACompleteFuture)
{
   // Act
   auto future = Make_Ready_Future();

   // Assert
   EXPECT_TRUE(future.Valid());
   EXPECT_TRUE(future.Is_Ready());
   EXPECT_NO_THROW(future.Get());
}