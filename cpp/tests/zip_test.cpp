//
// Created by Mikhail Pokhikhilov on 13.04.17.
//

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <vector>
#include <list>

#include "zip.h"

using ::testing::ContainerEq;

using ::utils::make_zipped;

TEST(zip_test, zip_with_two_containers) {
    std::list<int> a  { 1, 2, 3, 4 };
    std::vector<int> b{23, 2, 1};
    auto actual = make_zipped<std::vector<std::tuple<int, int>>>(a, b);

    ASSERT_THAT(actual, ContainerEq(std::vector<std::tuple<int, int>> {
            std::make_tuple(1, 23),
            std::make_tuple(2, 2),
            std::make_tuple(3, 1)
    }));
}

TEST(zip_test, zip_with_three_containers) {
    std::list<int> a { 1, 2, 3, 4, 5};
    std::vector<std::string> b { "one", "two", "three"};
    auto c = { 1.0, 2.5, 3.7, 4.5 };
    auto actual = make_zipped<std::vector<std::tuple<int, std::string, float>>>(a, b, c);

    ASSERT_THAT(actual, ContainerEq(std::vector<std::tuple<int, std::string, float>> {
            std::make_tuple(1, "one", 1.0),
            std::make_tuple(2, "two", 2.5),
            std::make_tuple(3, "three", 3.7),
    }));
}

TEST(zip_test, zip_with_containers_with_empty_one) {
    std::list<int> a;
    std::vector<int> b {1, 2, 3};
    auto actual = make_zipped<std::vector<std::tuple<int, int>>>(a, b);

    ASSERT_THAT(actual, ContainerEq(std::vector<std::tuple<int, int>>()));
}