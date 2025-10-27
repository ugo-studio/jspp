#pragma once
#include <functional>
#include <future>
#include <condition_variable>
#include <deque>
#include <exception>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
#include <iostream>

// ---------------------
// Microtask scheduler
// ---------------------
class MicrotaskQueue
{
public:
    static MicrotaskQueue &instance()
    {
        static MicrotaskQueue q;
        return q;
    }

    void post(std::function<void()> fn)
    {
        {
            std::lock_guard<std::mutex> lock(mtx_);
            tasks_.push_back(std::move(fn));
        }
        cv_.notify_one();
    }

private:
    MicrotaskQueue()
    {
        worker_ = std::thread([this]
                              {
            for (;;) {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(mtx_);
                    cv_.wait(lock, [&]{ return stopping_ || !tasks_.empty(); });
                    if (stopping_ && tasks_.empty()) break;
                    task = std::move(tasks_.front());
                    tasks_.pop_front();
                }
                try {
                    task();
                } catch (...) {
                    // Swallow to avoid terminating the microtask thread.
                }
            } });
        std::atexit([]
                    { MicrotaskQueue::instance().stop(); });
    }

    ~MicrotaskQueue()
    {
        stop();
    }

    void stop()
    {
        {
            std::lock_guard<std::mutex> lock(mtx_);
            stopping_ = true;
        }
        cv_.notify_all();
        if (worker_.joinable())
            worker_.join();
    }

    std::thread worker_;
    std::mutex mtx_;
    std::condition_variable cv_;
    std::deque<std::function<void()>> tasks_;
    bool stopping_ = false;
};

// Helper to post a microtask
inline void queueMicrotask(std::function<void()> fn)
{
    MicrotaskQueue::instance().post(std::move(fn));
}

// ---------------------
// Utility types
// ---------------------
struct Unit
{
    // Represent "void" for Promise<Unit>
};

struct AggregateError : public std::exception
{
    std::vector<std::exception_ptr> errors;
    mutable std::string what_cache;

    explicit AggregateError(std::vector<std::exception_ptr> errs)
        : errors(std::move(errs)) {}

    const char *what() const noexcept override
    {
        if (what_cache.empty())
        {
            what_cache = "AggregateError: " + std::to_string(errors.size()) + " rejection(s)";
        }
        return what_cache.c_str();
    }
};

// Type trait to detect Promise<T>
template <typename>
class Promise;

template <typename X>
struct is_promise : std::false_type
{
};

template <typename X>
struct is_promise<Promise<X>> : std::true_type
{
};

template <typename X>
inline constexpr bool is_promise_v = is_promise<X>::value;

template <typename X>
struct promise_unwrap
{
    using type = X;
};

template <typename X>
struct promise_unwrap<Promise<X>>
{
    using type = X;
};

template <typename X>
using promise_unwrap_t = typename promise_unwrap<X>::type;

// ---------------------
// Promise<T>
// ---------------------
template <typename T>
class Promise
{
public:
    // Settlement struct for allSettled
    struct Settlement
    {
        bool fulfilled;
        std::optional<T> value;
        std::exception_ptr reason;
    };

    // State
    struct State
    {
        std::mutex mtx;
        bool settled = false;
        bool fulfilled = false;
        std::optional<T> value;
        std::exception_ptr reason;
        std::vector<std::function<void()>> continuations;
    };

    Promise()
        : state_(std::make_shared<State>()) {}

    // Executor: runs immediately, like JS. Provide resolve and reject.
    // resolve can be called with T or Promise<T>.
    template <typename Executor>
    explicit Promise(Executor &&exec)
        : Promise()
    {
        auto w = std::weak_ptr<State>(state_);
        auto resolve = [w, this](auto &&x) mutable
        {
            using X = std::decay_t<decltype(x)>;
            auto s = w.lock();
            if (!s)
                return;
            if constexpr (is_promise_v<X>)
            {
                if (x.state_.get() == s.get())
                {
                    // Self-resolution -> reject
                    reject(std::make_exception_ptr(std::runtime_error("TypeError: self resolution")));
                    return;
                }
                // Assimilate
                x.then([this](const T &v)
                       { this->resolve(v); })
                    .catchError([this](std::exception_ptr e)
                                { this->reject(e); });
            }
            else
            {
                this->resolve(std::forward<X>(x));
            }
        };

        auto reject = [this](std::exception_ptr e)
        {
            this->reject(e);
        };

        try
        {
            exec(resolve, reject);
        }
        catch (...)
        {
            reject(std::current_exception());
        }
    }

    // then: onFulfilled returns U or Promise<U>
    template <typename OnFulfilled>
    auto then(OnFulfilled &&onFulfilled) const
    {
        using RawR = decltype(std::declval<OnFulfilled>()(std::declval<T>()));
        using U = promise_unwrap_t<RawR>;
        Promise<U> next;

        auto s = state_;
        auto cont = [s, onFulfilled = std::forward<OnFulfilled>(onFulfilled), next]() mutable
        {
            if (s->fulfilled)
            {
                try
                {
                    if constexpr (is_promise_v<RawR>)
                    {
                        RawR r = onFulfilled(*(s->value));
                        // adopt
                        r.then([next](const U &v)
                               { next.resolve(v); })
                            .catchError([next](std::exception_ptr e)
                                        { next.reject(e); });
                    }
                    else
                    {
                        U out = static_cast<U>(onFulfilled(*(s->value)));
                        next.resolve(std::move(out));
                    }
                }
                catch (...)
                {
                    next.reject(std::current_exception());
                }
            }
            else
            {
                // propagate rejection
                next.reject(s->reason);
            }
        };

        enqueueContinuation(cont);
        return next;
    }

    // then with onRejected to intercept rejections; both must resolve to same U type
    template <typename OnFulfilled, typename OnRejected>
    auto then(OnFulfilled &&onFulfilled, OnRejected &&onRejected) const
    {
        using RawRFulfill = decltype(std::declval<OnFulfilled>()(std::declval<T>()));
        using UF = promise_unwrap_t<RawRFulfill>;

        using RawRReject = decltype(std::declval<OnRejected>()(std::declval<std::exception_ptr>()));
        using UR = promise_unwrap_t<RawRReject>;

        static_assert(std::is_convertible<UF, UR>::value || std::is_convertible<UR, UF>::value,
                      "onFulfilled and onRejected must resolve to the same type (or convertible).");

        using U = std::conditional_t<std::is_convertible<UF, UR>::value, UR, UF>;

        Promise<U> next;
        auto s = state_;
        auto cont = [s,
                     onFulfilled = std::forward<OnFulfilled>(onFulfilled),
                     onRejected = std::forward<OnRejected>(onRejected),
                     next]() mutable
        {
            if (s->fulfilled)
            {
                try
                {
                    if constexpr (is_promise_v<RawRFulfill>)
                    {
                        RawRFulfill r = onFulfilled(*(s->value));
                        r.then([next](const U &v)
                               { next.resolve(v); })
                            .catchError([next](std::exception_ptr e)
                                        { next.reject(e); });
                    }
                    else
                    {
                        U out = static_cast<U>(onFulfilled(*(s->value)));
                        next.resolve(std::move(out));
                    }
                }
                catch (...)
                {
                    next.reject(std::current_exception());
                }
            }
            else
            {
                try
                {
                    if constexpr (is_promise_v<RawRReject>)
                    {
                        RawRReject r = onRejected(s->reason);
                        r.then([next](const U &v)
                               { next.resolve(v); })
                            .catchError([next](std::exception_ptr e)
                                        { next.reject(e); });
                    }
                    else
                    {
                        U out = static_cast<U>(onRejected(s->reason));
                        next.resolve(std::move(out));
                    }
                }
                catch (...)
                {
                    next.reject(std::current_exception());
                }
            }
        };

        enqueueContinuation(cont);
        return next;
    }

    // catch(onRejected)
    template <typename OnRejected>
    auto catchError(OnRejected &&onRejected) const
    {
        // Result type is either T (if onRejected returns T), or unwrapped promise of T
        using RawR = decltype(std::declval<OnRejected>()(std::declval<std::exception_ptr>()));
        using U = promise_unwrap_t<RawR>;

        // If handler resolves to something not T, require it to be convertible to T
        static_assert(std::is_convertible<U, T>::value || std::is_same<U, T>::value,
                      "catch handler must resolve to T (or Promise<T>)");

        Promise<T> next;
        auto s = state_;
        auto cont = [s, onRejected = std::forward<OnRejected>(onRejected), next]() mutable
        {
            if (s->fulfilled)
            {
                // pass through
                next.resolve(*(s->value));
            }
            else
            {
                try
                {
                    if constexpr (is_promise_v<RawR>)
                    {
                        RawR r = onRejected(s->reason);
                        r.then([next](const T &v)
                               { next.resolve(v); })
                            .catchError([next](std::exception_ptr e)
                                        { next.reject(e); });
                    }
                    else
                    {
                        T out = static_cast<T>(onRejected(s->reason));
                        next.resolve(std::move(out));
                    }
                }
                catch (...)
                {
                    next.reject(std::current_exception());
                }
            }
        };

        enqueueContinuation(cont);
        return next;
    }

    // finally: runs callback; passes through original value unless callback throws/rejects
    template <typename FinallyFn>
    auto finally(FinallyFn &&fn) const
    {
        Promise<T> next;
        auto s = state_;
        auto cont = [s, fn = std::forward<FinallyFn>(fn), next]() mutable
        {
            auto run_finally = [&]() -> std::optional<std::exception_ptr>
            {
                try
                {
                    using Ret = decltype(fn());
                    if constexpr (is_promise_v<Ret>)
                    {
                        // Wait asynchronously via chaining into next path below
                        // We'll run it by creating a small helper promise chain:
                        Ret p = fn();
                        // We need to continue only after p settles; so we chain
                        bool done = false;
                        std::mutex m;
                        std::condition_variable cv;
                        std::exception_ptr e;
                        p.then([&](const auto &)
                               {
                            std::lock_guard<std::mutex> lk(m);
                            done = true;
                            cv.notify_one(); })
                            .catchError([&](std::exception_ptr ex)
                                        {
                                            std::lock_guard<std::mutex> lk(m);
                                            e = ex;
                                            done = true;
                                            cv.notify_one();
                                            return Unit{}; // For type satisfaction if needed
                                        });
                        // Block? We must not block microtask thread. So instead handle async:
                        // We can't block. We'll restructure: do not wait here. Instead, adopt:
                        // Rebuild: We'll not execute pass-through here; we chain p asynchronously.
                        // >> Adjust approach: call fn first, then in its then/catch, continue.
                    }
                    else
                    {
                        fn();
                    }
                    return std::nullopt;
                }
                catch (...)
                {
                    return std::make_optional(std::current_exception());
                }
            };

            // We must handle the asynchronous finally correctly. Implement like JS:
            // value path:
            auto do_pass_through = [s, next]() mutable
            {
                if (s->fulfilled)
                {
                    next.resolve(*(s->value));
                }
                else
                {
                    next.reject(s->reason);
                }
            };

            // Execute fn; if it returns a Promise, we chain it and then pass-through.
            using Ret = decltype(fn());
            if constexpr (is_promise_v<Ret>)
            {
                try
                {
                    Ret p = fn();
                    p.then([do_pass_through](const auto &)
                           { do_pass_through(); })
                        .catchError([next](std::exception_ptr e)
                                    { next.reject(e); });
                }
                catch (...)
                {
                    next.reject(std::current_exception());
                }
            }
            else
            {
                auto ex = std::optional<std::exception_ptr>();
                try
                {
                    fn();
                }
                catch (...)
                {
                    ex = std::current_exception();
                }

                if (ex)
                {
                    next.reject(*ex);
                }
                else
                {
                    do_pass_through();
                }
            }
        };

        enqueueContinuation(cont);
        return next;
    }

    // Static helpers
    static Promise<T> resolve(const T &value)
    {
        Promise<T> p;
        p.resolve(value);
        return p;
    }
    static Promise<T> resolve(T &&value)
    {
        Promise<T> p;
        p.resolve(std::move(value));
        return p;
    }
    static Promise<T> resolve(const Promise<T> &other)
    {
        Promise<T> p;
        other.then([p](const T &v)
                   { p.resolve(v); })
            .catchError([p](std::exception_ptr e)
                        { p.reject(e); });
        return p;
    }
    static Promise<T> reject(std::exception_ptr e)
    {
        Promise<T> p;
        p.reject(e);
        return p;
    }
    static Promise<T> reject(const std::string &msg)
    {
        return reject(std::make_exception_ptr(std::runtime_error(msg)));
    }

    // Promise.all
    static Promise<std::vector<T>> all(const std::vector<Promise<T>> &arr)
    {
        Promise<std::vector<T>> out;
        if (arr.empty())
        {
            out.resolve({});
            return out;
        }
        auto results = std::make_shared<std::vector<std::optional<T>>>(arr.size());
        auto remaining = std::make_shared<std::atomic<size_t>>(arr.size());
        auto rejected = std::make_shared<std::atomic<bool>>(false);

        for (size_t i = 0; i < arr.size(); ++i)
        {
            arr[i].then([i, results, remaining, rejected, out](const T &v)
                        {
                if (rejected->load()) return;
                (*results)[i] = v;
                if (remaining->fetch_sub(1) == 1) {
                    // Build final vector
                    std::vector<T> final;
                    final.reserve(results->size());
                    for (auto& o : *results) final.push_back(std::move(*o));
                    out.resolve(std::move(final));
                } })
                .catchError([rejected, out](std::exception_ptr e)
                            {
                                bool expected = false;
                                if (rejected->compare_exchange_strong(expected, true))
                                {
                                    out.reject(e);
                                }
                                return T{}; // not used; just to satisfy template in some compilers
                            });
        }
        return out;
    }

    // Promise.allSettled
    static Promise<std::vector<Settlement>> allSettled(const std::vector<Promise<T>> &arr)
    {
        Promise<std::vector<Settlement>> out;
        if (arr.empty())
        {
            out.resolve({});
            return out;
        }
        auto settlements = std::make_shared<std::vector<Settlement>>(arr.size());
        auto remaining = std::make_shared<std::atomic<size_t>>(arr.size());

        for (size_t i = 0; i < arr.size(); ++i)
        {
            arr[i].then([i, settlements, remaining, out](const T &v)
                        {
                (*settlements)[i] = Settlement{true, std::make_optional<T>(v), nullptr};
                if (remaining->fetch_sub(1) == 1) out.resolve(*settlements); })
                .catchError([i, settlements, remaining, out](std::exception_ptr e)
                            {
                (*settlements)[i] = Settlement{false, std::nullopt, e};
                if (remaining->fetch_sub(1) == 1) out.resolve(*settlements);
                return T{}; });
        }
        return out;
    }

    // Promise.race
    static Promise<T> race(const std::vector<Promise<T>> &arr)
    {
        Promise<T> out;
        if (arr.empty())
        {
            out.reject(std::make_exception_ptr(std::runtime_error("race requires non-empty array")));
            return out;
        }
        auto done = std::make_shared<std::atomic<bool>>(false);
        for (auto &p : arr)
        {
            p.then([done, out](const T &v)
                   {
                bool expected = false;
                if (done->compare_exchange_strong(expected, true)) {
                    out.resolve(v);
                } })
                .catchError([done, out](std::exception_ptr e)
                            {
                bool expected = false;
                if (done->compare_exchange_strong(expected, true)) {
                    out.reject(e);
                }
                return T{}; });
        }
        return out;
    }

    // Promise.any
    static Promise<T> any(const std::vector<Promise<T>> &arr)
    {
        Promise<T> out;
        if (arr.empty())
        {
            out.reject(std::make_exception_ptr(AggregateError({})));
            return out;
        }
        auto remaining = std::make_shared<std::atomic<size_t>>(arr.size());
        auto errors = std::make_shared<std::vector<std::exception_ptr>>();
        errors->resize(arr.size());
        auto done = std::make_shared<std::atomic<bool>>(false);

        for (size_t i = 0; i < arr.size(); ++i)
        {
            arr[i].then([done, out](const T &v)
                        {
                bool expected = false;
                if (done->compare_exchange_strong(expected, true)) {
                    out.resolve(v);
                } })
                .catchError([i, errors, remaining, done, out](std::exception_ptr e)
                            {
                (*errors)[i] = e;
                if (remaining->fetch_sub(1) == 1) {
                    if (!done->load()) {
                        out.reject(std::make_exception_ptr(AggregateError(*errors)));
                    }
                }
                return T{}; });
        }
        return out;
    }

    // Introspection helpers (optional)
    bool isSettled() const
    {
        std::lock_guard<std::mutex> lock(state_->mtx);
        return state_->settled;
    }
    bool isFulfilled() const
    {
        std::lock_guard<std::mutex> lock(state_->mtx);
        return state_->settled && state_->fulfilled;
    }

private:
    // Internal resolve/reject
    void resolve(const T &v) const
    {
        settleFulfilled(v);
    }
    void resolve(T &&v) const
    {
        settleFulfilled(std::move(v));
    }
    void reject(std::exception_ptr e) const
    {
        settleRejected(e);
    }

    void settleFulfilled(const T &v) const
    {
        std::vector<std::function<void()>> toRun;
        {
            std::lock_guard<std::mutex> lock(state_->mtx);
            if (state_->settled)
                return;
            state_->settled = true;
            state_->fulfilled = true;
            state_->value = v;
            toRun.swap(state_->continuations);
        }
        for (auto &f : toRun)
            queueMicrotask(f);
    }

    void settleFulfilled(T &&v) const
    {
        std::vector<std::function<void()>> toRun;
        {
            std::lock_guard<std::mutex> lock(state_->mtx);
            if (state_->settled)
                return;
            state_->settled = true;
            state_->fulfilled = true;
            state_->value = std::move(v);
            toRun.swap(state_->continuations);
        }
        for (auto &f : toRun)
            queueMicrotask(f);
    }

    void settleRejected(std::exception_ptr e) const
    {
        std::vector<std::function<void()>> toRun;
        {
            std::lock_guard<std::mutex> lock(state_->mtx);
            if (state_->settled)
                return;
            state_->settled = true;
            state_->fulfilled = false;
            state_->reason = e;
            toRun.swap(state_->continuations);
        }
        for (auto &f : toRun)
            queueMicrotask(f);
    }

    void enqueueContinuation(std::function<void()> cont) const
    {
        bool shouldQueueImmed = false;
        {
            std::lock_guard<std::mutex> lock(state_->mtx);
            if (!state_->settled)
            {
                state_->continuations.push_back(std::move(cont));
                return;
            }
            else
            {
                shouldQueueImmed = true;
            }
        }
        if (shouldQueueImmed)
            queueMicrotask(std::move(cont));
    }

    std::shared_ptr<State> state_;

    template <typename U>
    friend class Promise;
};

// ---------------------
// Example usage
// ---------------------
// int main() {
//     using P = Promise<int>;
//
//     P p([](auto resolve, auto reject) {
//         // Simulate async work
//         queueMicrotask([resolve]{
//             resolve(42);
//         });
//     });
//
//     p.then([](int v){
//         std::cout << "Got: " << v << "\n";
//         return v + 1;
//     })
//     .finally([]{
//         std::cout << "Finally runs\n";
//     })
//     .then([](int v){
//         std::cout << "Then after finally: " << v << "\n";
//         return v * 2;
//     })
//     .catchError([](std::exception_ptr e){
//         try { if (e) std::rethrow_exception(e); }
//         catch (const std::exception& ex) { std::cout << "Caught: " << ex.what() << "\n"; }
//         return 0;
//     })
//     .then([](int v){
//         std::cout << "End: " << v << "\n";
//         return v;
//     });
//
//     // Demonstrate Promise.all
//     auto a = P::resolve(1);
//     auto b = P::resolve(2);
//     auto c = P([](auto res, auto){ res(3); });
//     P::all({a,b,c}).then([](const std::vector<int>& vals){
//         std::cout << "all size: " << vals.size() << "\n";
//         return 0;
//     });
//
//     // Keep process alive briefly to let microtasks run (for demo only).
//     std::this_thread::sleep_for(std::chrono::milliseconds(100));
// }
