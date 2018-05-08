//
// Created by dilletante on 06.05.18.
//
#include <algorithm>
#include <tuple>
#include <cassert>
#include "merge_allocator.h"


struct Node {
    size_t size;
    Node* next;
};

namespace {

Node *createNodeIn(char *place, size_t size, Node *next = nullptr) noexcept {
    assert(place != nullptr);
    const auto node = reinterpret_cast<Node*>(place);
    node->size = size;
    node->next = next;
    return node;
}

Node* mergeLeftAndReturnPrev(Node* right, char* block, size_t size) noexcept {
    const auto right_block = reinterpret_cast<char*>(right);
    const auto end = block + size;
    assert(end <= right_block);

    const auto left = right_block - end;
    if (left < sizeof(Node)) {
        return createNodeIn(block, right->size + size + left, right->next);
    } else {
        return createNodeIn(block, size, right);
    }
};

void mergeRight(Node* left, char* block, size_t size) noexcept {
    const auto end = reinterpret_cast<char*>(left) + left->size;
    assert(end <= block);

    const auto left_diff = block - end;
    if (left_diff < sizeof(Node)) {
        left->size += size + left_diff;
        left->next = left->next ? left->next->next : nullptr;
    }
}

size_t padding(size_t size, size_t padding) {
    const auto reminder = size % padding;
    if (reminder > 0) {
        size += padding - reminder;
    }
    return size;
}

}

MergeAllocator::MergeAllocator(ArenaHolder& holder) noexcept
    : holder { holder }
    , head { createNodeIn(holder.begin(), holder.size()) } {
    assert(holder.size() >= sizeof(Node));
}

char* MergeAllocator::allocate(size_t size) noexcept {
    if (size == 0) {
        return nullptr;
    }
    // we can allocate only fold of sizeof(Node) space
    size = padding(size, sizeof(Node));

    Node* best_candidate = nullptr;
    Node* prev_of_best = nullptr;
    for (Node *iter = head, *prev = nullptr; iter != nullptr; prev = iter, iter = iter->next) {
        if (iter->size >= size && (best_candidate == nullptr || iter->size < best_candidate->size)) {
            best_candidate = iter;
            prev_of_best = prev;
            if (best_candidate->size == size) {
                break;
            }
        }
    }

    if (best_candidate != nullptr) {
        const auto allocated_space = reinterpret_cast<char*>(best_candidate);
        const auto left = best_candidate->size - size;
        const auto new_node = left >= sizeof(Node) ?
                        createNodeIn(allocated_space + size, left, best_candidate->next) :
                        best_candidate->next;
        (prev_of_best != nullptr ? prev_of_best->next : head) = new_node;
        return allocated_space;
    }

    return nullptr;
}

void MergeAllocator::deallocate(char* ptr, size_t size) noexcept {
    // we can allocate only fold of sizeof(Node) space
    size = padding(size, sizeof(Node));

    const auto end = ptr + size;
    Node* prev = nullptr;
    for (auto iter = head; iter != nullptr; prev = iter, iter = iter->next) {
        if (reinterpret_cast<char*>(iter) >= end) {
            if (prev != nullptr) {
                prev->next = mergeLeftAndReturnPrev(iter, ptr, size);
                mergeRight(prev, reinterpret_cast<char*>(prev->next), prev->next->size);
            } else {
                head = mergeLeftAndReturnPrev(iter, ptr, size);
            }
            return;
        }
    }

    if (prev != nullptr) {
        mergeRight(prev, ptr, size);
    } else {
        head = createNodeIn(ptr, size);
    }
}

