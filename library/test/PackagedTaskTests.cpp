#include "stdafx.h"
#include <thread>
#include <cstdint>
#include <eventual/eventual.h>
#include "BasicAllocator.h"

using namespace eventual;

namespace
{
   class PackagedTaskTestException { };
}

TEST(PackagedTaskTest, DefaultConstructorCreatesInvalidTask)
{
   // Arrange
   packaged_task<int(int)> task;

   // Act/Assert
   EXPECT_FALSE(task.valid());
}

TEST(PackagedTaskTest, ConstructorWithFunctorCreatesValidTask)
{
   // Arrange
   packaged_task<int(int)> task([](int i) { return i; });

   // Act/Assert
   EXPECT_TRUE(task.valid());
}

namespace
{
    typedef int(*dummy_t)(int object);
}

TEST(PackagedTaskTest, ConstructorWithAllocatorCreatesValidTask)
{
    // Arrange
    auto alloc = BasicAllocator<int>();

    // create a stupid-large type to force std::function to use the allocator
    struct large
    {
        int_least64_t data[12];
    };

    large capture;
    capture.data[0] = 5;

    packaged_task<int(int)> task(
        std::allocator_arg_t(), 
        alloc, 
        [capture](int i) { return static_cast<int>(i + capture.data[0]); });

    // Act/Assert
    EXPECT_TRUE(task.valid());
    EXPECT_GT(alloc.GetCount(), 0);
}

TEST(PackagedTaskTest, IsMoveConstructable)
{
   // Arrange
   packaged_task<int(int)> task([](int i) { return i; });

   // Act
   packaged_task<int(int)> moved(std::move(task));

   // Assert
   EXPECT_TRUE(moved.valid());
   EXPECT_FALSE(task.valid());
}

TEST(PackagedTaskTest, IsNotCopyConstructable)
{
   // Assert
   EXPECT_FALSE(std::is_copy_constructible<packaged_task<int(int)>>::value);
}

TEST(PackagedTaskTest, DestructorReleasesState)
{
   // Arrange
   auto alloc = BasicAllocator<int>();
   {
      packaged_task<int(int)> task(std::allocator_arg_t(), alloc, [](int i) { return i; });
      task(3);
   } // Act

     // Assert
   EXPECT_EQ(0, alloc.GetCount());
}

TEST(PackagedTaskTest, DestructorSignalsBrokenPromiseIfNotComplete)
{
   // Arrange
   future<int> future;
   {
      packaged_task<int(int)> task([](int i) { return i; });
      future = task.get_future();
   } // Act

     // Assert
   EXPECT_THROW(future.get(), std::future_error);
}

TEST(PackagedTaskTest, IsMoveAssignable)
{
   // Arrange
   packaged_task<int(int)> task([](int i) { return i; });

   // Act
   packaged_task<int(int)> moved = std::move(task);

   // Assert
   EXPECT_TRUE(moved.valid());
   EXPECT_FALSE(task.valid());
}

TEST(PackagedTaskTest, IsNotCopyAssignable)
{
   // Assert
   EXPECT_FALSE(std::is_copy_assignable<packaged_task<int(int)>>::value);
}

TEST(PackagedTaskTest, Valid_ReturnsTrueIfTaskHasState)
{
   //Arrange
   packaged_task<int(int)> task([](int i) { return i; });

   // Act/Assert
   EXPECT_TRUE(task.valid());
}

TEST(PackagedTaskTest, Valid_ReturnsFalseIfTaskHasNoState)
{
   //Arrange
   packaged_task<int(int)> task;

   // Act/Assert
   EXPECT_FALSE(task.valid());
}

TEST(PackagedTaskTest, IsSwapable)
{
   // Arrange
   packaged_task<int(int)> task([](int i) { return i; });
   packaged_task<int(int)> moved;

   // Act
   task.swap(moved);

   // Assert
   EXPECT_TRUE(moved.valid());
   EXPECT_FALSE(task.valid());
}

TEST(PackagedTaskTest, IsSwapableWithVoid)
{
    // Arrange
    packaged_task<void(int)> task([](int) { });
    packaged_task<void(int)> moved;

    // Act
    task.swap(moved);

    // Assert
    EXPECT_TRUE(moved.valid());
    EXPECT_FALSE(task.valid());
}

TEST(PackagedTaskTest, STD_IsSwapable)
{
   // Arrange
   packaged_task<int(int)> task([](int i) { return i; });
   packaged_task<int(int)> moved;

   // Act
   std::swap(task, moved);

   // Assert
   EXPECT_TRUE(moved.valid());
   EXPECT_FALSE(task.valid());
}

TEST(PackagedTaskTest, GetFuture_ReturnsValidFuture)
{
   // Arrange
   packaged_task<int(int)> task([](int i) { return i; });

   // Act
   auto future = task.get_future();

   // Assert
   EXPECT_TRUE(future.valid()) << "Future does not have a valid state.";
}

TEST(PackagedTaskTest, GetFuture_ThrowsIfTaskIsInvalid)
{
   // Arrange
   packaged_task<int(int)> task;

   // Act/Assert
   EXPECT_THROW(task.get_future(), std::future_error);
}

TEST(PackagedTaskTest, GetFuture_ThrowsIfCalledMoreThanOnce)
{
   // Arrange
   packaged_task<int(int)> task([](int i) { return i; });
   task.get_future();

   // Act/Assert
   EXPECT_THROW(task.get_future(), std::future_error);
}

TEST(PackagedTaskTest, Execute_FullfillsTaskPromise)
{
   // Arrange
   int expected = 3;
   packaged_task<int(int)> task([](int i) { return i; });
   auto future = task.get_future();

   // Act
   task(expected);

   // Assert
   EXPECT_EQ(expected, future.get());
}

TEST(PackagedTaskTest, ExecuteWithVoidResult_FullfillsTaskPromise)
{
    // Arrange
    int expected = 3;
    auto actualPtr = std::make_unique<int>(0);
    packaged_task<void(int)> task([ptr = actualPtr.get()](int i) { *ptr = i; });
    auto future = task.get_future();

    // Act
    task(expected);
    future.get();

    // Assert
    EXPECT_EQ(expected, *actualPtr);
}

TEST(PackagedTaskTest, Execute_ForwardsTaskExceptions)
{
   // Arrange
   packaged_task<int(int)> task([](int) -> int { throw PackagedTaskTestException(); });
   auto future = task.get_future();

   // Act
   task(3);

   // Assert
   EXPECT_THROW(future.get(), PackagedTaskTestException);
}

TEST(PackagedTaskTest, Execute_ForwardsTaskExceptions_FutureError)
{
   // Arrange
   packaged_task<int(int)> task([](int) -> int { throw std::future_error(std::future_errc::no_state); });
   auto future = task.get_future();

   // Act
   task(0);

   // Assert
   EXPECT_THROW(future.get(), std::future_error);
}

TEST(PackagedTaskTest, ExecuteWithVoid_ForwardsTaskExceptions_FutureError)
{
    // Arrange
    packaged_task<void(int)> task([](int) { throw std::future_error(std::future_errc::no_state); });
    auto future = task.get_future();

    // Act
    task(0);

    // Assert
    EXPECT_THROW(future.get(), std::future_error);
}

TEST(PackagedTaskTest, Execute_ThrowsIfTaskIsInvalid)
{
   // Arrange
   packaged_task<int(int)> task;

   // Act/Assert
   EXPECT_THROW(task(0), std::future_error);
}

TEST(PackagedTaskTest, Execute_ThrowsIfTaskPromiseSatisfied)
{
   // Arrange
   packaged_task<int(int)> task([](int i) { return i; });
   task(0);

   // Act/Assert
   EXPECT_THROW(task(0), std::future_error);
}

TEST(PackagedTaskTest, ExecuteWithVoid_ThrowsIfTaskPromiseSatisfied)
{
    // Arrange
    packaged_task<void(int)> task([](int) { });
    task(0);

    // Act/Assert
    EXPECT_THROW(task(0), std::future_error);
}

TEST(PackagedTaskTest, MakeReadyAtThreadExit_FullfillsTaskPromise)
{
   // Arrange
   int expected = 3;
   packaged_task<int(int)> task([](int i) { return i; });
   auto future = task.get_future();

   std::mutex m;
   std::condition_variable cv;
   bool ready = false;

   // Act
   std::thread worker([&]()
   {
      task.make_ready_at_thread_exit(expected);

      std::unique_lock<std::mutex> l(m);
      cv.wait(l, [&]() { return ready; });
   });

   // Assert
   EXPECT_FALSE(future.is_ready()) << "The value changed signal was set prematurely.";
   {
      std::unique_lock<std::mutex> l(m);
      ready = true;
   }
   cv.notify_all();
   worker.join();
   EXPECT_EQ(expected, future.get());
}

TEST(PackagedTaskTest, MakeReadyAtThreadExitVoid_FullfillsTaskPromise)
{
   // Arrange
   int expected = 5;
   packaged_task<void(int)> task([](int) { });
   auto future = task.get_future();

   std::mutex m;
   std::condition_variable cv;
   bool ready = false;

   // Act
   std::thread worker([&]()
   {
      task.make_ready_at_thread_exit(expected);

      std::unique_lock<std::mutex> l(m);
      cv.wait(l, [&]() { return ready; });
   });

   // Assert
   EXPECT_FALSE(future.is_ready()) << "The value changed signal was set prematurely.";
   {
      std::unique_lock<std::mutex> l(m);
      ready = true;
   }
   cv.notify_all();
   worker.join();
   EXPECT_TRUE(future.is_ready());
}

TEST(PackagedTaskTest, MakeReadyAtThreadExit_ForwardsTaskExceptions)
{
   // Arrange
   packaged_task<int(int)> task([](int) -> int { throw PackagedTaskTestException(); });
   auto future = task.get_future();

   std::mutex m;
   std::condition_variable cv;
   bool ready = false;

   // Act
   std::thread worker([&]()
   {
      task.make_ready_at_thread_exit(3);

      std::unique_lock<std::mutex> l(m);
      cv.wait(l, [&]() { return ready; });
   });

   // Assert
   EXPECT_FALSE(future.is_ready()) << "The value changed signal was set prematurely.";
   {
      std::unique_lock<std::mutex> l(m);
      ready = true;
   }
   cv.notify_all();
   worker.join();
   EXPECT_THROW(future.get(), PackagedTaskTestException);
}

TEST(PackagedTaskTest, MakeReadyAtThreadExitVoid_ForwardsTaskExceptions)
{
    // Arrange
    packaged_task<void(int)> task([](int) { throw PackagedTaskTestException(); });
    auto future = task.get_future();

    std::mutex m;
    std::condition_variable cv;
    bool ready = false;

    // Act
    std::thread worker([&]()
    {
        task.make_ready_at_thread_exit(3);

        std::unique_lock<std::mutex> l(m);
        cv.wait(l, [&]() { return ready; });
    });

    // Assert
    EXPECT_FALSE(future.is_ready()) << "The value changed signal was set prematurely.";
    {
        std::unique_lock<std::mutex> l(m);
        ready = true;
    }
    cv.notify_all();
    worker.join();
    EXPECT_THROW(future.get(), PackagedTaskTestException);
}

TEST(PackagedTaskTest, MakeReadyAtThreadExit_ForwardsTaskExceptions_FutureError)
{
   // Arrange
   packaged_task<int(int)> task([](int) -> int { throw std::future_error(std::future_errc::no_state); });
   auto future = task.get_future();

   std::mutex m;
   std::condition_variable cv;
   bool ready = false;

   // Act
   std::thread worker([&]()
   {
      task.make_ready_at_thread_exit(3);

      std::unique_lock<std::mutex> l(m);
      cv.wait(l, [&]() { return ready; });
   });

   // Assert
   EXPECT_FALSE(future.is_ready()) << "The value changed signal was set prematurely.";
   {
      std::unique_lock<std::mutex> l(m);
      ready = true;
   }
   cv.notify_all();
   worker.join();
   EXPECT_THROW(future.get(), std::future_error);
}

TEST(PackagedTaskTest, MakeReadyAtThreadExitVoid_ForwardsTaskExceptions_FutureError)
{
    // Arrange
    packaged_task<void(int)> task([](int) { throw std::future_error(std::future_errc::no_state); });
    auto future = task.get_future();

    std::mutex m;
    std::condition_variable cv;
    bool ready = false;

    // Act
    std::thread worker([&]()
    {
        task.make_ready_at_thread_exit(3);

        std::unique_lock<std::mutex> l(m);
        cv.wait(l, [&]() { return ready; });
    });

    // Assert
    EXPECT_FALSE(future.is_ready()) << "The value changed signal was set prematurely.";
    {
        std::unique_lock<std::mutex> l(m);
        ready = true;
    }
    cv.notify_all();
    worker.join();
    EXPECT_THROW(future.get(), std::future_error);
}

TEST(PackagedTaskTest, MakeReadyAtThreadExit_ThrowsIfTaskIsInvalid)
{
   // Arrange
   packaged_task<int(int)> task;

   // Act/Assert
   EXPECT_THROW(task.make_ready_at_thread_exit(0), std::future_error);
}

TEST(PackagedTaskTest, MakeReadyAtThreadExit_ThrowsIfTaskPromiseSatisfied)
{
   // Arrange
   packaged_task<int(int)> task([](int i) { return i; });
   task.make_ready_at_thread_exit(0);

   // Act/Assert
   EXPECT_THROW(task.make_ready_at_thread_exit(0), std::future_error);
}

TEST(PackagedTaskTest, MakeReadyAtThreadExitVoid_ThrowsIfTaskPromiseSatisfied)
{
    // Arrange
    packaged_task<void(int)> task([](int) { });
    task.make_ready_at_thread_exit(0);

    // Act/Assert
    EXPECT_THROW(task.make_ready_at_thread_exit(0), std::future_error);
}

TEST(PackagedTaskTest, Reset_ResetsPromiseState)
{
   // Arrange
   packaged_task<int(int)> task([](int i) { return i; });
   task(0); // fullfilled

   // Act
   task.reset();

   // Assert
   auto future = task.get_future();
   EXPECT_FALSE(future.is_ready());
}

TEST(PackagedTaskTest, Reset_ThrowsIfTaskIsInvalid)
{
   // Arrange
   packaged_task<int(int)> task;
   
   // Act/Assert
   EXPECT_THROW(task.reset(), std::future_error);
}
