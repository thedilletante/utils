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
        const auto reminder = size % sizeof(Node);
        if (reminder > 0) {
            size += sizeof(Node) - reminder;
        }
        auto candidate = head;
        Node* best_candidate = nullptr;
        Node* prev_of_best = nullptr;
        for (Node* prev = nullptr; candidate != nullptr; prev = candidate, candidate = candidate->next) {
            if (candidate->size >= size && (best_candidate == nullptr || candidate->size < best_candidate->size)) {
                best_candidate = candidate;
                prev_of_best = prev;
            }
        }

        if (best_candidate != nullptr) {
            auto allocated_space = reinterpret_cast<char*>(best_candidate);
            const auto left = best_candidate->size - size;
            auto new_node = (left < sizeof(Node)) ?
                            best_candidate->next :
                            new (allocated_space + size) Node { left, best_candidate->next };
            (prev_of_best != nullptr ? prev_of_best->next : head) = new_node;
            return allocated_space;
        }

        return nullptr;
    }

    void deallocate(char* ptr, size_t size) override {
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
        } else {
            head = new (ptr) Node { size, nullptr };
        }

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
          left->next = left->next ? left->next->next : nullptr;
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
    DynamicPoolArena arena(1024);
    NodesInPlaceAllocator alloc(arena);
     print("init", alloc);

     auto p1 = alloc.allocate(20);
     auto p2 = alloc.allocate(1);
     auto p3 = alloc.allocate(1);
     auto p4 = alloc.allocate(1);

     alloc.deallocate(p1, 20);
     alloc.deallocate(p3, 1);

     print("setup", alloc);

     auto p5 = alloc.allocate(10);
     print("allocated", alloc);

     alloc.deallocate(p4, 1);
     alloc.deallocate(p5, 1);
     alloc.deallocate(p2, 1);

     print("deallocated", alloc);


    return 0;
}