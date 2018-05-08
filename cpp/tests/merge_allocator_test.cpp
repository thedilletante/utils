//
// Created by dilletante on 06.05.18.
//

#include <gtest/gtest.h>
#include "merge_allocator.h"

template <size_t N>
struct StaticArrayArena
    : public ArenaHolder {
    char *begin() noexcept override { return block; }
    size_t size() const noexcept override { return N; }
    char block[N];
};

TEST(merge_allocator, firstAllocateShouldReturnTheBeggining) {
    StaticArrayArena<16> arena;
    MergeAllocator allocator(arena);

    ASSERT_EQ(allocator.allocate(1), arena.begin());
}

TEST(merge_allocator, allocateAfterDeallocateAfterAllocateShouldReturnTheSameMemory) {
    StaticArrayArena<16> arena;
    MergeAllocator allocator(arena);

    auto theFirstAllocation = allocator.allocate(1);
    allocator.deallocate(theFirstAllocation, 1);

    ASSERT_EQ(allocator.allocate(1), theFirstAllocation);
}