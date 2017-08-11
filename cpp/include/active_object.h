#ifndef CPP_UTILS_ACTIVE_OBJECT_H
#define CPP_UTILS_ACTIVE_OBJECT_H

#include <iostream>
#include <thread>
#include <future>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <cassert>


namespace utils {

template <class T>
class ConcurrentQueue
        : public std::enable_shared_from_this<ConcurrentQueue<T>> {
public:
    void push(T&& elem) {
        {
            std::lock_guard<std::mutex> guard(mutex);
            queue.push(elem);
        }
        condition_variable.notify_one();
    }

    template <class CALLBACK>
    void clear(CALLBACK callback) {
        std::queue<T> old_queue;
        {
            std::lock_guard<std::mutex> guard(mutex);
            std::swap(old_queue, queue);
        }

        while (!old_queue.empty()) {
            auto elem = std::move(old_queue.front());
            old_queue.pop();

            callback(elem);
        }
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
        queue->clear([](const Message&){
            // TODO: tell waiters about cancellation
        });
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
    static void task_processor(std::shared_ptr<Queue> queue, Args&&... args) {

        // need to provide user-defined creating function
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
                        assert(false && "Unknown message type");
                        need_to_stop = true;
                        break;
                    }
                }
            });
        }
    }

private:
    std::shared_ptr<Queue> queue;
    std::thread working_thread;
};

} // namespace utils


#endif //CPP_UTILS_ACTIVE_OBJECT_H
