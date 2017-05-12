//
// Created by Mikhail Pokhikhilov on 13.04.17.
//

#ifndef CPP_ZIP_H
#define CPP_ZIP_H

#include <tuple>
#include <list>
#include <type_traits>
#include <iostream>
#include <iterator>
#include <utility>

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
private:
    class ZipIterator {
    public:
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

template <typename ...Args>
auto zip(Args&& ...containers) {
    return ZipContainer<Args...>(std::forward<Args>(containers)...);
//    std::list<std::tuple<value_type_decay_t<Args>...>> result;
//
//    auto currents = std::make_tuple(std::begin(containers)...);
//    auto ends = std::make_tuple(std::end(containers)...);
//
//    while (currents != ends) {
//        result.push_back(getValues(currents));
//        increment(currents);
//    }
//
//    return result;
}

//template <class C1, class C2>
//std::list<std::tuple<typename C1::value_type, typename C2::value_type>> zip(const C1& c1, const C2& c2) {
//    std::list<std::tuple<typename C1::value_type, typename C2::value_type>> result;
//    auto c1_cur = std::begin(c1), c1_end = std::end(c1);
//    auto c2_cur = std::begin(c2), c2_end = std::end(c2);
//    while (c1_cur != c1_end && c2_cur != c2_end) {
//        result.push_back(std::make_tuple(*c1_cur, *c2_cur));
//        ++c1_cur;
//        ++c2_cur;
//    }
//    return result;
//}

#endif // CPP_ZIP_H
