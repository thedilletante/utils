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


template <size_t ArenaSize>
struct NodesInPlaceAllocator {
public:
    struct Node {
        size_t size;
        Node* next;
    };

    char arena[ArenaSize];
    Node* first = new (arena) Node { ArenaSize, nullptr };

    char* allocate(size_t size) {

        for (Node* iter = first, *prev = nullptr;
             iter != nullptr;
             prev = iter, iter = iter->next) {

            if (iter->size == size || iter->size - size < sizeof(Node)) {
                if (prev != nullptr) {
                    prev->next = iter->next;
                } else {
                    first = iter->next;
                }
                iter->~Node();
                return reinterpret_cast<char*>(iter);
            } else if (iter->size - size >= sizeof(Node)) {
                auto ret = reinterpret_cast<char *>(iter);
                auto next = iter->next;
                auto new_size = iter->size - size;
                iter->~Node();
                iter = new (ret + size) Node { new_size, next };
                if (prev != nullptr) {
                    prev->next = iter;
                } else {
                    first = iter;
                }
            }
        }

        return nullptr;
    }

    void deallocate(char* ptr, size_t size) {
        Node* iter = first;
        Node* prev = nullptr;

        for (;
             iter != nullptr;
             prev = iter, iter = iter->next) {
            auto block = reinterpret_cast<char*>(iter);
            auto prev_block = reinterpret_cast<char*>(prev);

            if (block > ptr) {
                if (block - ptr < sizeof(Node)) {
                    if (prev != nullptr) {
                        if (ptr - prev_block < sizeof(Node)) {
                            prev->next = iter->next;
                            prev->size += iter->size + size + (ptr - prev_block) + (block - ptr);
                        } else {
                            auto new_node = new (ptr) Node { iter->size + size + (block - ptr)};
                            iter->~Node();
                            prev->next = new_node;
                        }
                    } else {
                        first = new (ptr) Node { iter->size + size + (block - ptr) };
                        iter->~Node();
                    }
                } else {
                    if (prev != nullptr) {
                        if (ptr - prev_block < sizeof(Node)) {
                            prev->size += size + (ptr - prev_block);
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


int main() {
    NodesInPlaceAllocator<1024> alloc;
    print("init", alloc);

    auto ptr1 = alloc.allocate(8);
    print("allocated 8", alloc);

    alloc.deallocate(ptr1, 8);
    print("deallocated 8", alloc);

//    auto ptr2 = alloc.allocate(64);
//    print("allocated 64", alloc);
//
//    auto ptr3 = alloc.allocate(16);
//    print("allocated 16", alloc);





//    MyAllocator<1024> alloc;
//    print("init", alloc);
//
//    auto ptr = alloc.allocate(8);
//    print("allocated 8", alloc);
//
//    auto ptr1 = alloc.allocate(64);
//    print("allocated 64", alloc);
//
//    alloc.deallocate(ptr, 8);
//    print("deallocated 8", alloc);
//
//    auto ptr2 = alloc.allocate(16);
//    print("allocated 16", alloc);
//
//    auto ptr3 = alloc.allocate(8);
//    print("allocated 8", alloc);
//
//    alloc.deallocate(ptr1, 64);
//    print("deallocated 64", alloc);
//
//    alloc.deallocate(ptr2, 16);
//    print("deallocated 16", alloc);
//
//    alloc.deallocate(ptr3, 8);
//    print("deallocated 8", alloc);

    return 0;
}