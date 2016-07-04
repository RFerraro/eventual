#include "stdafx.h"
#include <memory>
#include <type_traits>
#include <eventual/eventual.h>

#include "NullResource.h"

using namespace eventual::detail;

namespace local
{
    template<std::size_t Len, class... Type>
    using aligned_union_t = typename std::aligned_union<Len, Type...>::type;
}

TEST(PolymorphicAllocatorTest, PolymorphicAllocator_ShouldBe_DefaultConstructable)
{
    // Arrange/Act
    polymorphic_allocator<int> alloc { };

    // Assert
    EXPECT_TRUE(alloc.resource() != nullptr);
}

TEST(PolymorphicAllocatorTest, PolymorphicAllocator_ShouldBe_ConstructableWithMemoryResource)
{
    // Arrange
    NullResource resource { };

    // Act
    polymorphic_allocator<int> alloc(&resource);

    // Assert
    EXPECT_TRUE(resource.is_equal(*alloc.resource()));
}

TEST(PolymorphicAllocatorTest, PolymorphicAllocator_ShouldBe_ConstructableWithAnotherPolyAllocatorType)
{
    // Arrange
    polymorphic_allocator<char> other { };

    // Act
    polymorphic_allocator<int> alloc(other);

    // Assert
    EXPECT_EQ(other.resource(), alloc.resource());
}

TEST(PolymorphicAllocatorTest, PolymorphicAllocator_ShouldBe_CopyConstructable)
{
    // Arrange
    const polymorphic_allocator<int> other { };

    // Act
    polymorphic_allocator<int> alloc(other);

    // Assert
    EXPECT_EQ(other, alloc);
}

TEST(PolymorphicAllocatorTest, PolymorphicAllocator_ShouldBe_CopyAssignable)
{
    // Arrange
    const polymorphic_allocator<int> other { };

    // Act
    auto alloc = other;

    // Assert
    EXPECT_EQ(other, alloc);
}

TEST(PolymorphicAllocatorTest, Resource_Should_ReturnMemoryResource)
{
    // Arrange
    polymorphic_allocator<int> alloc { };

    // Act
    auto resource = alloc.resource();

    // Assert
    EXPECT_TRUE(resource != nullptr);
}

TEST(PolymorphicAllocatorTest, Allocate_Should_ReturnMemoryWithANumberOfItems)
{
    // Arrange
    std::size_t count(10);
    polymorphic_allocator<int> alloc { };

    // Act
    auto memory = alloc.allocate(count);

    // Assert
    EXPECT_TRUE(memory != nullptr);

    // try to write to the memory...
    for (std::size_t i = 0; i < count; i++)
        memory[i] = static_cast<int>(i);

    alloc.deallocate(memory, count);
}

TEST(PolymorphicAllocatorTest, DeAllocate_Should_ReleaseMemoryWithANumberOfItems)
{
    // Arrange
    std::size_t count(10);
    polymorphic_allocator<int> alloc { };
    auto memory = alloc.allocate(count);

    // Act/Assert
    EXPECT_NO_THROW(alloc.deallocate(memory, count));
}

TEST(PolymorphicAllocatorTest, Construct_Should_PlacementConstruct)
{
    // Arrange
    struct tempObj { };
    using storage_t = local::aligned_union_t<1, tempObj>;
    storage_t memory { };

    polymorphic_allocator<tempObj> alloc { };
 
    // Act/Assert
    EXPECT_NO_THROW(alloc.construct(&memory));
}

TEST(PolymorphicAllocatorTest, Destroy_Should_PlacementDestruct)
{
    // Arrange
    struct tempObj { };
    using storage_t = local::aligned_union_t<1, tempObj>;
    storage_t memory { };

    polymorphic_allocator<tempObj> alloc { };
    alloc.construct(&memory);
    auto object = reinterpret_cast<tempObj*>(&memory);

    // Act/Assert
    EXPECT_NO_THROW(alloc.destroy(object));
}

TEST(PolymorphicAllocatorTest, SelectOnContainerCopyConstruction_Should_ReturnDefaultPolymorphicAllocator)
{
    // Arrange
    NullResource resource { };
    polymorphic_allocator<int> alloc { &resource };

    // Act
    auto other = alloc.select_on_container_copy_construction();

    // Assert
    EXPECT_FALSE(&alloc == &other);
    EXPECT_FALSE(alloc.resource() == other.resource());
}