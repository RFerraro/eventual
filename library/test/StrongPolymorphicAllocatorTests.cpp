#include "stdafx.h"
#include <memory>
#include <eventual/eventual.h>

#include "NullResource.h"
#include "BasicAllocator.h"

using namespace eventual::detail;

TEST(StrongPolymorphicAllocatorTest, StrongPolymorphicAllocator_ShouldBe_ConstructableWithAllocator)
{
    // Arrange
    BasicAllocator<int> basicAlloc { };

    // Act
    strong_polymorphic_allocator<int> alloc(basicAlloc);

    // Assert
    EXPECT_TRUE(alloc.resource() != nullptr);
}

TEST(StrongPolymorphicAllocatorTest, StrongPolymorphicAllocator_ShouldBe_ConstructableWithAnotherStrongPolyAllocator)
{
    // Arrange
    BasicAllocator<char> basicAlloc { };
    strong_polymorphic_allocator<char> other(basicAlloc);

    // Act
    strong_polymorphic_allocator<int> alloc(other);

    // Assert
    EXPECT_TRUE(alloc.resource() == other.resource());
}

TEST(StrongPolymorphicAllocatorTest, StrongPolymorphicAllocator_ShouldBe_ConstructableWithSharedMemoryResources)
{
    // Arrange
    using shared_resource_t = std::shared_ptr<memory_resource>;
    shared_resource_t resource(new NullResource());

    // Act
    strong_polymorphic_allocator<int> alloc(std::move(resource));

    // Assert
    EXPECT_TRUE(alloc.resource() != nullptr);
}

TEST(StrongPolymorphicAllocatorTest, StrongPolymorphicAllocator_ShouldBe_CopyConstructable)
{
    // Arrange
    BasicAllocator<int> basicAlloc { };
    strong_polymorphic_allocator<int> other(basicAlloc);

    // Act
    strong_polymorphic_allocator<int> alloc(other);

    // Assert
    EXPECT_TRUE(alloc.resource() == other.resource());
}

TEST(StrongPolymorphicAllocatorTest, StrongPolymorphicAllocator_ShouldBe_CopyAssignable)
{
    // Arrange
    BasicAllocator<int> basicAlloc { };
    strong_polymorphic_allocator<int> other(basicAlloc);

    // Act
    strong_polymorphic_allocator<int> alloc = other;

    // Assert
    EXPECT_TRUE(alloc.resource() == other.resource());
}

TEST(StrongPolymorphicAllocatorTest, Share_Should_ReturnSharedMemoryResource)
{
    // Arrange
    using shared_resource_t = std::shared_ptr<memory_resource>;
    shared_resource_t expected(new NullResource());
    auto copy = expected;
    strong_polymorphic_allocator<int> alloc(std::move(copy));

    // Act
    auto resource = alloc.share();

    // Assert
    EXPECT_TRUE(resource.get() == expected.get());
}

TEST(StrongPolymorphicAllocatorTest, OperatorEqual_Should_ReturnFalse_IfImplementationsAreNotEqual)
{
    // Arrange
    using shared_resource_t = std::shared_ptr<memory_resource>;
    shared_resource_t firstResource(new NullResource());
    shared_resource_t secondResource(new NullResource());

    strong_polymorphic_allocator<int> firstAlloc(std::move(firstResource));
    strong_polymorphic_allocator<int> secondAlloc(std::move(secondResource));

    // Act/Assert
    EXPECT_FALSE(firstAlloc == secondAlloc);
}

TEST(StrongPolymorphicAllocatorTest, OperatorEqual_Should_ReturnTrue_IfImplementationsAreEqual)
{
    // Arrange
    using shared_resource_t = std::shared_ptr<memory_resource>;
    shared_resource_t resource(new NullResource());
    auto copy = resource;

    strong_polymorphic_allocator<int> firstAlloc(std::move(resource));
    strong_polymorphic_allocator<int> secondAlloc(std::move(copy));

    // Act/Assert
    EXPECT_TRUE(firstAlloc == secondAlloc);
}

TEST(StrongPolymorphicAllocatorTest, OperatorNotEqual_Should_ReturnTrue_IfImplementationsAreNotEqual)
{
    // Arrange
    using shared_resource_t = std::shared_ptr<memory_resource>;
    shared_resource_t firstResource(new NullResource());
    shared_resource_t secondResource(new NullResource());

    strong_polymorphic_allocator<int> firstAlloc(std::move(firstResource));
    strong_polymorphic_allocator<int> secondAlloc(std::move(secondResource));

    // Act/Assert
    EXPECT_TRUE(firstAlloc != secondAlloc);
}

TEST(StrongPolymorphicAllocatorTest, OperatorNotEqual_Should_ReturnFalse_IfImplementationsAreEqual)
{
    // Arrange
    using shared_resource_t = std::shared_ptr<memory_resource>;
    shared_resource_t resource(new NullResource());
    auto copy = resource;

    strong_polymorphic_allocator<int> firstAlloc(std::move(resource));
    strong_polymorphic_allocator<int> secondAlloc(std::move(copy));

    // Act/Assert
    EXPECT_FALSE(firstAlloc != secondAlloc);
}