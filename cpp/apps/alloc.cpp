
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