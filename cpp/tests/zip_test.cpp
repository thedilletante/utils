//
// Created by Mikhail Pokhikhilov on 13.04.17.
//

#include <gtest/gtest.h>
#include <list>
#include "zip.h"

TEST(test, testPrintTheZip) {
    std::list<int> a { 1, 2, 3, 4 };
//    std::vector<char> b{ 'a', 'b', 'c'};
    std::vector<int> b{23, 2, 1};
    for (const auto& elem : zip(a, b)) {
        std::cout << std::get<0>(elem) << " <-> " << std::get<1>(elem) << std::endl;
    }
}

TEST(test, test1) {

    std::list<int> a { 1, 2, 3, 4, 5};
    std::vector<std::string> b { "one", "two", "three"};
    auto c = { 1.0, 2.5, 3.7, 4.5 };

    for (const auto& elem : zip(a, b, c)) {
        std::cout << std::get<0>(elem) << " <> " << std::get<1>(elem) << " <> " << std::get<2>(elem) << std::endl;
    }
}