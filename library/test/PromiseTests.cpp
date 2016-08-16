#include "stdafx.h"
#include <exception>
#include <thread>
#include <eventual/eventual.h>
#include "BasicAllocator.h"
#include "NonCopyable.h"

using namespace eventual;

namespace
{
   class PromiseTestException { };

   template<class R>
   void CompletePromise(promise<R>& promise)
   {
      struct Anonymous { };
      promise.set_exception(std::make_exception_ptr(Anonymous()));
   }
}

// Typed Tests
template<typename T>
class PromiseTest : public testing::Test { };
typedef testing::Types<void, int, int&> PromiseReturnTypes;
TYPED_TEST_CASE(PromiseTest, PromiseReturnTypes); // ignore intellisense warning


TYPED_TEST(PromiseTest, DefaultConstructorCreatesValidState)
{
   // Arrange/Act
   promise<TypeParam> promise;

   // Assert
   EXPECT_NO_THROW(promise.get_future()) << "Promise CTor did not create a valid state.";
}

TYPED_TEST(PromiseTest, CustomAllocatorConstructorCreatesValidState)
{
   // Arrange
   auto alloc = BasicAllocator<int>();

   // Act
   promise<TypeParam> promise(std::allocator_arg_t(), alloc);
   
   // Assert
   EXPECT_NO_THROW(promise.get_future()) << "Promise CTor did not create a valid state.";
   EXPECT_GT(alloc.GetCount(), 0) << "Custom allocator did not detect any heap creation.";
}

TYPED_TEST(PromiseTest, IsMoveConstructable)
{
   // Arrange
   promise<TypeParam> target;

   // Act
   promise<TypeParam> other(std::move(target));

   // Assert
   EXPECT_NO_THROW(other.get_future()) << "Promise move CTor did not transfer a valid state.";
   EXPECT_THROW(target.get_future(), std::future_error) << "Original promise still has valid state.";
}

TYPED_TEST(PromiseTest, IsNotCopyConstructable)
{
   // Assert
   EXPECT_FALSE(std::is_copy_constructible<promise<TypeParam>>::value) << "Promise should not be copy constructable.";
}

TYPED_TEST(PromiseTest, DestructorReleasesState)
{
   // Arrange
   auto alloc = BasicAllocator<int>();

   {
      promise<TypeParam> promise(std::allocator_arg_t(), alloc);
      CompletePromise(promise);

   } // Act
   
   // Assert
   EXPECT_EQ(0, alloc.GetCount()) << "Custom allocator did not deallocate all the objects it created.";
}

TYPED_TEST(PromiseTest, DestructorSignalsBrokenPromiseIfNotComplete)
{
   // Arrange
   future<TypeParam> future;
   {
      promise<TypeParam> promise;
      future = promise.get_future();
   } // Act

   // Assert
   EXPECT_THROW(future.get(), std::future_error) << "Promise did not signal a broken promise status.";
}

TYPED_TEST(PromiseTest, IsMoveAssignable)
{
   // Arrange
   promise<TypeParam> promise;

   // Act
   auto other = std::move(promise);

   // Assert
   EXPECT_NO_THROW(other.get_future()) << "Promise move CTor did not transfer a valid state.";
   EXPECT_THROW(promise.get_future(), std::future_error) << "Original promise still has valid state.";
}

TYPED_TEST(PromiseTest, IsNotCopyAssignable)
{
   // Assert
   EXPECT_FALSE(std::is_copy_assignable<promise<TypeParam>>::value) << "Promise should not be copy assignable.";
}

TYPED_TEST(PromiseTest, IsSwapable)
{
   // Arrange
   promise<TypeParam> target;
   promise<TypeParam> moved(std::move(target)); // transfers state.

   // Act
   target.swap(moved); // transfers back

   // Assert
   EXPECT_NO_THROW(target.get_future()) << "Swap did not transfer a valid state.";
   EXPECT_THROW(moved.get_future(), std::future_error) << "Swap source still has valid state.";
}

TYPED_TEST(PromiseTest, STD_IsSwapable)
{
   // Arrange
   promise<TypeParam> target;
   promise<TypeParam> moved(std::move(target)); // transfers state.

   // Act
   std::swap(target, moved); // transfers back

   // Assert
   EXPECT_NO_THROW(target.get_future()) << "Swap did not transfer a valid state.";
   EXPECT_THROW(moved.get_future(), std::future_error) << "Swap source still has valid state.";
}

TYPED_TEST(PromiseTest, GetFuture_ReturnsValidFuture)
{
   // Arrange
   promise<TypeParam> promise;

   // Act
   auto future = promise.get_future();

   // Assert
   EXPECT_TRUE(future.valid()) << "Future does not have a valid state.";
}

TYPED_TEST(PromiseTest, GetFuture_ThrowsIfInvalid)
{
   // Arrange
   promise<TypeParam> target;
   promise<TypeParam> moved(std::move(target));

   // Act/Assert
   EXPECT_THROW(target.get_future(), std::future_error) << "Promise returned a future, despite having an invalid state.";
}

TYPED_TEST(PromiseTest, GetFuture_ThrowsIfCalledMoreThanOnce)
{
   // Arrange
   promise<TypeParam> promise;
   promise.get_future();

   // Act/Assert
   EXPECT_THROW(promise.get_future(), std::future_error) << "Promise returned a future multiple times.";
}

TYPED_TEST(PromiseTest, SetException_SetsAnExceptionPtrInState)
{
   // Arrange
   promise<TypeParam> promise;

   // Act
   promise.set_exception(std::make_exception_ptr(PromiseTestException()));

   // Assert
   EXPECT_THROW(promise.get_future().get(), PromiseTestException) << "Promise did not set the correct exception.";
}

TYPED_TEST(PromiseTest, SetException_ThrowsIfStateIsInvalid)
{
   // Arrange
   promise<TypeParam> target;
   promise<TypeParam> moved(std::move(target));

   // Act/Assert
   EXPECT_THROW(target.set_exception(std::make_exception_ptr(PromiseTestException())), std::future_error) << "Promise set an exception on invalid state.";
}

TYPED_TEST(PromiseTest, SetException_ThrowsIfPromiseSatisfied)
{
   // Arrange
   auto exPtr = std::make_exception_ptr(PromiseTestException());
   promise<TypeParam> promise;
   promise.set_exception(exPtr);

   // Act/Assert
   EXPECT_THROW(promise.set_exception(exPtr), std::future_error) << "Promise set an exception when already satisfied.";
}

TYPED_TEST(PromiseTest, SetExceptionAtThreadExit_SetsAnExceptionPtrInState)
{
   // Arrange
   promise<TypeParam> promise;

   std::mutex m;
   std::condition_variable cv;
   bool ready = false;

   // Act
   std::thread worker([&]()
   {
      promise.set_exception_at_thread_exit(std::make_exception_ptr(PromiseTestException()));

      std::unique_lock<std::mutex> l(m);
      cv.wait(l, [&]() { return ready; });
   });

   // Assert
   auto future = promise.get_future();
   EXPECT_FALSE(future.is_ready()) << "The value changed signal was set prematurely.";
   {
      std::unique_lock<std::mutex> l(m);
      ready = true;
   }
   cv.notify_all();
   worker.join();
   EXPECT_THROW(future.get(), PromiseTestException) << "Promise did not set the correct exception.";
}

TYPED_TEST(PromiseTest, SetExceptionAtThreadExit_ThrowsIfStateIsInvalid)
{
   // Arrange
   promise<TypeParam> target;
   promise<TypeParam> moved(std::move(target));
   std::atomic_bool result(false);

   std::thread worker([&]()
   {
      try
      {
          target.set_exception_at_thread_exit(std::make_exception_ptr(PromiseTestException()));
      }
      catch (const std::future_error&)
      {
         result.store(true);
      }
   });
   worker.join();

   // Act/Assert
   EXPECT_TRUE(result.load()) << "Promise set an exception on invalid state.";
}

TYPED_TEST(PromiseTest, SetExceptionAtThreadExit_ThrowsIfPromiseSatisfied)
{
   // Arrange
   auto exPtr = std::make_exception_ptr(PromiseTestException());
   promise<TypeParam> promise;
   std::atomic_bool result(false);

   // Act
   std::thread worker([&]()
   {
      try
      {
         promise.set_exception_at_thread_exit(exPtr);
      }
      catch (...) { }

      try
      {
         promise.set_exception_at_thread_exit(exPtr);
      }
      catch (const std::future_error&)
      {
         result.store(true);
      }
   });
   worker.join();

   // Assert
   EXPECT_TRUE(result.load()) << "Promise set an exception when already satisfied.";
}

// Set_Value

TEST(PromiseTest_Value, SetValue_CopiesValueIntoState)
{
   // Arrange
   int original = 3;
   const int expected = original;
   promise<int> promise;

   // Act
   promise.set_value(original);
   original++;

   // Assert
   auto actual = promise.get_future().get();
   EXPECT_EQ(expected, actual) << "The returned value was not what was expected.";
}

TEST(PromiseTest_Value, SetValue_ThrowsIfCopiedValueIntoStateMultipleTimes)
{
   // Arrange
   promise<int> promise;
   promise.set_value(3);

   // Act/Assert
   EXPECT_THROW(promise.set_value(4), std::future_error) << "SetValue allowed changing the value after being set.";
}

TEST(PromiseTest_Value, SetValue_CopiesValueThrowsIfStateIsInvalid)
{
   // Arrange
   promise<int> target;
   promise<int> moved(std::move(target));

   // Act/Assert
   EXPECT_THROW(target.set_value(1), std::future_error) << "Set_Value did not throw an error when setting a value to an invalid promise.";
}

TEST(PromiseTest_Value, SetValue_MovesValueIntoState)
{
   // Arrange
   int expectedValue = 483;
   NonCopyable original = NonCopyable();
   original._markerValue = expectedValue;
   promise<NonCopyable> promise;

   // Act
   promise.set_value(std::move(original));

   // Assert
   auto result = promise.get_future().get();
   EXPECT_NE(expectedValue, original._markerValue) << "SetValue did not move the object as an RValue.";
   EXPECT_EQ(expectedValue, result._markerValue) << "The expected value was not returned.";
}

TEST(PromiseTest_Value, SetValue_ThrowsIfMovedValueIntoStateMultipleTimes)
{
   // Arrange
   promise<NonCopyable> promise;
   promise.set_value(NonCopyable());

   // Act/Assert
   EXPECT_THROW(promise.set_value(NonCopyable()), std::future_error) << "SetValue allowed changing the value after being set.";
}

TEST(PromiseTest_ConstValue, SetValue_ThrowsIfSetMultipleTimes)
{
    // Arrange
    const int value = 13;

    promise<int> promise;
    promise.set_value(value);

    // Act/Assert
    EXPECT_THROW(promise.set_value(value), std::future_error) << "SetValue allowed changing the value after being set.";
}

TEST(PromiseTest_Value, SetValue_MoveValueThrowsIfStateIsInvalid)
{
   // Arrange
   promise<NonCopyable> target;
   promise<NonCopyable> moved(std::move(target));

   // Act/Assert
   EXPECT_THROW(target.set_value(NonCopyable()), std::future_error) << "Set_Value did not throw an error when setting a value to an invalid promise.";
}

TEST(PromiseTest_Reference, SetValue_SetsReferenceInState)
{
   // Arrange
   int value = 1;
   auto expectedPtr = &value;
   promise<int&> promise;

   // Act
   promise.set_value(value);

   // Assert
   auto& result = promise.get_future().get();
   auto actualPtr = &result;
   EXPECT_EQ(expectedPtr, actualPtr) << "SetValue did not set the correct reference.";
}

TEST(PromiseTest_Reference, SetValue_ThrowsIfReferenceSetMultipleTimes)
{
   // Arrange
   int first = 1;
   int second = 1;
   promise<int&> promise;
   promise.set_value(first);

   // Act/Assert
   EXPECT_THROW(promise.set_value(second), std::future_error) << "SetValue allowed changing the reference after being set.";
}

TEST(PromiseTest_Reference, SetValue_ThrowsIfStateIsInvalid)
{
   // Arrange
   int value = 1;
   promise<int&> target;
   promise<int&> moved(std::move(target));

   // Act/Assert
   EXPECT_THROW(target.set_value(value), std::future_error) << "Set_Value did not throw an error when setting a value to an invalid promise.";
}

TEST(PromiseTest_Void, SetValue_SignalsCompletion)
{
   // Arrange
   promise<void> promise;

   // Act
   promise.set_value();

   // Assert
   EXPECT_NO_THROW(promise.get_future().get()) << "SetValue did not signal completion.";
}

TEST(PromiseTest_Void, SetValue_ThrowsIfSetMultipleTimes)
{
   // Arrange
   promise<void> promise;
   promise.set_value();

   // Act/Assert
   EXPECT_THROW(promise.set_value(), std::future_error) << "SetValue was called multiple times after being set.";
}

TEST(PromiseTest_Void, SetValue_ThrowsIfStateIsInvalid)
{
   // Arrange
   promise<void> target;
   promise<void> moved(std::move(target));

   // Act/Assert
   EXPECT_THROW(target.set_value(), std::future_error) << "Set_Value did not throw an error when setting a value to an invalid promise.";
}

TEST(PromiseTest_Value, SetValueAtThreadExit_CopiesValueIntoState)
{
   // Arrange
   int original = 3;
   const int expected = original;
   promise<int> promise;

   std::mutex m;
   std::condition_variable cv;
   bool ready = false;

   // Act
   std::thread worker([&]()
   {
      promise.set_value_at_thread_exit(original);
      original++;

      std::unique_lock<std::mutex> l(m);
      cv.wait(l, [&]() { return ready; });
   });

   // Assert
   auto future = promise.get_future();
   EXPECT_FALSE(future.is_ready()) << "The value changed signal was set prematurely.";
   {
      std::unique_lock<std::mutex> l(m);
      ready = true;
   }
   cv.notify_all();
   worker.join();
   auto actual = future.get();
   EXPECT_EQ(expected, actual) << "The returned value was not what was expected.";
}

TEST(PromiseTest_Value, SetValueAtThreadExit_ThrowsIfCopiedValueIntoStateMultipleTimes_RValue)
{
   // Arrange
   promise<int> promise;
   std::atomic_bool result(false);

   // Act
   std::thread worker([&]()
   {
      try
      {
         promise.set_value_at_thread_exit(3);
      }
      catch (...) { }

      try
      {
         promise.set_value_at_thread_exit(4);
      }
      catch (const std::future_error&)
      {
         result.store(true);
      }
   });
   worker.join();

   // Assert
   EXPECT_TRUE(result.load()) << "SetValue allowed changing the value after being set.";
}

TEST(PromiseTest_Value, SetValueAtThreadExit_ThrowsIfCopiedValueIntoStateMultipleTimes_LValue)
{
   // Arrange
   promise<int> promise;
   std::atomic_bool result(false);

   // Act
   std::thread worker([&]()
   {
      auto lValue = 4;
      try
      {
         promise.set_value_at_thread_exit(3);
      }
      catch (...) { }

      try
      {
         promise.set_value_at_thread_exit(lValue);
      }
      catch (const std::future_error&)
      {
         result.store(true);
      }
   });
   worker.join();

   // Assert
   EXPECT_TRUE(result.load()) << "SetValue allowed changing the value after being set.";
}

TEST(PromiseTest_Value, SetValueAtThreadExit_CopiesValueThrowsIfStateIsInvalid)
{
   // Arrange
   promise<int> target;
   promise<int> moved(std::move(target));
   std::atomic_bool result(false);

   // Act
   std::thread worker([&]()
   {
      try
      {
          target.set_value_at_thread_exit(1);
      }
      catch (const std::future_error&)
      {
         result.store(true);
      }
   });
   worker.join();

   // Assert
   EXPECT_TRUE(result.load()) << "Set_Value did not throw an error when setting a value to an invalid promise.";
}

TEST(PromiseTest_Value, SetValueAtThreadExit_MovesValueIntoState)
{
   // Arrange
   int expectedValue = 483;
   NonCopyable original = NonCopyable();
   original._markerValue = expectedValue;
   promise<NonCopyable> promise;

   std::mutex m;
   std::condition_variable cv;
   bool ready = false;

   // Act
   std::thread worker([&]()
   {
      promise.set_value_at_thread_exit(std::move(original));

      std::unique_lock<std::mutex> l(m);
      cv.wait(l, [&]() { return ready; });
   });

   // Assert
   auto future = promise.get_future();
   EXPECT_FALSE(future.is_ready()) << "The value changed signal was set prematurely.";
   {
      std::unique_lock<std::mutex> l(m);
      ready = true;
   }
   cv.notify_all();
   worker.join();
   auto result = future.get();
   EXPECT_NE(expectedValue, original._markerValue) << "SetValue did not move the object as an RValue.";
   EXPECT_EQ(expectedValue, result._markerValue) << "The expected value was not returned.";
}

TEST(PromiseTest_Value, SetValueAtThreadExit_ThrowsIfMovedValueIntoStateMultipleTimes)
{
   // Arrange
   promise<NonCopyable> promise;
   std::atomic_bool result(false);

   // Act
   std::thread worker([&]()
   {
      try
      {
         promise.set_value_at_thread_exit(NonCopyable());
      }
      catch (...) { }

      try
      {
         promise.set_value_at_thread_exit(NonCopyable());
      }
      catch (const std::future_error&)
      {
         result.store(true);
      }
   });
   worker.join();

   // Act/Assert
   EXPECT_TRUE(result.load()) << "SetValue allowed changing the value after being set.";
}

TEST(PromiseTest_Value, SetValueAtThreadExit_MoveValueThrowsIfStateIsInvalid)
{
   // Arrange
   promise<NonCopyable> target;
   promise<NonCopyable> moved(std::move(target));
   std::atomic_bool result(false);

   // Act
   std::thread worker([&]()
   {
      try
      {
          target.set_value_at_thread_exit(NonCopyable());
      }
      catch (const std::future_error&)
      {
         result.store(true);
      }
   });
   worker.join();

   // Assert
   EXPECT_TRUE(result.load()) << "Set_Value did not throw an error when setting a value to an invalid promise.";
}

TEST(PromiseTest_Reference, SetValueAtThreadExit_SetsReferenceInState)
{
   // Arrange
   int value = 1;
   auto expectedPtr = &value;
   promise<int&> promise;

   std::mutex m;
   std::condition_variable cv;
   bool ready = false;

   // Act
   std::thread worker([&]()
   {
      promise.set_value_at_thread_exit(value);

      std::unique_lock<std::mutex> l(m);
      cv.wait(l, [&]() { return ready; });
   });

   // Assert
   auto future = promise.get_future();
   EXPECT_FALSE(future.is_ready()) << "The value changed signal was set prematurely.";
   {
      std::unique_lock<std::mutex> l(m);
      ready = true;
   }
   cv.notify_all();
   worker.join();
   auto& result = future.get();
   auto actualPtr = &result;
   EXPECT_EQ(expectedPtr, actualPtr) << "SetValue did not set the correct reference.";
}

TEST(PromiseTest_Reference, SetValueAtThreadExit_ThrowsIfReferenceSetMultipleTimes)
{
   // Arrange
   int first = 1;
   int second = 1;
   promise<int&> promise;
   std::atomic_bool result(false);

   // Act
   std::thread worker([&]()
   {
      try
      {
         promise.set_value_at_thread_exit(first);
      }
      catch (...) { }

      try
      {
         promise.set_value_at_thread_exit(second);
      }
      catch (const std::future_error&)
      {
         result.store(true);
      }
   });
   worker.join();

   // Assert
   EXPECT_TRUE(result.load()) << "SetValue allowed changing the reference after being set.";
}

TEST(PromiseTest_Reference, SetValueAtThreadExit_ThrowsIfStateIsInvalid)
{
   // Arrange
   int value = 1;
   promise<int&> target;
   promise<int&> moved(std::move(target));
   std::atomic_bool result(false);

   // Act
   std::thread worker([&]()
   {
      try
      {
          target.set_value_at_thread_exit(value);
      }
      catch (const std::future_error&)
      {
         result.store(true);
      }
   });
   worker.join();

   // Assert
   EXPECT_TRUE(result.load()) << "Set_Value did not throw an error when setting a value to an invalid promise.";
}

TEST(PromiseTest_Void, SetValueAtThreadExit_SignalsCompletion)
{
   // Arrange
   promise<void> promise;

   std::mutex m;
   std::condition_variable cv;
   bool ready = false;

   // Act
   std::thread worker([&]()
   {
      promise.set_value_at_thread_exit();

      std::unique_lock<std::mutex> l(m);
      cv.wait(l, [&]() { return ready; });
   });

   // Assert
   auto future = promise.get_future();
   EXPECT_FALSE(future.is_ready()) << "The value changed signal was set prematurely.";
   {
      std::unique_lock<std::mutex> l(m);
      ready = true;
   }
   cv.notify_all();
   worker.join();
   EXPECT_NO_THROW(future.get()) << "SetValue did not signal completion.";
}

TEST(PromiseTest_Void, SetValueAtThreadExit_ThrowsIfSetMultipleTimes)
{
   // Arrange
   promise<void> promise;
   std::atomic_bool result(false);

   // Act
   std::thread worker([&]()
   {
      try
      {
         promise.set_value_at_thread_exit();
      }
      catch (...) { }

      try
      {
         promise.set_value_at_thread_exit();
      }
      catch (const std::future_error&)
      {
         result.store(true);
      }
   });
   worker.join();

   // Assert
   EXPECT_TRUE(result.load()) << "SetValue was called multiple times after being set.";
}

TEST(PromiseTest_Void, SetValueAtThreadExit_ThrowsIfStateIsInvalid)
{
   // Arrange
   promise<void> target;
   promise<void> moved(std::move(target));
   std::atomic_bool result(false);

   // Act
   std::thread worker([&]()
   {
      try
      {
          target.set_value_at_thread_exit();
      }
      catch (const std::future_error&)
      {
         result.store(true);
      }
   });
   worker.join();

   // Assert
   EXPECT_TRUE(result.load()) << "Set_Value did not throw an error when setting a value to an invalid promise.";
}

TEST(PromiseTest, CanCreateDeeplyNestedFutures)
{
    // Edge case test

    // Arrange
    promise<future<future<future<int>>>> promise;

    // Act
    auto future = promise.get_future();
    future.then([](auto) { });

    // Assert
    SUCCEED();
}
