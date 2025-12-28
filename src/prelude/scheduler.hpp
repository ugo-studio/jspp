#pragma once
#include <deque>
#include <functional>
#include <chrono>
#include <queue>
#include <unordered_set>
#include <vector>
#include <thread>
#include <iostream>

namespace jspp {
    class Scheduler {
    public:
        using Task = std::function<void()>;
        using TimePoint = std::chrono::steady_clock::time_point;
        
        struct Timer {
            size_t id;
            TimePoint next_run;
            std::chrono::milliseconds interval; // 0 if not repeating
            Task task;
            
            // Min-heap priority queue (smallest time at top)
            bool operator>(const Timer& other) const {
                return next_run > other.next_run;
            }
        };

        static Scheduler& instance() {
            static Scheduler s;
            return s;
        }

        void enqueue(Task task) {
            tasks.push_back(std::move(task));
        }

        size_t set_timeout(Task task, size_t delay_ms) {
            return schedule_timer(std::move(task), delay_ms, false);
        }

        size_t set_interval(Task task, size_t delay_ms) {
             return schedule_timer(std::move(task), delay_ms, true);
        }
        
        void clear_timer(size_t id) {
            cancelled_timers.insert(id);
        }

        void run() {
            while (true) {
                bool has_work = false;

                // 1. Process all immediate tasks (microtask/task queue)
                while (!tasks.empty()) {
                    Task task = tasks.front();
                    tasks.pop_front();
                    task();
                    has_work = true;
                }

                // 2. Process timers
                auto now = std::chrono::steady_clock::now();
                while (!timers.empty()) {
                    // Peek top
                    const auto& top = timers.top();
                    
                    // Cleanup cancelled timers lazily
                    if (cancelled_timers.count(top.id)) {
                        cancelled_timers.erase(top.id);
                        timers.pop();
                        continue;
                    }
                    
                    if (top.next_run <= now) {
                        // Timer is ready
                        Timer t = top;
                        timers.pop();
                        
                        // Execute task
                        t.task();
                        has_work = true;
                        
                        // Reschedule if interval and not cancelled during execution
                        if (t.interval.count() > 0 && cancelled_timers.find(t.id) == cancelled_timers.end()) {
                            // Drift-safe(ish) scheduling: run next interval relative to now
                            // Note: JS allows drift.
                            t.next_run = std::chrono::steady_clock::now() + t.interval;
                            timers.push(t);
                        }
                    } else {
                        break; // Next timer is in the future
                    }
                }

                // 3. Exit or Wait
                if (!has_work) {
                    if (tasks.empty() && timers.empty()) {
                        break; // No pending work, exit event loop
                    }
                    
                    if (!timers.empty()) {
                        auto next_time = timers.top().next_run;
                        std::this_thread::sleep_until(next_time);
                    }
                }
            }
        }
        
        bool has_tasks() const {
            return !tasks.empty() || !timers.empty();
        }

    private:
        std::deque<Task> tasks;
        std::priority_queue<Timer, std::vector<Timer>, std::greater<Timer>> timers;
        std::unordered_set<size_t> cancelled_timers;
        size_t next_timer_id = 1;
        const size_t MAX_TIMER_ID = 2147483647; // 2^31 - 1

        size_t schedule_timer(Task task, size_t delay_ms, bool repeat) {
            size_t id = next_timer_id++;
            if (next_timer_id > MAX_TIMER_ID) {
                next_timer_id = 1;
            }
            
            // If we wrap around, ensure we don't accidentally treat this new ID as cancelled
            // (in case an old timer with this ID is still lingering in the 'cancelled' set)
            if (cancelled_timers.count(id)) {
                cancelled_timers.erase(id);
            }
            
            auto now = std::chrono::steady_clock::now();
            
            Timer t;
            t.id = id;
            t.next_run = now + std::chrono::milliseconds(delay_ms);
            t.interval = repeat ? std::chrono::milliseconds(delay_ms) : std::chrono::milliseconds(0);
            t.task = std::move(task);
            
            timers.push(t);
            return id;
        }
    };
}