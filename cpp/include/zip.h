#ifndef CPP_ZIP_H
#define CPP_ZIP_H

#include <iterator>
#include <tuple>
#include <type_traits>
#include <utility>

namespace utils {

namespace __impl {

template <typename ...Args>
class ZipContainer;

} // namespace __impl

template <typename ...Args>
auto zip(Args&& ...containers) {
    return __impl::ZipContainer<Args...>(std::forward<Args>(containers)...);
}

template <class ContainerT, typename ...Args>
auto make_zipped(Args&& ...containers) {
    auto zipped = __impl::ZipContainer<Args...>(std::forward<Args>(containers)...);
    ContainerT ret(zipped.begin(), zipped.end());
    return ret;
}


namespace __impl {

template <class T>
using value_type_decay_t = typename std::remove_reference<T>::type::value_type;

template <class T>
using iterator_type_decay_t = decltype(std::begin(std::declval<T>()));

template <typename... T, std::size_t... I>
auto subtuple_(const std::tuple<T...>& t, std::index_sequence<I...>) {
    return std::make_tuple(std::get<I>(t)...);
}

template <int Trim, typename... T>
auto subtuple(const std::tuple<T...>& t) {
    return subtuple_(t, std::make_index_sequence<sizeof...(T) - Trim>());
}

template <unsigned N>
struct IteratorTupleHelper {
public:
    template <class ...T>
    static bool is(const std::tuple<T...> &current, const std::tuple<T...> &end) {
        return std::get<N>(current) == std::get<N>(end) || IteratorTupleHelper<N - 1>::is(current, end);
    };

    template <class ...T>
    static void increment(std::tuple<T...> &iter) {
        ++std::get<N>(iter);
        IteratorTupleHelper<N - 1>::increment(iter);
    }

    template <class ...T>
    static auto iterValues(const std::tuple<T...> &iter) {
        return std::tuple_cat(IteratorTupleHelper<N-1>::iterValues(subtuple<1>(iter)), std::tie(*std::get<N>(iter)));
    }
};

template <>
struct IteratorTupleHelper<0> {
public:
    template <class ...T>
    static bool is(const std::tuple<T...> &current, const std::tuple<T...> &end) {
        return std::get<0>(current) == std::get<0>(end);
    };

    template <class ...T>
    static void increment(std::tuple<T...> &iter) {
        ++std::get<0>(iter);
    }

    template <class ...T>
    static auto iterValues(const std::tuple<T...> &iter) {
        return std::tie(*std::get<0>(iter));
    }
};

template <class ...T>
bool is(const std::tuple<T...> &current, const std::tuple<T...> &end) {
    return IteratorTupleHelper<std::tuple_size<std::tuple<T...>>::value - 1>::is(current, end);
};

template <class ...T>
void increment(std::tuple<T...> &iter) {
    IteratorTupleHelper<std::tuple_size<std::tuple<T...>>::value - 1>::increment(iter);
}

template <class ...T>
auto getValues(const std::tuple<T...> &iter) {
    return IteratorTupleHelper<std::tuple_size<std::tuple<T...>>::value - 1>::iterValues(iter);
}

template <typename ...Args>
class ZipContainer {
public:
    class ZipIterator {
    public:
        using value_type = std::tuple<value_type_decay_t<Args>...>;
        using pointer_type = value_type*;
        using reference_type = value_type&;
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using pointer = pointer_type;
        using reference = reference_type;

        explicit ZipIterator(std::tuple<iterator_type_decay_t<Args>...> iter)
                : current { std::move(iter) } {}

        std::tuple<value_type_decay_t<Args>...> operator*() const {
            return getValues(current);
        }

        ZipIterator& operator++() {
            increment(current);
            return *this;
        }

        ZipIterator operator++(int) {
            ZipIterator tmp { current };
            increment(current);
            return tmp;
        }

        bool operator==(const ZipIterator& other) {
            return is(current, other.current);
        }

        bool operator!=(const ZipIterator& other) {
            return !is(current, other.current);
        }

    private:
        std::tuple<iterator_type_decay_t<Args>...> current;
    };
public:
    explicit ZipContainer(Args&& ...containers)
            : begins { std::make_tuple(std::begin(containers)...) }
            , ends { std::make_tuple(std::end(containers)...) }
    {}

    ZipIterator begin() const { return ZipIterator(begins); }
    ZipIterator end() const { return ZipIterator(ends); }
private:
    std::tuple<iterator_type_decay_t<Args>...> begins;
    std::tuple<iterator_type_decay_t<Args>...> ends;

};

} // namespace __impl
} // namespace utils

#endif // CPP_ZIP_H
