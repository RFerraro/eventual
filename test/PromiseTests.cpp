#include "stdafx.h"
#include <exception>
#include <thread>
#include <eventual\eventual.h>
#include "BasicAllocator.h"
#include "NonCopyable.h"

using namespace eventual;

namespace
{
   class PromiseTestException { };

   template<class R>
   void CompletePromise(Promise<R>& promise)
   {
      struct Anonymous { };
      promise.Set_Exception(std::make_exception_ptr(Anonymous()));
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
   Promise<TypeParam> promise;

   // Assert
   EXPECT_NO_THROW(promise.Get_Future()) << "Promise CTor did not create a valid state.";
}

TYPED_TEST(PromiseTest, CustomAllocatorConstructorCreatesValidState)
{
   // Arrange
   auto alloc = BasicAllocator<int>();

   // Act
   Promise<TypeParam> promise(std::allocator_arg_t(), alloc);
   
   // Assert
   EXPECT_NO_THROW(promise.Get_Future()) << "Promise CTor did not create a valid state.";
   EXPECT_EQ(1, alloc.GetCount()) << "Custom allocator did not detect any heap creation.";
}

TYPED_TEST(PromiseTest, IsMoveConstructable)
{
   // Arrange
   Promise<TypeParam> promise;

   // Act
   Promise<TypeParam> other(std::move(promise));

   // Assert
   EXPECT_NO_THROW(other.Get_Future()) << "Promise move CTor did not transfer a valid state.";
   EXPECT_THROW(promise.Get_Future(), future_error) << "Original promise still has valid state.";
}

TYPED_TEST(PromiseTest, IsNotCopyConstructable)
{
   // Assert
   EXPECT_FALSE(std::is_copy_constructible<Promise<TypeParam>>::value) << "Promise should not be copy constructable.";
}

TYPED_TEST(PromiseTest, DestructorReleasesState)
{
   // Arrange
   auto alloc = BasicAllocator<int>();

   {
      Promise<TypeParam> promise(std::allocator_arg_t(), alloc);
      CompletePromise(promise);

   } // Act
   
   // Assert
   EXPECT_EQ(0, alloc.GetCount()) << "Custom allocator did not deallocate all the objects it created.";
}

TYPED_TEST(PromiseTest, DestructorSignalsBrokenPromiseIfNotComplete)
{
   // Arrange
   Future<TypeParam> future;
   {
      Promise<TypeParam> promise;
      future = promise.Get_Future();
   } // Act

   // Assert
   EXPECT_THROW(future.Get(), future_error) << "Promise did not signal a broken promise status.";
}

TYPED_TEST(PromiseTest, IsMoveAssignable)
{
   // Arrange
   Promise<TypeParam> promise;

   // Act
   Promise<TypeParam> other = std::move(promise);

   // Assert
   EXPECT_NO_THROW(other.Get_Future()) << "Promise move CTor did not transfer a valid state.";
   EXPECT_THROW(promise.Get_Future(), future_error) << "Original promise still has valid state.";
}

TYPED_TEST(PromiseTest, IsNotCopyAssignable)
{
   // Assert
   EXPECT_FALSE(std::is_copy_assignable<Promise<TypeParam>>::value) << "Promise should not be copy assignable.";
}

TYPED_TEST(PromiseTest, IsSwapable)
{
   // Arrange
   Promise<TypeParam> promise;
   Promise<TypeParam> moved(std::move(promise)); // transfers state.

   // Act
   promise.Swap(moved); // transfers back

   // Assert
   EXPECT_NO_THROW(promise.Get_Future()) << "Swap did not transfer a valid state.";
   EXPECT_THROW(moved.Get_Future(), future_error) << "Swap source still has valid state.";
}

TYPED_TEST(PromiseTest, STD_IsSwapable)
{
   // Arrange
   Promise<TypeParam> promise;
   Promise<TypeParam> moved(std::move(promise)); // transfers state.

   // Act
   std::swap(promise, moved); // transfers back

   // Assert
   EXPECT_NO_THROW(promise.Get_Future()) << "Swap did not transfer a valid state.";
   EXPECT_THROW(moved.Get_Future(), future_error) << "Swap source still has valid state.";
}

TYPED_TEST(PromiseTest, GetFuture_ReturnsValidFuture)
{
   // Arrange
   Promise<TypeParam> promise;

   // Act
   auto future = promise.Get_Future();

   // Assert
   EXPECT_TRUE(future.Valid()) << "Future does not have a valid state.";
}

TYPED_TEST(PromiseTest, GetFuture_ThrowsIfInvalid)
{
   // Arrange
   Promise<TypeParam> promise;
   Promise<TypeParam> moved(std::move(promise));

   // Act/Assert
   EXPECT_THROW(promise.Get_Future(), future_error) << "Promise returned a future, despite having an invalid state.";
}

TYPED_TEST(PromiseTest, GetFuture_ThrowsIfCalledMoreThanOnce)
{
   // Arrange
   Promise<TypeParam> promise;
   promise.Get_Future();

   // Act/Assert
   EXPECT_THROW(promise.Get_Future(), future_error) << "Promise returned a future multiple times.";
}

TYPED_TEST(PromiseTest, SetException_SetsAnExceptionPtrInState)
{
   // Arrange
   Promise<TypeParam> promise;

   // Act
   promise.Set_Exception(std::make_exception_ptr(PromiseTestException()));

   // Assert
   EXPECT_THROW(promise.Get_Future().Get(), PromiseTestException) << "Promise did not set the correct exception.";
}

TYPED_TEST(PromiseTest, SetException_ThrowsIfStateIsInvalid)
{
   // Arrange
   Promise<TypeParam> promise;
   Promise<TypeParam> moved(std::move(promise));

   // Act/Assert
   EXPECT_THROW(promise.Set_Exception(std::make_exception_ptr(PromiseTestException())), future_error) << "Promise set an exception on invalid state.";
}

TYPED_TEST(PromiseTest, SetException_ThrowsIfPromiseSatisfied)
{
   // Arrange
   auto exPtr = std::make_exception_ptr(PromiseTestException());
   Promise<TypeParam> promise;
   promise.Set_Exception(exPtr);

   // Act/Assert
   EXPECT_THROW(promise.Set_Exception(exPtr), future_error) << "Promise set an exception when already satisfied.";
}

TYPED_TEST(PromiseTest, SetExceptionAtThreadExit_SetsAnExceptionPtrInState)
{
   // Arrange
   Promise<TypeParam> promise;

   std::mutex m;
   std::condition_variable cv;
   bool ready = false;

   // Act
   std::thread worker([&]()
   {
      promise.Set_Exception_At_Thread_Exit(std::make_exception_ptr(PromiseTestException()));

      std::unique_lock<std::mutex> l(m);
      cv.wait(l, [&]() { return ready; });
   });

   // Assert
   auto future = promise.Get_Future();
   EXPECT_FALSE(future.Is_Ready()) << "The value changed signal was set prematurely.";
   {
      std::unique_lock<std::mutex> l(m);
      ready = true;
   }
   cv.notify_all();
   worker.join();
   EXPECT_THROW(future.Get(), PromiseTestException) << "Promise did not set the correct exception.";
}

TYPED_TEST(PromiseTest, SetExceptionAtThreadExit_ThrowsIfStateIsInvalid)
{
   // Arrange
   Promise<TypeParam> promise;
   Promise<TypeParam> moved(std::move(promise));
   std::atomic_bool result(false);

   std::thread worker([&]()
   {
      try
      {
         promise.Set_Exception_At_Thread_Exit(std::make_exception_ptr(PromiseTestException()));
      }
      catch (const future_error&)
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
   Promise<TypeParam> promise;
   std::atomic_bool result(false);

   // Act
   std::thread worker([&]()
   {
      try
      {
         promise.Set_Exception_At_Thread_Exit(exPtr);
      }
      catch (...) { }

      try
      {
         promise.Set_Exception_At_Thread_Exit(exPtr);
      }
      catch (const future_error&)
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
   Promise<int> promise;

   // Act
   promise.Set_Value(original);
   original++;

   // Assert
   auto actual = promise.Get_Future().Get();
   EXPECT_EQ(expected, actual) << "The returned value was not what was expected.";
}

TEST(PromiseTest_Value, SetValue_ThrowsIfCopiedValueIntoStateMultipleTimes)
{
   // Arrange
   Promise<int> promise;
   promise.Set_Value(3);

   // Act/Assert
   EXPECT_THROW(promise.Set_Value(4), future_error) << "SetValue allowed changing the value after being set.";
}

TEST(PromiseTest_Value, SetValue_CopiesValueThrowsIfStateIsInvalid)
{
   // Arrange
   Promise<int> promise;
   Promise<int> moved(std::move(promise));

   // Act/Assert
   EXPECT_THROW(promise.Set_Value(1), future_error) << "Set_Value did not throw an error when setting a value to an invalid promise.";
}

TEST(PromiseTest_Value, SetValue_MovesValueIntoState)
{
   // Arrange
   int expectedValue = 483;
   NonCopyable original = NonCopyable();
   original._markerValue = expectedValue;
   Promise<NonCopyable> promise;

   // Act
   promise.Set_Value(std::move(original));

   // Assert
   auto result = promise.Get_Future().Get();
   EXPECT_NE(expectedValue, original._markerValue) << "SetValue did not move the object as an RValue.";
   EXPECT_EQ(expectedValue, result._markerValue) << "The expected value was not returned.";
}

TEST(PromiseTest_Value, SetValue_ThrowsIfMovedValueIntoStateMultipleTimes)
{
   // Arrange
   Promise<NonCopyable> promise;
   promise.Set_Value(NonCopyable());

   // Act/Assert
   EXPECT_THROW(promise.Set_Value(NonCopyable()), future_error) << "SetValue allowed changing the value after being set.";
}

TEST(PromiseTest_Value, SetValue_MoveValueThrowsIfStateIsInvalid)
{
   // Arrange
   Promise<NonCopyable> promise;
   Promise<NonCopyable> moved(std::move(promise));

   // Act/Assert
   EXPECT_THROW(promise.Set_Value(NonCopyable()), future_error) << "Set_Value did not throw an error when setting a value to an invalid promise.";
}

TEST(PromiseTest_Reference, SetValue_SetsReferenceInState)
{
   // Arrange
   int value = 1;
   auto expectedPtr = &value;
   Promise<int&> promise;

   // Act
   promise.Set_Value(value);

   // Assert
   auto& result = promise.Get_Future().Get();
   auto actualPtr = &result;
   EXPECT_EQ(expectedPtr, actualPtr) << "SetValue did not set the correct reference.";
}

TEST(PromiseTest_Reference, SetValue_ThrowsIfReferenceSetMultipleTimes)
{
   // Arrange
   int first = 1;
   int second = 1;
   Promise<int&> promise;
   promise.Set_Value(first);

   // Act/Assert
   EXPECT_THROW(promise.Set_Value(second), future_error) << "SetValue allowed changing the reference after being set.";
}

TEST(PromiseTest_Reference, SetValue_ThrowsIfStateIsInvalid)
{
   // Arrange
   int value = 1;
   Promise<int&> promise;
   Promise<int&> moved(std::move(promise));

   // Act/Assert
   EXPECT_THROW(promise.Set_Value(value), future_error) << "Set_Value did not throw an error when setting a value to an invalid promise.";
}

TEST(PromiseTest_Void, SetValue_SignalsCompletion)
{
   // Arrange
   Promise<void> promise;

   // Act
   promise.Set_Value();

   // Assert
   EXPECT_NO_THROW(promise.Get_Future().Get()) << "SetValue did not signal completion.";
}

TEST(PromiseTest_Void, SetValue_ThrowsIfSetMultipleTimes)
{
   // Arrange
   Promise<void> promise;
   promise.Set_Value();

   // Act/Assert
   EXPECT_THROW(promise.Set_Value(), future_error) << "SetValue was called multiple times after being set.";
}

TEST(PromiseTest_Void, SetValue_ThrowsIfStateIsInvalid)
{
   // Arrange
   Promise<void> promise;
   Promise<void> moved(std::move(promise));

   // Act/Assert
   EXPECT_THROW(promise.Set_Value(), future_error) << "Set_Value did not throw an error when setting a value to an invalid promise.";
}

TEST(PromiseTest_Value, SetValueAtThreadExit_CopiesValueIntoState)
{
   // Arrange
   int original = 3;
   const int expected = original;
   Promise<int> promise;

   std::mutex m;
   std::condition_variable cv;
   bool ready = false;

   // Act
   std::thread worker([&]()
   {
      promise.Set_Value_At_Thread_Exit(original);
      original++;

      std::unique_lock<std::mutex> l(m);
      cv.wait(l, [&]() { return ready; });
   });

   // Assert
   auto future = promise.Get_Future();
   EXPECT_FALSE(future.Is_Ready()) << "The value changed signal was set prematurely.";
   {
      std::unique_lock<std::mutex> l(m);
      ready = true;
   }
   cv.notify_all();
   worker.join();
   auto actual = future.Get();
   EXPECT_EQ(expected, actual) << "The returned value was not what was expected.";
}

TEST(PromiseTest_Value, SetValueAtThreadExit_ThrowsIfCopiedValueIntoStateMultipleTimes_RValue)
{
   // Arrange
   Promise<int> promise;
   std::atomic_bool result(false);

   // Act
   std::thread worker([&]()
   {
      try
      {
         promise.Set_Value_At_Thread_Exit(3);
      }
      catch (...) { }

      try
      {
         promise.Set_Value_At_Thread_Exit(4);
      }
      catch (const future_error&)
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
   Promise<int> promise;
   std::atomic_bool result(false);

   // Act
   std::thread worker([&]()
   {
      auto lValue = 4;
      try
      {
         promise.Set_Value_At_Thread_Exit(3);
      }
      catch (...) { }

      try
      {
         promise.Set_Value_At_Thread_Exit(lValue);
      }
      catch (const future_error&)
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
   Promise<int> promise;
   Promise<int> moved(std::move(promise));
   std::atomic_bool result(false);

   // Act
   std::thread worker([&]()
   {
      try
      {
         promise.Set_Value_At_Thread_Exit(1);
      }
      catch (const future_error&)
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
   Promise<NonCopyable> promise;

   std::mutex m;
   std::condition_variable cv;
   bool ready = false;

   // Act
   std::thread worker([&]()
   {
      promise.Set_Value_At_Thread_Exit(std::move(original));

      std::unique_lock<std::mutex> l(m);
      cv.wait(l, [&]() { return ready; });
   });

   // Assert
   auto future = promise.Get_Future();
   EXPECT_FALSE(future.Is_Ready()) << "The value changed signal was set prematurely.";
   {
      std::unique_lock<std::mutex> l(m);
      ready = true;
   }
   cv.notify_all();
   worker.join();
   auto result = future.Get();
   EXPECT_NE(expectedValue, original._markerValue) << "SetValue did not move the object as an RValue.";
   EXPECT_EQ(expectedValue, result._markerValue) << "The expected value was not returned.";
}

TEST(PromiseTest_Value, SetValueAtThreadExit_ThrowsIfMovedValueIntoStateMultipleTimes)
{
   // Arrange
   Promise<NonCopyable> promise;
   std::atomic_bool result(false);

   // Act
   std::thread worker([&]()
   {
      try
      {
         promise.Set_Value_At_Thread_Exit(NonCopyable());
      }
      catch (...) { }

      try
      {
         promise.Set_Value_At_Thread_Exit(NonCopyable());
      }
      catch (const future_error&)
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
   Promise<NonCopyable> promise;
   Promise<NonCopyable> moved(std::move(promise));
   std::atomic_bool result(false);

   // Act
   std::thread worker([&]()
   {
      try
      {
         promise.Set_Value_At_Thread_Exit(NonCopyable());
      }
      catch (const future_error&)
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
   Promise<int&> promise;

   std::mutex m;
   std::condition_variable cv;
   bool ready = false;

   // Act
   std::thread worker([&]()
   {
      promise.Set_Value_At_Thread_Exit(value);

      std::unique_lock<std::mutex> l(m);
      cv.wait(l, [&]() { return ready; });
   });

   // Assert
   auto future = promise.Get_Future();
   EXPECT_FALSE(future.Is_Ready()) << "The value changed signal was set prematurely.";
   {
      std::unique_lock<std::mutex> l(m);
      ready = true;
   }
   cv.notify_all();
   worker.join();
   auto& result = future.Get();
   auto actualPtr = &result;
   EXPECT_EQ(expectedPtr, actualPtr) << "SetValue did not set the correct reference.";
}

TEST(PromiseTest_Reference, SetValueAtThreadExit_ThrowsIfReferenceSetMultipleTimes)
{
   // Arrange
   int first = 1;
   int second = 1;
   Promise<int&> promise;
   std::atomic_bool result(false);

   // Act
   std::thread worker([&]()
   {
      try
      {
         promise.Set_Value_At_Thread_Exit(first);
      }
      catch (...) { }

      try
      {
         promise.Set_Value_At_Thread_Exit(second);
      }
      catch (const future_error&)
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
   Promise<int&> promise;
   Promise<int&> moved(std::move(promise));
   std::atomic_bool result(false);

   // Act
   std::thread worker([&]()
   {
      try
      {
         promise.Set_Value_At_Thread_Exit(value);
      }
      catch (const future_error&)
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
   Promise<void> promise;

   std::mutex m;
   std::condition_variable cv;
   bool ready = false;

   // Act
   std::thread worker([&]()
   {
      promise.Set_Value_At_Thread_Exit();

      std::unique_lock<std::mutex> l(m);
      cv.wait(l, [&]() { return ready; });
   });

   // Assert
   auto future = promise.Get_Future();
   EXPECT_FALSE(future.Is_Ready()) << "The value changed signal was set prematurely.";
   {
      std::unique_lock<std::mutex> l(m);
      ready = true;
   }
   cv.notify_all();
   worker.join();
   EXPECT_NO_THROW(future.Get()) << "SetValue did not signal completion.";
}

TEST(PromiseTest_Void, SetValueAtThreadExit_ThrowsIfSetMultipleTimes)
{
   // Arrange
   Promise<void> promise;
   std::atomic_bool result(false);

   // Act
   std::thread worker([&]()
   {
      try
      {
         promise.Set_Value_At_Thread_Exit();
      }
      catch (...) { }

      try
      {
         promise.Set_Value_At_Thread_Exit();
      }
      catch (const future_error&)
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
   Promise<void> promise;
   Promise<void> moved(std::move(promise));
   std::atomic_bool result(false);

   // Act
   std::thread worker([&]()
   {
      try
      {
         promise.Set_Value_At_Thread_Exit();
      }
      catch (const future_error&)
      {
         result.store(true);
      }
   });
   worker.join();

   // Assert
   EXPECT_TRUE(result.load()) << "Set_Value did not throw an error when setting a value to an invalid promise.";
}
