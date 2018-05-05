#include <iostream>
#include <vector>
#include <algorithm>
#include <iterator>
#include <memory>
#include <mutex>
#include <thread>
#include <list>

template <size_t ArenaSize>
struct MyAllocator {
public:
    struct Node {
        char* start_ptr;
        size_t size;
    };

    char* allocate(size_t size) {
        const auto free_node = std::find_if(std::begin(nodes), std::end(nodes), [size](const Node& node){
            return node.size >= size;
        });

        if (free_node == std::end(nodes)) {
            return nullptr;
        }

        const auto ret = free_node->start_ptr;
        free_node->size -= size;
        free_node->start_ptr = (char *)ret + size;

        if (free_node->size == 0) {
            nodes.erase(free_node);
        }

        return ret;
    }

    void deallocate(char* ptr, size_t size) {
        auto upper_bound = std::find_if(std::begin(nodes), std::end(nodes), [ptr](const Node& node){
            return node.start_ptr > ptr;
        });

        if (upper_bound == std::end(nodes)) {
            nodes.emplace_back(Node{ptr, size});
            return;
        }

        if (ptr + size == upper_bound->start_ptr) {
            upper_bound->start_ptr = ptr;
            upper_bound->size += size;

            if (upper_bound != std::begin(nodes)) {
                auto left = upper_bound;
                --left;
                if (left->start_ptr + left->size == upper_bound->start_ptr) {
                    left->size += upper_bound->size;
                    nodes.erase(upper_bound);
                }
            }
            return;
        }

        if (upper_bound != std::begin(nodes)) {
            auto left = upper_bound--;

            if (left->start_ptr + left->size == ptr) {
                left->size += size;
                return;
            }
        }

        nodes.emplace(upper_bound, Node{ptr, size});
    }

    explicit MyAllocator() {
        nodes.emplace_back(Node{arena, ArenaSize});
    }

    char arena[ArenaSize];
    std::list<Node> nodes;
};

template <size_t N>
void print(const char* str, const MyAllocator<N>& allocator) {
    std::cout << "==== allocator<" << N << ">, " << (void *)allocator.arena << " after: " << str << "\n";
    for (const auto& node : allocator.nodes) {
        std::cout << " - node: " << (void *)node.start_ptr << " of " << node.size << " bytes\n";
    }
}

class Arena {
public:
    virtual ~Arena() = default;
    virtual char* allocate(size_t size) = 0;
    virtual void deallocate(char* ptr, size_t size) = 0;
};


template <size_t ArenaSize>
struct NodesInPlaceAllocator
    : Arena {
public:
    struct Node {
        size_t size;
        Node* next;
    };

    char arena[ArenaSize];
    Node* first = new (arena) Node { ArenaSize, nullptr };

    char* allocate(size_t size) override {
        for (Node* iter = first, *prev = nullptr;
             iter != nullptr;
             prev = iter, iter = iter->next) {

            if (iter->size >= size) {
                if (iter->size == size || iter->size - size < sizeof(Node)) {
                    if (prev != nullptr) {
                        prev->next = iter->next;
                    } else {
                        first = iter->next;
                    }
                    return reinterpret_cast<char*>(iter);
                } else if (iter->size - size >= sizeof(Node)) {
                    auto ret = reinterpret_cast<char *>(iter);
                    auto next = iter->next;
                    auto new_size = iter->size - size;
                    iter = new (ret + size) Node { new_size, next };
                    if (prev != nullptr) {
                        prev->next = iter;
                    } else {
                        first = iter;
                    }
                    return ret;
                }
            }
        }

        return nullptr;
    }

    void deallocate(char* ptr, size_t size) override {
        Node* iter = first;
        Node* prev = nullptr;
        auto end = ptr + size;

        for (;
             iter != nullptr;
             prev = iter, iter = iter->next) {
            auto block = reinterpret_cast<char*>(iter);
            auto prev_block = reinterpret_cast<char*>(prev);

            if (block >= end) {
                const auto right_diff = block - end;
                if (right_diff < sizeof(Node)) {
                    if (prev != nullptr) {
                        const auto left_diff = ptr - (prev_block + prev->size);
                        if (left_diff < sizeof(Node)) {
                            prev->next = iter->next;
                            prev->size += iter->size + size + left_diff + right_diff;
                        } else {
                            auto new_node = new (ptr) Node { iter->size + size + right_diff};
                            prev->next = new_node;
                        }
                    } else {
                        first = new (ptr) Node { iter->size + size + right_diff, nullptr };
                    }
                } else {
                    if (prev != nullptr) {
                        const auto left_diff = ptr - (prev_block + prev->size);
                        if (left_diff < sizeof(Node)) {
                            prev->size += size + left_diff;
                        } else {
                            auto new_node = new (ptr) Node { size, prev->next };
                            prev->next = new_node;
                        }
                    } else {
                        first = new (ptr) Node { size, iter };
                    }
                }
                return;
            }
        }

        if (prev != nullptr) {
            auto prev_block = reinterpret_cast<char*>(prev);
            if (ptr - prev_block < sizeof(Node)) {
                prev->size += ptr - prev_block + size;
            } else {
                auto node = new (ptr) Node { size, prev->next };
                prev->next = node;
            }
        }

        first = new (ptr) Node { size, nullptr };
    }
};

template <size_t N>
void print(const char* str, const NodesInPlaceAllocator<N>& allocator) {
    std::cout << "==== allocator<" << N << ">, " << (void *)allocator.arena << " after: " << str << "\n";
    for (auto iter = allocator.first; iter != nullptr; iter = iter->next) {
        std::cout << " - node: " << (void *)iter << " of " << iter->size << " bytes\n";
    }
}




template <class Tp>
struct SimpleAllocator {
    using value_type = Tp;
    using size_type = typename std::allocator<Tp>::size_type;
    using difference_type = typename std::allocator<Tp>::difference_type;
    using pointer = typename std::allocator<Tp>::pointer;
    using const_pointer = typename std::allocator<Tp>::const_pointer;
    // "reference" не входит Allocator Requirements,
    // но libstdc++ думает что она всегда работает с std::allocator.
    using reference = typename std::allocator<Tp>::reference;
    using const_reference = typename std::allocator<Tp>::const_reference;
    Arena& arena;

    SimpleAllocator(Arena& arena)
        : arena { arena } {}

    template <class T>
    SimpleAllocator(const SimpleAllocator<T>& other)
        : arena { other.arena } {};

    value_type * allocate(std::size_t n) {
        return reinterpret_cast<value_type *>(arena.allocate(sizeof(Tp) * n));
    }

    void deallocate(Tp* p, std::size_t n) {
        arena.deallocate(reinterpret_cast<char*>(p), sizeof(Tp) * n);
    }

    template<class U, class... Args> void construct(U* p, Args&&... args) { std::allocator<Tp>().construct(p, std::forward<Args>(args)...); }
    template<class U> void destroy(U* p) { std::allocator<Tp>().destroy(p); }
    template<class U> struct rebind { using other = SimpleAllocator<U>; };
};

template <class T, class U>
bool operator==(const SimpleAllocator<T>& lhs, const SimpleAllocator<U>& rhs) {
    return &lhs.arena == &rhs.arena;
};

template <class T, class U>
bool operator!=(const SimpleAllocator<T>& lhs, const SimpleAllocator<U>& rhs) {
    return !(lhs == rhs);
};

int main() {
//    NodesInPlaceAllocator<1024 * 10> alloc;
//    print("init", alloc);
//
//    auto ptr1 = alloc.allocate(8);
//    print("allocated 8", alloc);
//
//    auto ptr2 = alloc.allocate(64);
//    print("allocated 64", alloc);
//
//    alloc.deallocate(ptr1, 8);
//    print("deallocated 8", alloc);
//
//    auto ptr3 = alloc.allocate(16);
//    print("allocated 16", alloc);
//
//    auto ptr4 = alloc.allocate(8);
//    print("allocated 8", alloc);
//
//    alloc.deallocate(ptr2, 64);
//    print("deallocated 64", alloc);
//
//    alloc.deallocate(ptr3, 16);
//    print("deallocated 16", alloc);
//
//    alloc.deallocate(ptr4, 8);
//    print("deallocated 8", alloc);

    NodesInPlaceAllocator<1024> arena;
    print("inited", arena);
    {

        std::list<int, SimpleAllocator<int>> l{SimpleAllocator<int>(arena)};

        print("list created", arena);
        l.push_back(3);
        l.push_back(5);
        l.push_back(577);
        l.erase(++l.begin());
        l.push_back(6);
        print("pushed 2 elements", arena);
    }
    print("list deallocated", arena);
//
    return 0;
}