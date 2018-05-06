//
// Created by dilletante on 06.05.18.
//

#include <cassert>
#include "merge_allocator.h"

MergeAllocator::MergeAllocator(ArenaHolder& holder) noexcept
    : holder { holder }
    , head { Node::makeOn(holder.begin(), holder.size(), nullptr )}{
    assert(holder.size() >= sizeof(Node));
}

char* MergeAllocator::allocate(size_t size) noexcept {
    // we can allocate only fold of sizeof(Node) space
    const auto reminder = size % sizeof(Node);
    if (reminder > 0) {
        size += sizeof(Node) - reminder;
    }
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
                        Node::makeOn(allocated_space + size, left, best_candidate->next) :
                        best_candidate->next;
        (prev_of_best != nullptr ? prev_of_best->next : head) = new_node;
        return allocated_space;
    }

    return nullptr;
}

void MergeAllocator::deallocate(char* ptr, size_t size) noexcept {
    // we can allocate only fold of sizeof(Node) space
    const auto reminder = size % sizeof(Node);
    if (reminder > 0) {
        size += sizeof(Node) - reminder;
    }
    Node* prev = nullptr;
    auto end = ptr + size;

    for (auto iter = head;
         iter != nullptr;
         prev = iter, iter = iter->next) {

        const auto block = reinterpret_cast<char*>(iter);
        if (block >= end) {
            if (prev != nullptr) {
                prev->next = mergeLeftAndReturnPrev(iter, ptr, size);
                mergeRight(prev, reinterpret_cast<char*>(prev->next), prev->next->size);
            } else {
                head = mergeLeftAndReturnPrev(iter, ptr, size);
            }
            return;
        }
    }

    auto tail = prev;
    if (tail != nullptr) {
        mergeRight(tail, ptr, size);
    } else {
        head = Node::makeOn(ptr, size, nullptr);
    }

}

MergeAllocator::Node* MergeAllocator::mergeLeftAndReturnPrev(Node* right, char* block, size_t size) noexcept {
    const auto right_block = reinterpret_cast<char*>(right);
    const auto end = block + size;
    assert(end <= right_block);

    const auto left = right_block - end;
    if (left < sizeof(Node)) {
        return Node::makeOn(block, right->size + size + left, right->next);
    } else {
        return Node::makeOn(block, size, right);
    }
};

void MergeAllocator::mergeRight(Node* left, char* block, size_t size) noexcept {
    const auto end = reinterpret_cast<char*>(left) + left->size;
    assert(end <= block);

    const auto left_diff = block - end;
    if (left_diff < sizeof(Node)) {
        left->size += size + left_diff;
        left->next = left->next ? left->next->next : nullptr;
    }
}

MergeAllocator::Node *MergeAllocator::Node::makeOn(char *place, size_t size, MergeAllocator::Node *next) noexcept {
    const auto node = reinterpret_cast<Node*>(place);
    node->size = size;
    node->next = next;
    return node;
}
