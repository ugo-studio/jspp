#pragma once
#include <deque>
#include <functional>

namespace jspp {
    class Scheduler {
    public:
        using Task = std::function<void()>;
        
        static Scheduler& instance() {
            static Scheduler s;
            return s;
        }

        void enqueue(Task task) {
            tasks.push_back(std::move(task));
        }

        void run() {
            // Process tasks until the queue is empty.
            // Note: Tasks executing can enqueue new tasks.
            while (!tasks.empty()) {
                Task task = tasks.front();
                tasks.pop_front();
                task();
            }
        }
        
        bool has_tasks() const {
            return !tasks.empty();
        }

    private:
        std::deque<Task> tasks;
    };
}
