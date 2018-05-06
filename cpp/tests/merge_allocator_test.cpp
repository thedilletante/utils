//
// Created by dilletante on 06.05.18.
//

#include <gtest/gtest.h>
#include "merge_allocator.h"

template <size_t N>
struct StaticArrayArena
    : public ArenaHolder {

    char *begin() noexcept override {
        return block;
    }

    size_t size() const noexcept override {
        return N;
    }

    char block[N];
};

TEST(alloc, test) {
    StaticArrayArena<16> arena;
    MergeAllocator allocator(arena);

    auto ptr = allocator.allocate(1);
    ASSERT_EQ(ptr, arena.begin());

    allocator.deallocate(ptr, 1);
    auto ptr1 = allocator.allocate(10);
    ASSERT_EQ(ptr1, arena.begin());

    ASSERT_EQ(nullptr, allocator.allocate(1));
}