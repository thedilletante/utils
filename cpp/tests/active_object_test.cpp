#include <gtest/gtest.h>

#include <iostream>
#include <thread>
#include <future>

#include "active_object.h"

TEST(ActiveObjectTest, testWithState) {

    class State {
    public:
        void f1() {
            std::cout << std::this_thread::get_id() << " f1" << std::endl;
        }

        void f2(int i) {
            std::cout << std::this_thread::get_id() << " f2 with " << i << std::endl;
        }

        int f3(int i) {
            std::cout << std::this_thread::get_id() << " f3 with " << i << std::endl;
            return 2;
        }
    };

    utils::ActiveObject<State> a;
    a.sync(&State::f1);
    a.async([](){
        std::cout << std::this_thread::get_id() << " async f1 finished" << std::endl;
    }, &State::f1);

    std::promise<void> everythingIsDone;


    a.async([&everythingIsDone](){
        std::cout << std::this_thread::get_id() << " async f2 finished" << std::endl;
        everythingIsDone.set_value();
    }, &State::f2, 5);

//    a.sync(&State::f2, 4);

    everythingIsDone.get_future().wait();
}

TEST(ActiveObjectTest, testWithA) {
    class A{
    public:
        char c;
        A(char _c) : c(_c) {}

        char getChar(char a, int i) {
            std::cout << "invoced with " << a << " and " << i << std::endl;
            return c;
        }
    };

    utils::ActiveObject<A> aa { 'i' };



    aa.async([](char a){
        std::cout << std::this_thread::get_id() << " result " << a << std::endl;
    }, &A::getChar, 'd', 3);
}