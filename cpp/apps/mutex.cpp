#include <iostream>
#include <vector>
#include <algorithm>
#include <iterator>
#include <memory>
#include <mutex>
#include <thread>
#include <list>

template <class T, class mutex_type = std::mutex>
class mutex {
public:
    explicit mutex(T&& t)
            : t { std::move(t) } {}

    mutex(mutex&& m)
            : t { std::move(m.t) } {}

    class mutexed_wrapper {
    public:
        explicit mutexed_wrapper(mutex& m)
                : mutex_ { m }
                , lock { mutex_.m } {}

        mutexed_wrapper(mutexed_wrapper&& wrapper)
                : mutex_ { wrapper.mutex_ }
                , lock { mutex_.m } {}

        T* operator->() {
            return &mutex_.t;
        }

        T& operator*() {
            return mutex_.t;
        }

        operator bool() {
            return true;
        }

    private:
        mutex& mutex_;
        std::lock_guard<mutex_type> lock;
    };

    mutexed_wrapper lock() {
        return mutexed_wrapper(*this);
    }


private:
    T t;
    mutex_type m;
};

template <class T>
mutex<T> make_mutex(T&& t) {
    return mutex<T>(std::move(t));
}

int main() {

    auto string = make_mutex(std::string("one"));


    auto second = std::thread([&string](){
        if (auto locked_string = string.lock()) {
            std::cout << *locked_string << std::endl;
        }
    });
    auto first = std::thread([&string](){
        if (auto locked_string = string.lock()) {
            locked_string->at(1) = 'o';
        }
    });

    first.join();
    second.join();

    return 0;
}