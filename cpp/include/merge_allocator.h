//
// Created by dilletante on 06.05.18.
//

#ifndef CPP_UTILS_MERGE_ALLOCATOR_H
#define CPP_UTILS_MERGE_ALLOCATOR_H

#include <cstddef>

class ArenaHolder {
public:
    virtual ~ArenaHolder() = default;
    virtual char* begin() noexcept = 0;
    virtual size_t size() const noexcept = 0;
};

class MergeAllocator {
public:
    explicit MergeAllocator(ArenaHolder& holder) noexcept ;

    MergeAllocator(const MergeAllocator& copy) = delete;
    MergeAllocator& operator=(const MergeAllocator& copy) = delete;

    char* allocate(size_t size) noexcept ;
    void deallocate(char* ptr, size_t size) noexcept ;

private:
    struct Node {
        size_t size;
        Node* next;

        static Node* makeOn(char* place, size_t size, Node* next) noexcept;
    };

    static Node* mergeLeftAndReturnPrev(Node* right, char* block, size_t size) noexcept;
    static void mergeRight(Node* left, char* block, size_t size) noexcept;

private:
    ArenaHolder& holder;
    Node* head;
};


#endif //CPP_UTILS_MERGE_ALLOCATOR_H
