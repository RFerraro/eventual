#include "stdafx.h"
#include <thread>
#include <eventual\eventual.h>
#include "BasicAllocator.h"

using namespace eventual;

namespace
{
   class PackagedTaskTestException { };
}

TEST(PackagedTaskTest, DefaultConstructorCreatesInvalidTask)
{
   // Arrange
   Packaged_Task<int(int)> task;

   // Act/Assert
   EXPECT_FALSE(task.Valid());
}

TEST(PackagedTaskTest, ConstructorWithFunctorCreatesValidTask)
{
   // Arrange
   Packaged_Task<int(int)> task([](int i) { return i; });

   // Act/Assert
   EXPECT_TRUE(task.Valid());
}

TEST(PackagedTaskTest, ConstructorWithAllocatorCreatesValidTask)
{
   // Arrange
   auto alloc = BasicAllocator<int>();
   Packaged_Task<int(int)> task(std::allocator_arg_t(), alloc, [](int i) { return i; });

   // Act/Assert
   EXPECT_TRUE(task.Valid());
   EXPECT_EQ(2, alloc.GetCount());
}

TEST(PackagedTaskTest, IsMoveConstructable)
{
   // Arrange
   Packaged_Task<int(int)> task([](int i) { return i; });

   // Act
   Packaged_Task<int(int)> moved(std::move(task));

   // Assert
   EXPECT_TRUE(moved.Valid());
   EXPECT_FALSE(task.Valid());
}

TEST(PackagedTaskTest, IsNotCopyConstructable)
{
   // Assert
   EXPECT_FALSE(std::is_copy_constructible<Packaged_Task<int(int)>>::value);
}

TEST(PackagedTaskTest, DestructorReleasesState)
{
   // Arrange
   auto alloc = BasicAllocator<int>();
   {
      Packaged_Task<int(int)> task(std::allocator_arg_t(), alloc, [](int i) { return i; });
      task(3);
   } // Act

     // Assert
   EXPECT_EQ(0, alloc.GetCount());
}

TEST(PackagedTaskTest, DestructorSignalsBrokenPromiseIfNotComplete)
{
   // Arrange
   Future<int> future;
   {
      Packaged_Task<int(int)> task([](int i) { return i; });
      future = task.Get_Future();
   } // Act

     // Assert
   EXPECT_THROW(future.Get(), future_error);
}

TEST(PackagedTaskTest, IsMoveAssignable)
{
   // Arrange
   Packaged_Task<int(int)> task([](int i) { return i; });

   // Act
   Packaged_Task<int(int)> moved = std::move(task);

   // Assert
   EXPECT_TRUE(moved.Valid());
   EXPECT_FALSE(task.Valid());
}

TEST(PackagedTaskTest, IsNotCopyAssignable)
{
   // Assert
   EXPECT_FALSE(std::is_copy_assignable<Packaged_Task<int(int)>>::value);
}

TEST(PackagedTaskTest, Valid_ReturnsTrueIfTaskHasState)
{
   //Arrange
   Packaged_Task<int(int)> task([](int i) { return i; });

   // Act/Assert
   EXPECT_TRUE(task.Valid());
}

TEST(PackagedTaskTest, Valid_ReturnsFalseIfTaskHasNoState)
{
   //Arrange
   Packaged_Task<int(int)> task;

   // Act/Assert
   EXPECT_FALSE(task.Valid());
}

TEST(PackagedTaskTest, IsSwapable)
{
   // Arrange
   Packaged_Task<int(int)> task([](int i) { return i; });
   Packaged_Task<int(int)> moved;

   // Act
   task.Swap(moved);

   // Assert
   EXPECT_TRUE(moved.Valid());
   EXPECT_FALSE(task.Valid());
}

TEST(PackagedTaskTest, STD_IsSwapable)
{
   // Arrange
   Packaged_Task<int(int)> task([](int i) { return i; });
   Packaged_Task<int(int)> moved;

   // Act
   std::swap(task, moved);

   // Assert
   EXPECT_TRUE(moved.Valid());
   EXPECT_FALSE(task.Valid());
}

TEST(PackagedTaskTest, GetFuture_ReturnsValidFuture)
{
   // Arrange
   Packaged_Task<int(int)> task([](int i) { return i; });

   // Act
   auto future = task.Get_Future();

   // Assert
   EXPECT_TRUE(future.Valid()) << "Future does not have a valid state.";
}

TEST(PackagedTaskTest, GetFuture_ThrowsIfTaskIsInvalid)
{
   // Arrange
   Packaged_Task<int(int)> task;

   // Act/Assert
   EXPECT_THROW(task.Get_Future(), future_error);
}

TEST(PackagedTaskTest, GetFuture_ThrowsIfCalledMoreThanOnce)
{
   // Arrange
   Packaged_Task<int(int)> task([](int i) { return i; });
   task.Get_Future();

   // Act/Assert
   EXPECT_THROW(task.Get_Future(), future_error);
}

TEST(PackagedTaskTest, Execute_FullfillsTaskPromise)
{
   // Arrange
   int expected = 3;
   Packaged_Task<int(int)> task([](int i) { return i; });
   auto future = task.Get_Future();

   // Act
   task(expected);

   // Assert
   EXPECT_EQ(expected, future.Get());
}

TEST(PackagedTaskTest, Execute_ForwardsTaskExceptions)
{
   // Arrange
   Packaged_Task<int(int)> task([](int i) -> int { throw PackagedTaskTestException(); });
   auto future = task.Get_Future();

   // Act
   task(3);

   // Assert
   EXPECT_THROW(future.Get(), PackagedTaskTestException);
}

TEST(PackagedTaskTest, Execute_ForwardsTaskExceptions_FutureError)
{
   // Arrange
   Packaged_Task<int(int)> task([](int i) -> int { throw future_error(std::make_error_code(future_errc::no_state)); });
   auto future = task.Get_Future();

   // Act
   task(0);

   // Assert
   EXPECT_THROW(future.Get(), future_error);
}

TEST(PackagedTaskTest, Execute_ThrowsIfTaskIsInvalid)
{
   // Arrange
   Packaged_Task<int(int)> task;

   // Act/Assert
   EXPECT_THROW(task(0), future_error);
}

TEST(PackagedTaskTest, Execute_ThrowsIfTaskPromiseSatisfied)
{
   // Arrange
   Packaged_Task<int(int)> task([](int i) { return i; });
   task(0);

   // Act/Assert
   EXPECT_THROW(task(0), future_error);
}

TEST(PackagedTaskTest, MakeReadyAtThreadExit_FullfillsTaskPromise)
{
   // Arrange
   int expected = 3;
   Packaged_Task<int(int)> task([](int i) { return i; });
   auto future = task.Get_Future();

   std::mutex m;
   std::condition_variable cv;
   bool ready = false;

   // Act
   std::thread worker([&]()
   {
      task.Make_Ready_At_Thread_Exit(expected);

      std::unique_lock<std::mutex> l(m);
      cv.wait(l, [&]() { return ready; });
   });

   // Assert
   EXPECT_FALSE(future.Is_Ready()) << "The value changed signal was set prematurely.";
   {
      std::unique_lock<std::mutex> l(m);
      ready = true;
   }
   cv.notify_all();
   worker.join();
   EXPECT_EQ(expected, future.Get());
}

TEST(PackagedTaskTest, MakeReadyAtThreadExitVoid_FullfillsTaskPromise)
{
   // Arrange
   int expected = 5;
   Packaged_Task<void(int)> task([](int i) { });
   auto future = task.Get_Future();

   std::mutex m;
   std::condition_variable cv;
   bool ready = false;

   // Act
   std::thread worker([&]()
   {
      task.Make_Ready_At_Thread_Exit(expected);

      std::unique_lock<std::mutex> l(m);
      cv.wait(l, [&]() { return ready; });
   });

   // Assert
   EXPECT_FALSE(future.Is_Ready()) << "The value changed signal was set prematurely.";
   {
      std::unique_lock<std::mutex> l(m);
      ready = true;
   }
   cv.notify_all();
   worker.join();
   EXPECT_TRUE(future.Is_Ready());
}

TEST(PackagedTaskTest, MakeReadyAtThreadExit_ForwardsTaskExceptions)
{
   // Arrange
   Packaged_Task<int(int)> task([](int i) -> int { throw PackagedTaskTestException(); });
   auto future = task.Get_Future();

   std::mutex m;
   std::condition_variable cv;
   bool ready = false;

   // Act
   std::thread worker([&]()
   {
      task.Make_Ready_At_Thread_Exit(3);

      std::unique_lock<std::mutex> l(m);
      cv.wait(l, [&]() { return ready; });
   });

   // Assert
   EXPECT_FALSE(future.Is_Ready()) << "The value changed signal was set prematurely.";
   {
      std::unique_lock<std::mutex> l(m);
      ready = true;
   }
   cv.notify_all();
   worker.join();
   EXPECT_THROW(future.Get(), PackagedTaskTestException);
}

TEST(PackagedTaskTest, MakeReadyAtThreadExit_ForwardsTaskExceptions_FutureError)
{
   // Arrange
   Packaged_Task<int(int)> task([](int i) -> int { throw future_error(std::make_error_code(future_errc::no_state)); });
   auto future = task.Get_Future();

   std::mutex m;
   std::condition_variable cv;
   bool ready = false;

   // Act
   std::thread worker([&]()
   {
      task.Make_Ready_At_Thread_Exit(3);

      std::unique_lock<std::mutex> l(m);
      cv.wait(l, [&]() { return ready; });
   });

   // Assert
   EXPECT_FALSE(future.Is_Ready()) << "The value changed signal was set prematurely.";
   {
      std::unique_lock<std::mutex> l(m);
      ready = true;
   }
   cv.notify_all();
   worker.join();
   EXPECT_THROW(future.Get(), future_error);
}

TEST(PackagedTaskTest, MakeReadyAtThreadExit_ThrowsIfTaskIsInvalid)
{
   // Arrange
   Packaged_Task<int(int)> task;

   // Act/Assert
   EXPECT_THROW(task.Make_Ready_At_Thread_Exit(0), future_error);
}

TEST(PackagedTaskTest, MakeReadyAtThreadExit_ThrowsIfTaskPromiseSatisfied)
{
   // Arrange
   Packaged_Task<int(int)> task([](int i) { return i; });
   task.Make_Ready_At_Thread_Exit(0);

   // Act/Assert
   EXPECT_THROW(task.Make_Ready_At_Thread_Exit(0), future_error);
}

TEST(PackagedTaskTest, Reset_ResetsPromiseState)
{
   // Arrange
   Packaged_Task<int(int)> task([](int i) { return i; });
   task(0); // fullfilled

   // Act
   task.Reset();

   // Assert
   auto future = task.Get_Future();
   EXPECT_FALSE(future.Is_Ready());
}

TEST(PackagedTaskTest, Reset_ThrowsIfTaskIsInvalid)
{
   // Arrange
   Packaged_Task<int(int)> task;
   
   // Act/Assert
   EXPECT_THROW(task.Reset(), future_error);
}
