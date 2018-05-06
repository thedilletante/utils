#include <iostream>
#include <vector>
#include <algorithm>
#include <iterator>
#include <memory>
#include <mutex>
#include <thread>
#include <list>
#include <cassert>

class ArenaHolder {
public:
    virtual ~ArenaHolder() = default;
    virtual char* begin() = 0;
    virtual size_t size() = 0;
};

class Allocator {
public:
    virtual ~Allocator() = default;
    virtual char* allocate(size_t size) = 0;
    virtual void deallocate(char* ptr, size_t size) = 0;
};


struct NodesInPlaceAllocator
    : Allocator {
private:
    struct Node {
        size_t size;
        Node* next;
    };

public:
    explicit NodesInPlaceAllocator(ArenaHolder& holder)
        : holder { holder }
        , head { new (holder.begin()) Node { holder.size(), nullptr } }
    {}

    char* allocate(size_t size) override {
        // we can allocate only fold of sizeof(Node) space
        size = size + size % sizeof(Node);
        for (Node* iter = head, *prev = nullptr;
             iter != nullptr;
             prev = iter, iter = iter->next) {

            if (iter->size >= size) {
                auto allocated_space = reinterpret_cast<char*>(iter);
                auto new_node = iter->next;
                const auto left = iter->size - size;
                if (left >= sizeof(Node)) {
                    new_node = new (allocated_space + size) Node { left, new_node };
                }
                (prev != nullptr ? prev->next : head) = new_node;
                return allocated_space;
            }
        }

        return nullptr;
    }

    void deallocate(char* ptr, size_t size) override {
        // we can allocate only fold of sizeof(Node) space
        size = size + size % sizeof(Node);
        Node* prev = nullptr;
        auto end = ptr + size;

        for (auto iter = head;
             iter != nullptr;
             prev = iter, iter = iter->next) {
            auto block = reinterpret_cast<char*>(iter);
            auto prev_block = reinterpret_cast<char*>(prev);

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
        }

        head = new (ptr) Node { size, nullptr };
    }

    void print(const char* str) const {
        std::cout << "==== allocator<" << holder.size() << ">, " << (void *)holder.begin() << " after: " << str << "\n";
        for (auto iter = head; iter != nullptr; iter = iter->next) {
            std::cout << " - node: " << (void *)iter << " of " << iter->size << " bytes\n";
        }
    }

private:
    static Node* mergeLeftAndReturnPrev(Node* right, char* block, size_t size) {
        auto right_block = reinterpret_cast<char*>(right);
        auto end = block + size;
        assert(end <= right_block);

        const auto left = right_block - end;
        if (left < sizeof(Node)) {
            return new (block) Node { right->size + size + left, right->next };
        } else {
            return new (block) Node { size, right };
        }
    };

    static void mergeRight(Node* left, char* block, size_t size) {
        auto next_next = left->next ? left->next->next : nullptr;
        auto left_block = reinterpret_cast<char*>(left);
        auto end = left_block + left->size;
        assert(end <= block);

        const auto left_diff = block - end;
        if (left_diff < sizeof(Node)) {
          left->size += size + left_diff;
          left->next = left->next->next; 
        } else {
          left->next = new (block) Node { size, left->next };
        }
    }

private:
    ArenaHolder& holder;
    Node* head;
};

void print(const char* str, const NodesInPlaceAllocator& allocator) {
    allocator.print(str);
}

// template <class Tp>
// struct SimpleAllocator {
//     using value_type = Tp;
//     using size_type = typename std::allocator<Tp>::size_type;
//     using difference_type = typename std::allocator<Tp>::difference_type;
//     using pointer = typename std::allocator<Tp>::pointer;
//     using const_pointer = typename std::allocator<Tp>::const_pointer;
//     using reference = typename std::allocator<Tp>::reference;
//     using const_reference = typename std::allocator<Tp>::const_reference;

//     Arena& arena;

//     SimpleAllocator(Arena& arena)
//         : arena { arena } {}

//     template <class T>
//     SimpleAllocator(const SimpleAllocator<T>& other)
//         : arena { other.arena } {};

//     value_type * allocate(std::size_t n) {
//         return reinterpret_cast<value_type *>(arena.allocate(sizeof(Tp) * n));
//     }

//     void deallocate(Tp* p, std::size_t n) {
//         arena.deallocate(reinterpret_cast<char*>(p), sizeof(Tp) * n);
//     }

//     template<class U, class... Args> void construct(U* p, Args&&... args) { std::allocator<Tp>().construct(p, std::forward<Args>(args)...); }
//     template<class U> void destroy(U* p) { std::allocator<Tp>().destroy(p); }
//     template<class U> struct rebind { using other = SimpleAllocator<U>; };
// };

// template <class T, class U>
// bool operator==(const SimpleAllocator<T>& lhs, const SimpleAllocator<U>& rhs) {
//     return &lhs.arena == &rhs.arena;
// };

// template <class T, class U>
// bool operator!=(const SimpleAllocator<T>& lhs, const SimpleAllocator<U>& rhs) {
//     return !(lhs == rhs);
// };

template <size_t N>
class StaticArrayArena
    : public ArenaHolder {
public:
    char* begin() override { return block; }
    size_t size() override { return N; }
private:
    char block[N];
};

class DynamicPoolArena
    : public ArenaHolder {
public:
    explicit DynamicPoolArena(size_t n)
        : block { new char[n] }
        , n { n } {}

    char* begin() override { return block.get(); }
    size_t size() override { return n; }
private:
    std::unique_ptr<char[]> block;
    size_t n;
};

int main() {
    DynamicPoolArena arena(1024 * 1024 * 1024);
    NodesInPlaceAllocator alloc(arena);
    print("init", alloc);

    auto ptr1 = alloc.allocate(8);
    print("allocated 8", alloc);

    auto ptr2 = alloc.allocate(64);
    print("allocated 64", alloc);

    alloc.deallocate(ptr1, 8);
    print("deallocated 8", alloc);

    auto ptr3 = alloc.allocate(16);
    print("allocated 16", alloc);

    auto ptr4 = alloc.allocate(8);
    print("allocated 8", alloc);

    alloc.deallocate(ptr2, 64);
    print("deallocated 64", alloc);

    alloc.deallocate(ptr3, 16);
    print("deallocated 16", alloc);

    alloc.deallocate(ptr4, 8);
    print("deallocated 8", alloc);

    return 0;
}