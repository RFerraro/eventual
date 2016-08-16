#include "stdafx.h"
#include <cstdint>
#include <cstddef>
#include <memory>
#include <eventual/eventual.h>

#include "BasicAllocator.h"

namespace
{
    constexpr uintptr_t max_align_v = alignof(std::max_align_t);
}

using namespace eventual::detail;

TEST(ResourceAdapterTest, CreateShared_ShouldReturn_AnAdapterOfAllocator)
{
    // Arrange/Act
    auto adapter = resource_adapter<BasicAllocator<int>>::create_shared(BasicAllocator<int>{});

    // Act/Assert
    EXPECT_TRUE(adapter.get() != nullptr);
}

TEST(ResourceAdapterTest, ResourceAdapter_Should_BeDefaultConstructable)
{
    // Arrange/Act/Assert
    EXPECT_NO_THROW(resource_adapter<BasicAllocator<int>> adapter { });
}

TEST(ResourceAdapterTest, ResourceAdapter_Should_BeCopyConstructable)
{
    // Arrange
    resource_adapter<BasicAllocator<int>> adapter { };

    // Act/Assert
    EXPECT_NO_THROW(auto copy = adapter);
}

TEST(ResourceAdapterTest, ResourceAdapter_Should_BeConstructableWithAnAllocator)
{
    // Arrange
    BasicAllocator<int> allocator { };

    // Act/Assert
    EXPECT_NO_THROW(resource_adapter<BasicAllocator<int>> adapter(allocator));
}

TEST(ResourceAdapterTest, Allocate_Should_AllocateRequestedAmountOfMemoryWithMaxAlignment)
{
    // Arrange
    resource_adapter<BasicAllocator<int>> adapter { };

    // Act
    auto memory = adapter.allocate(sizeof(int));

    // Assert
    EXPECT_TRUE(memory != nullptr);
    auto address = reinterpret_cast<uintptr_t>(memory);

    EXPECT_EQ(0, address % max_align_v) << "memory is not aligned.";

    adapter.deallocate(memory, sizeof(int));
}

namespace
{
    void TestMemoryAlignment(std::size_t size, uintptr_t alignment)
    {
        // Arrange
        resource_adapter<BasicAllocator<int>> adapter { };

        // Act
        auto memory = adapter.allocate(size, alignment);

        // Assert
        EXPECT_TRUE(memory != nullptr);
        
        //todo: not aligning reliably under VS2015?
        auto address = reinterpret_cast<uintptr_t>(memory);
        EXPECT_EQ(0, address % alignment) << "memory is not aligned.";

        adapter.deallocate(memory, size, alignment);
    }
}

TEST(ResourceAdapterTest, Allocate_Should_AllocateRequestedAmountOfMemoryWithRequestedAlignment_1)
{
    TestMemoryAlignment(4, 1);
}

TEST(ResourceAdapterTest, Allocate_Should_AllocateRequestedAmountOfMemoryWithRequestedAlignment_2)
{
    TestMemoryAlignment(4, 2);
}

TEST(ResourceAdapterTest, Allocate_Should_AllocateRequestedAmountOfMemoryWithRequestedAlignment_4)
{
    TestMemoryAlignment(4, 4);
}

TEST(ResourceAdapterTest, Allocate_Should_AllocateRequestedAmountOfMemoryWithRequestedAlignment_8)
{
    TestMemoryAlignment(4, 8);
}

TEST(ResourceAdapterTest, Allocate_Should_AllocateRequestedAmountOfMemoryWithRequestedAlignment_16)
{
    TestMemoryAlignment(4, 16);
}

TEST(ResourceAdapterTest, Allocate_Should_AllocateRequestedAmountOfMemoryWithRequestedAlignment_32)
{
    TestMemoryAlignment(4, 32);
}

TEST(ResourceAdapterTest, Allocate_Should_AllocateRequestedAmountOfMemoryWithRequestedAlignment_64)
{
    TestMemoryAlignment(4, 64);
}

TEST(ResourceAdapterTest, Allocate_Should_AllocateRequestedAmountOfMemoryWithRequestedAlignment_128)
{
    TestMemoryAlignment(4, 128);
}

TEST(ResourceAdapterTest, Deallocate_Should_ReleaseRequestedAmountOfMemoryWithMaxAlignment)
{
    // Arrange
    resource_adapter<BasicAllocator<int>> adapter { };
    auto memory = adapter.allocate(sizeof(int));

    // Act/Assert
    EXPECT_NO_THROW(adapter.deallocate(memory, sizeof(int)));
}

TEST(ResourceAdapterTest, Deallocate_Should_ReleaseRequestedAmountOfMemoryWithRequestedAlignment)
{
    // Arrange
    resource_adapter<BasicAllocator<int>> adapter { };
    size_t alignment(4u);
    auto memory = adapter.allocate(sizeof(int), alignment);

    // Assert
    EXPECT_NO_THROW(adapter.deallocate(memory, sizeof(int), alignment));
}

TEST(ResourceAdapterTest, IsEqual_Should_ReturnFalse_IfImplementationsAreNotEqual)
{
    // Arrange
    resource_adapter<BasicAllocator<int>> first { };
    resource_adapter<std::allocator<int>> second { };

    // Act/Assert
    EXPECT_FALSE(first.is_equal(second));
}

TEST(ResourceAdapterTest, IsEqual_Should_ReturnTrue_IfImplementationsAreEqual)
{
    // Arrange
    resource_adapter<BasicAllocator<int>> first { };
    resource_adapter<BasicAllocator<int>> second { };

    // Act/Assert
    EXPECT_TRUE(first.is_equal(second));
}

TEST(ResourceAdapterTest, OperatorEqual_Should_ReturnFalse_IfImplementationsAreNotEqual)
{
    // Arrange
    resource_adapter<BasicAllocator<int>> first { };
    resource_adapter<std::allocator<int>> second { };

    // Act/Assert
    EXPECT_FALSE(first == second);
}

TEST(ResourceAdapterTest, OperatorEqual_Should_ReturnTrue_IfImplementationsAreEqual)
{
    // Arrange
    resource_adapter<BasicAllocator<int>> first { };
    resource_adapter<BasicAllocator<int>> second { };

    // Act/Assert
    EXPECT_TRUE(first == second);
}

TEST(ResourceAdapterTest, OperatorNotEqual_Should_ReturnTrue_IfImplementationsAreNotEqual)
{
    // Arrange
    resource_adapter<BasicAllocator<int>> first { };
    resource_adapter<std::allocator<int>> second { };

    // Act/Assert
    EXPECT_TRUE(first != second);
}

TEST(ResourceAdapterTest, OperatorNotEqual_Should_ReturnFalse_IfImplementationsAreEqual)
{
    // Arrange
    resource_adapter<BasicAllocator<int>> first { };
    resource_adapter<BasicAllocator<int>> second { };

    // Act/Assert
    EXPECT_FALSE(first != second);
}

