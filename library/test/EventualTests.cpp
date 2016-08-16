#include "stdafx.h"
#include <exception>
#include <memory>
#include <vector>
#include <eventual/eventual.h>
#include "NonCopyable.h"

using namespace eventual;

namespace
{
   class EventualTestException { };

   template<class R>
   future<R> MakeCompleteFuture()
   {
      struct Anonymous { };
      return make_exceptional_future<R, Anonymous>(Anonymous());
   }

   template<class R>
   void CompletePromise(promise<R>& promise)
   {
      struct Anonymous { };
      promise.set_exception(std::make_exception_ptr(Anonymous()));
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
   auto future = make_exceptional_future<TypeParam>(exPtr);

   // Assert
   EXPECT_TRUE(future.valid());
   EXPECT_TRUE(future.is_ready());
   EXPECT_THROW(future.get(), EventualTestException);
}

TYPED_TEST(EventualTest, MakeExceptionFuture_ReturnsExceptionalFuture)
{
   // Act
   auto future = make_exceptional_future<TypeParam, EventualTestException>(EventualTestException());

   // Assert
   EXPECT_TRUE(future.valid());
   EXPECT_TRUE(future.is_ready());
   EXPECT_THROW(future.get(), EventualTestException);
}

TYPED_TEST(EventualTest, WhenAllIterator_ReturnsCompleteFuture_WhenNoFuturesAreProvided)
{
   // Arrange
   std::vector<future<TypeParam>> futures;

   // Act
   auto allFuture = when_all(futures.begin(), futures.end());

   // Assert
   EXPECT_TRUE(allFuture.valid());
   EXPECT_TRUE(allFuture.is_ready());
   EXPECT_TRUE(allFuture.get().empty());
}

TYPED_TEST(EventualTest, WhenAllIterator_ReturnsIncompleteFuture_WhenNoneAreComplete)
{
   // Arrange
   promise<TypeParam> promise1;
   promise<TypeParam> promise2;
   promise<TypeParam> promise3;
   promise<TypeParam> promise4;
   promise<TypeParam> promise5;
   std::vector<future<TypeParam>> futures;
   futures.emplace_back(promise1.get_future());
   futures.emplace_back(promise2.get_future());
   futures.emplace_back(promise3.get_future());
   futures.emplace_back(promise4.get_future());
   futures.emplace_back(promise5.get_future());

   // Act
   auto allFuture = when_all(futures.begin(), futures.end());

   // Assert
   EXPECT_TRUE(allFuture.valid());
   EXPECT_FALSE(allFuture.is_ready());

   CompletePromise(promise1, promise2, promise3, promise4, promise5);
}

TYPED_TEST(EventualTest, WhenAllIterator_ReturnsCompleteFuture_WhenAllAreComplete)
{
   // Arrange
   std::vector<future<TypeParam>> futures;
   futures.emplace_back(MakeCompleteFuture<TypeParam>());
   futures.emplace_back(MakeCompleteFuture<TypeParam>());
   futures.emplace_back(MakeCompleteFuture<TypeParam>());
   futures.emplace_back(MakeCompleteFuture<TypeParam>());
   futures.emplace_back(MakeCompleteFuture<TypeParam>());

   // Act
   auto allFuture = when_all(futures.begin(), futures.end());

   // Assert
   EXPECT_TRUE(allFuture.valid());
   EXPECT_TRUE(allFuture.is_ready());
}

TYPED_TEST(EventualTest, WhenAllIterator_ReturnsIncompleteFuture_WhenPartiallyComplete)
{
   // Arrange
   promise<TypeParam> promise;
   std::vector<future<TypeParam>> futures;
   futures.emplace_back(MakeCompleteFuture<TypeParam>());
   futures.emplace_back(MakeCompleteFuture<TypeParam>());
   futures.emplace_back(MakeCompleteFuture<TypeParam>());
   futures.emplace_back(promise.get_future());
   futures.emplace_back(MakeCompleteFuture<TypeParam>());

   // Act
   auto allFuture = when_all(futures.begin(), futures.end());

   // Assert
   EXPECT_TRUE(allFuture.valid());
   EXPECT_FALSE(allFuture.is_ready());

   CompletePromise(promise);
}

TYPED_TEST(EventualTest, WhenAllIterator_CompletesAfterReturning)
{
   // Arrange
   promise<TypeParam> promise;
   std::vector<future<TypeParam>> futures;
   futures.emplace_back(MakeCompleteFuture<TypeParam>());
   futures.emplace_back(MakeCompleteFuture<TypeParam>());
   futures.emplace_back(MakeCompleteFuture<TypeParam>());
   futures.emplace_back(promise.get_future());
   futures.emplace_back(MakeCompleteFuture<TypeParam>());
   auto allFuture = when_all(futures.begin(), futures.end());
   EXPECT_TRUE(allFuture.valid());
   EXPECT_FALSE(allFuture.is_ready());

   // Act
   CompletePromise(promise);

   // Assert
   EXPECT_TRUE(allFuture.is_ready());
}



TYPED_TEST(EventualTest, WhenAllVeradic_ReturnsIncompleteFuture_WhenNoneAreComplete)
{
   // Arrange
   promise<TypeParam> promise1;
   promise<TypeParam> promise2;
   promise<TypeParam> promise3;
   promise<TypeParam> promise4;
   promise<TypeParam> promise5;

   // Act
   auto allFuture = when_all(
      promise1.get_future(), 
      promise2.get_future(), 
      promise3.get_future(), 
      promise4.get_future(), 
      promise5.get_future());

   // Assert
   EXPECT_TRUE(allFuture.valid());
   EXPECT_FALSE(allFuture.is_ready());

   CompletePromise(promise1, promise2, promise3, promise4, promise5);
   EXPECT_TRUE(allFuture.is_ready());
}

TYPED_TEST(EventualTest, WhenAllVeradic_ReturnsCompleteFuture_WhenAllAreComplete)
{
   // Act
   auto allFuture = when_all(
      MakeCompleteFuture<TypeParam>(), 
      MakeCompleteFuture<TypeParam>(), 
      MakeCompleteFuture<TypeParam>(), 
      MakeCompleteFuture<TypeParam>(), 
      MakeCompleteFuture<TypeParam>());

   // Assert
   EXPECT_TRUE(allFuture.valid());
   EXPECT_TRUE(allFuture.is_ready());
}

TYPED_TEST(EventualTest, WhenAllVeradic_ReturnsIncompleteFuture_WhenPartiallyComplete)
{
   // Arrange
   promise<TypeParam> promise;
   
   // Act
   auto allFuture = when_all(
      MakeCompleteFuture<TypeParam>(),
      MakeCompleteFuture<TypeParam>(),
      MakeCompleteFuture<TypeParam>(),
      promise.get_future(),
      MakeCompleteFuture<TypeParam>());

   // Assert
   EXPECT_TRUE(allFuture.valid());
   EXPECT_FALSE(allFuture.is_ready());
   CompletePromise(promise);
   EXPECT_TRUE(allFuture.is_ready());
}

TYPED_TEST(EventualTest, WhenAllVeradic_CompletesAfterReturning)
{
   // Arrange
   promise<TypeParam> promise;
   auto allFuture = when_all(
      MakeCompleteFuture<TypeParam>(),
      MakeCompleteFuture<TypeParam>(),
      MakeCompleteFuture<TypeParam>(),
      promise.get_future(),
      MakeCompleteFuture<TypeParam>());
   EXPECT_TRUE(allFuture.valid());
   EXPECT_FALSE(allFuture.is_ready());

   // Act
   CompletePromise(promise);

   // Assert
   EXPECT_TRUE(allFuture.is_ready());
}

TYPED_TEST(EventualTest, WhenAnyIterator_ReturnsCompleteFuture_WhenNoFuturesAreProvided)
{
   // Arrange
   auto expectedIndex = static_cast<size_t>(-1);
   std::vector<future<TypeParam>> futures;

   // Act
   auto allFuture = when_any(futures.begin(), futures.end());

   // Assert
   EXPECT_TRUE(allFuture.valid());
   EXPECT_TRUE(allFuture.is_ready());
   auto result = allFuture.get();
   EXPECT_TRUE(result.futures.empty());
   EXPECT_EQ(expectedIndex, result.index);
}

TYPED_TEST(EventualTest, WhenAnyIterator_ReturnsIncompleteFuture_WhenNoneAreComplete)
{
   // Arrange
   promise<TypeParam> promise1;
   promise<TypeParam> promise2;
   promise<TypeParam> promise3;
   promise<TypeParam> promise4;
   promise<TypeParam> promise5;
   auto futures = std::vector<future<TypeParam>>();
   futures.emplace_back(promise1.get_future());
   futures.emplace_back(promise2.get_future());
   futures.emplace_back(promise3.get_future());
   futures.emplace_back(promise4.get_future());
   futures.emplace_back(promise5.get_future());

   // Act
   auto anyFuture = when_any(futures.begin(), futures.end());

   // Assert
   EXPECT_TRUE(anyFuture.valid());
   EXPECT_FALSE(anyFuture.is_ready());

   CompletePromise(promise1, promise2, promise3, promise4, promise5);
   EXPECT_TRUE(anyFuture.is_ready());
}

TYPED_TEST(EventualTest, WhenAnyIterator_ReturnsCompleteFuture_WhenAllAreComplete)
{
   // Arrange
   auto futures = std::vector<future<TypeParam>>();
   futures.emplace_back(MakeCompleteFuture<TypeParam>());
   futures.emplace_back(MakeCompleteFuture<TypeParam>());
   futures.emplace_back(MakeCompleteFuture<TypeParam>());
   futures.emplace_back(MakeCompleteFuture<TypeParam>());
   futures.emplace_back(MakeCompleteFuture<TypeParam>());
   
   // Act
   auto anyFuture = when_any(futures.begin(), futures.end());

   // Assert
   EXPECT_TRUE(anyFuture.valid());
   EXPECT_TRUE(anyFuture.is_ready());
}

TYPED_TEST(EventualTest, WhenAnyIterator_ReturnsCompleteFuture_WhenPartiallyComplete)
{
   // Arrange
   promise<TypeParam> promise;
   std::vector<future<TypeParam>> futures;
   futures.emplace_back(MakeCompleteFuture<TypeParam>());
   futures.emplace_back(MakeCompleteFuture<TypeParam>());
   futures.emplace_back(MakeCompleteFuture<TypeParam>());
   futures.emplace_back(promise.get_future());
   futures.emplace_back(MakeCompleteFuture<TypeParam>());

   // Act
   auto anyFuture = when_any(futures.begin(), futures.end());

   // Assert
   EXPECT_TRUE(anyFuture.valid());
   EXPECT_TRUE(anyFuture.is_ready());

   CompletePromise(promise);
}

TYPED_TEST(EventualTest, WhenAnyIterator_CompletesAfterReturning)
{
   // Arrange
   promise<TypeParam> promise1;
   promise<TypeParam> promise2;
   std::vector<future<TypeParam>> futures;
   futures.emplace_back(promise1.get_future());
   futures.emplace_back(promise2.get_future());
   auto anyFuture = when_any(futures.begin(), futures.end());
   EXPECT_TRUE(anyFuture.valid());
   EXPECT_FALSE(anyFuture.is_ready());

   // Act
   CompletePromise(promise1);

   // Assert
   EXPECT_TRUE(anyFuture.is_ready());

   CompletePromise(promise2);
}

TYPED_TEST(EventualTest, WhenAnyIterator_ReturnsIndexOfCompleteFuture)
{
   // Arrange
   promise<TypeParam> promise0;
   promise<TypeParam> promise1;
   promise<TypeParam> promise2;
   promise<TypeParam> promise3;
   promise<TypeParam> promise4;
   std::vector<future<TypeParam>> futures;
   futures.emplace_back(promise0.get_future());
   futures.emplace_back(promise1.get_future());
   futures.emplace_back(promise2.get_future());
   futures.emplace_back(promise3.get_future());
   futures.emplace_back(promise4.get_future());

   // Act
   auto anyFuture = when_any(futures.begin(), futures.end());
   CompletePromise(promise3);

   // Assert
   auto results = anyFuture.get();
   EXPECT_EQ(3, results.index);

   CompletePromise(promise0, promise1, promise2, promise4);
}

TYPED_TEST(EventualTest, WhenAnyVeradic_ReturnsIncompleteFuture_WhenNoneAreComplete)
{
   // Arrange
   promise<TypeParam> promise1;
   promise<TypeParam> promise2;
   promise<TypeParam> promise3;
   promise<TypeParam> promise4;
   promise<TypeParam> promise5;

   // Act
   auto anyFuture = when_any(
      promise1.get_future(),
      promise2.get_future(),
      promise3.get_future(),
      promise4.get_future(),
      promise5.get_future());

   // Assert
   EXPECT_TRUE(anyFuture.valid());
   EXPECT_FALSE(anyFuture.is_ready());

   CompletePromise(promise1, promise2, promise3, promise4, promise5);
}

TYPED_TEST(EventualTest, WhenAnyVeradic_ReturnsCompleteFuture_WhenAllAreComplete)
{
   // Act
   auto anyFuture = when_any(
      MakeCompleteFuture<TypeParam>(),
      MakeCompleteFuture<TypeParam>(),
      MakeCompleteFuture<TypeParam>(),
      MakeCompleteFuture<TypeParam>(),
      MakeCompleteFuture<TypeParam>());

   // Assert
   EXPECT_TRUE(anyFuture.valid());
   EXPECT_TRUE(anyFuture.is_ready());
}

TYPED_TEST(EventualTest, WhenAnyVeradic_ReturnsCompleteFuture_WhenPartiallyComplete)
{
   // Arrange
   promise<TypeParam> promise;

   // Act
   auto anyFuture = when_any(
      MakeCompleteFuture<TypeParam>(),
      MakeCompleteFuture<TypeParam>(),
      MakeCompleteFuture<TypeParam>(),
      promise.get_future(),
      MakeCompleteFuture<TypeParam>());

   // Assert
   EXPECT_TRUE(anyFuture.valid());
   EXPECT_TRUE(anyFuture.is_ready());
   
   CompletePromise(promise);
}

TYPED_TEST(EventualTest, WhenAnyVeradic_CompletesAfterReturning)
{
   // Arrange
   promise<TypeParam> promise1;
   promise<TypeParam> promise2;
   auto anyFuture = when_any(promise1.get_future(), promise2.get_future());
   EXPECT_TRUE(anyFuture.valid());
   EXPECT_FALSE(anyFuture.is_ready());

   // Act
   CompletePromise(promise1);

   // Assert
   EXPECT_TRUE(anyFuture.is_ready());

   CompletePromise(promise2);
}

TYPED_TEST(EventualTest, WhenAnyVeradic_ReturnsIndexOfCompleteFuture)
{
   // Arrange
   promise<TypeParam> promise0;
   promise<TypeParam> promise1;
   promise<TypeParam> promise2;
   promise<TypeParam> promise3;
   promise<TypeParam> promise4;

   // Act
   auto anyFuture = when_any(
      promise0.get_future(),
      promise1.get_future(),
      promise2.get_future(),
      promise3.get_future(),
      promise4.get_future());
   CompletePromise(promise3);

   // Assert
   auto results = anyFuture.get();
   EXPECT_EQ(3, results.index);

   CompletePromise(promise0, promise1, promise2, promise4);
}

TEST(EventualTest_WhenAny, WhenAllVeradic_ReturnsCompleteFuture_WhenNoFuturesAreProvided)
{
   // Act
   future<std::tuple<>> allFuture = when_all();

   // Assert
   EXPECT_TRUE(allFuture.valid());
   EXPECT_TRUE(allFuture.is_ready());
}

TEST(EventualTest_WhenAny, WhenAnyVeradic_ReturnsCompleteFuture_WhenNoFuturesAreProvided)
{
   // Arrange
   auto expectedIndex = static_cast<size_t>(-1);
   
   // Act
   future<when_any_result<std::tuple<>>> allFuture = when_any();

   // Assert
   EXPECT_TRUE(allFuture.valid());
   EXPECT_TRUE(allFuture.is_ready());
   EXPECT_EQ(expectedIndex, allFuture.get().index);
}

TEST(EventualTest_WhenAll, WhenAllIterator_MaintainsResultOrderInVector)
{
   // Arrange
    promise<int> promise;
   std::vector<future<int>> futures;
   futures.emplace_back(make_ready_future(0));
   futures.emplace_back(make_ready_future(1));
   futures.emplace_back(make_ready_future(2));
   futures.emplace_back(promise.get_future());
   futures.emplace_back(make_ready_future(4));

   // Act
   auto allFuture = when_all(futures.begin(), futures.end());
   promise.set_value(3);

   // Assert
   auto results = allFuture.get();

   int idx = 0;
   for (auto& future : results)
   {
      EXPECT_EQ(idx, future.get());
      idx++;
   }
}

TEST(EventualTest_WhenAll, WhenAllVeradic_MaintainsResultOrderInTuple)
{
   // Arrange
    promise<int> promise;

   // Act
   auto allFuture = when_all(
      make_ready_future(0),
      make_ready_future(1),
      make_ready_future(2),
      promise.get_future(),
      make_ready_future(4));
   promise.set_value(3);

   // Assert
   auto results = allFuture.get();

   EXPECT_EQ(0, std::get<0>(results).get());
   EXPECT_EQ(1, std::get<1>(results).get());
   EXPECT_EQ(2, std::get<2>(results).get());
   EXPECT_EQ(3, std::get<3>(results).get());
   EXPECT_EQ(4, std::get<4>(results).get());
}

TEST(EventualTest_WhenAny, WhenAnyIterator_MaintainsResultOrderInVector)
{
   // Arrange
    promise<int> promise;
   std::vector<future<int>> futures;
   futures.emplace_back(make_ready_future(0));
   futures.emplace_back(make_ready_future(1));
   futures.emplace_back(make_ready_future(2));
   futures.emplace_back(promise.get_future());
   futures.emplace_back(make_ready_future(4));

   // Act
   auto anyFuture = when_any(futures.begin(), futures.end());
   promise.set_value(3);

   // Assert
   auto results = anyFuture.get();

   int idx = 0;
   for (auto& future : results.futures)
   {
      EXPECT_EQ(idx, future.get());
      idx++;
   }
}

TEST(EventualTest_WhenAny, WhenAnyVeradic_MaintainsResultOrderInTuple)
{
   // Arrange
    promise<int> promise;

   // Act
   auto anyFuture = when_any(
      make_ready_future(0),
      make_ready_future(1),
      make_ready_future(2),
      promise.get_future(),
      make_ready_future(4));
   promise.set_value(3);

   // Assert
   auto results = anyFuture.get();

   EXPECT_EQ(0, std::get<0>(results.futures).get());
   EXPECT_EQ(1, std::get<1>(results.futures).get());
   EXPECT_EQ(2, std::get<2>(results.futures).get());
   EXPECT_EQ(3, std::get<3>(results.futures).get());
   EXPECT_EQ(4, std::get<4>(results.futures).get());
}

TEST(EventualTest_Value, MakeReadyFuture_ReturnsACompleteFuture)
{
   // Act
   auto future = make_ready_future<int>(3);

   // Assert
   EXPECT_TRUE(future.valid());
   EXPECT_TRUE(future.is_ready());
   EXPECT_EQ(3, future.get());
}

TEST(EventualTest_Value, MakeReadyFuture_MoveValue_ReturnsACompleteFuture)
{
   // Arrange
   auto nc = NonCopyable();
   nc._markerValue = 1313;
   
   // Act
   auto future = make_ready_future(std::move(nc));

   // Assert
   EXPECT_TRUE(future.valid());
   EXPECT_TRUE(future.is_ready());
   EXPECT_EQ(1313, future.get()._markerValue);
}

TEST(EventualTest_Reference, MakeReadyFuture_ReturnsACompleteFuture)
{
   // Arrange
   int expected = 489;
   auto expectedAddress = &expected;

   // Act
   auto future = make_ready_future(std::reference_wrapper<int>(expected));

   // Assert
   EXPECT_TRUE(future.valid());
   EXPECT_TRUE(future.is_ready());
   auto& result = future.get();
   auto actualAddress = &result;
   EXPECT_EQ(expectedAddress, actualAddress);
}

TEST(EventualTest_Void, MakeReadyFuture_ReturnsACompleteFuture)
{
   // Act
   auto future = make_ready_future();

   // Assert
   EXPECT_TRUE(future.valid());
   EXPECT_TRUE(future.is_ready());
   EXPECT_NO_THROW(future.get());
}