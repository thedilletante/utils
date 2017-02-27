#include <iostream>
#include <thread>
#include <future>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>

template <class T>
struct SmartPointerTraits {
    using UniquePtr = std::unique_ptr<T>;
    using SharedPtr = std::shared_ptr<T>;
    using WeakPtr = std::weak_ptr<T>;
};

template <class T>
class ConcurrentQueue
    : public std::enable_shared_from_this<ConcurrentQueue<T>>
    , public SmartPointerTraits<ConcurrentQueue<T>> {
public:
    void push(T&& elem) {
        std::lock_guard<std::mutex> guard(mutex);
        queue.push(elem);
        condition_variable.notify_all();
    }

    void clear() {
        std::lock_guard<std::mutex> guard(mutex);
        queue = std::queue<T>();
    }

    template <class CALLBACK>
    void wait(CALLBACK callback) {
        std::unique_lock<std::mutex> lock(mutex);
        condition_variable.wait(lock, [this](){
            return !queue.empty();
        });

        auto elem = std::move(queue.front());
        queue.pop();
        lock.unlock();

        callback(elem);
    }

private:
    std::queue<T> queue;
    std::mutex mutex;
    std::condition_variable condition_variable;
};


template <class O>
class ActiveObject {
private:
    struct Message {
        enum class Type : uint8_t {
            Action,
            Stop
        };
        using ActionT = std::function<void(O&)>;

        Type type;
        ActionT task;

        static Message action(ActionT task) {
            return Message{Type::Action, task};
        }

        static Message stop() {
            return Message{Type::Stop};
        }
    };

    using Queue = ConcurrentQueue<Message>;

public:
    template <class... Args>
    ActiveObject(Args&&... args)
        : queue { std::make_shared<Queue>() }
        , working_thread { &ActiveObject::task_processor<Args...>, queue, std::forward<Args>(args)... }
    {}

    ActiveObject(const ActiveObject&) = delete;
    ActiveObject& operator=(const ActiveObject&) = delete;

    ~ActiveObject() {
        queue->push(Message::stop());
        working_thread.join();
    }

    template <class T, class ...Args>
    T sync(T (O::*f)(Args...), Args&&... args) {
        std::promise<T> promise;
        queue->push(Message::action([&promise, f, &args...](O& object) {
            promise.set_value(std::bind(f, &object, std::forward<Args>(args)...)());
        }));
        return promise.get_future().get();
    }

    template <class ...Args>
    void sync(void (O::*f)(Args...), Args&&... args) {
        std::promise<void> promise;
        queue->push(Message::action([&promise, f, &args...](O& object) {
            std::bind(f, &object, std::forward<Args>(args)...)();
            promise.set_value();
        }));
        promise.get_future().wait();
    }

    template <class C, class T, class ...Args>
    void async(C callback, T (O::*f)(Args...), Args&&... args) {
        queue->push(Message::action([this, callback, f, &args...](O& object) {
            std::function<T()> result_provider = [&object, f, &args...](){
                return std::bind(f, &object, std::forward<Args>(args)...)();
            };
            pass_result(callback, result_provider);
        }));
    }

    void cancel() {
        // it is dangerous if there are sync operations in the queue
        // we have to have mechanism to tell waiters about cancellation
        queue->clear();
    }

private:
    template<class R, class P>
    void pass_result(R receiver, P result_provider) {
        receiver(result_provider());
    }

    template<class R>
    void pass_result(R receiver, std::function<void()>) {
        receiver();
    }

    template <class... Args>
    static void task_processor(typename Queue::SharedPtr queue, Args&&... args) {

        // need to provide user-defined creator function
        O obj{ std::forward<Args>(args)... };

        bool need_to_stop = false;
        while (!need_to_stop) {
            queue->wait([&need_to_stop, &obj](Message msg){
                switch (msg.type) {
                    case Message::Type::Action: {
                        msg.task(obj);
                        break;
                    }
                    case Message::Type::Stop: {
                        need_to_stop = true;
                        break;
                    }
                    default: {
                        need_to_stop = true;
                        // exception ???
                    }
                }
            });
        }
    }

private:
    typename Queue::SharedPtr queue;
    std::thread working_thread;
};





class State {
public:
    // ctor

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

class A{
public:
    char c;
    A(char _c) : c(_c) {}

    char getChar(char a, int i) {
        std::cout << "invoced with " << a << " and " << i << std::endl;
        return c;
    }
};

int main() {

    // I want a wrapper around State
    // to execute State's methods at specific thread(s)
    // I want an ability to execute them
    // synchronously and asynchronously providing a callback
    // to be invoked at the end

    ActiveObject<State> a;
    a.sync(&State::f1);
    a.async([](){
        std::cout << std::this_thread::get_id() << " async f1 finished" << std::endl;
    }, &State::f1);

    a.sync(&State::f2, 4);
    a.async([](){
        std::cout << std::this_thread::get_id() << " async f2 finished" << std::endl;
    }, &State::f2, 5);

    auto i = a.sync(&State::f3, 4);
    a.async([](int i){
        std::cout << std::this_thread::get_id() << " result" << i << std::endl;
    }, &State::f3, 5);


    ActiveObject<A> aa { 'i' };

    aa.async([](char a){
        std::cout << std::this_thread::get_id() << " result " << a << std::endl;
    }, &A::getChar, 'd', 3);

    return 0;
}