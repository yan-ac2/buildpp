#include <coroutine>
#include <iostream>
#include <utility>
#include <cstdlib> // For std::abort
#include <type_traits>

template <typename T = void>
struct CoTask {
    struct promise_type;
    using value_type  = std::conditional_t<std::is_same_v<void, T>, void*, T>;
    using handle_type = std::coroutine_handle<promise_type>;

    struct promise_type {
        std::coroutine_handle<> continuation;
        value_type value;

        CoTask get_return_object() noexcept {
            return CoTask{ handle_type::from_promise(*this) };
        }

        std::suspend_always initial_suspend() noexcept { return {}; }

        // --- FINAL SUSPEND (SYMMETRIC TRANSFER) ---
        auto final_suspend() noexcept {
            struct FinalAwaiter {
                bool await_ready() noexcept { return false; }

                std::coroutine_handle<> await_suspend(handle_type h) noexcept {
                    auto parent = h.promise().continuation;
                    return parent ? parent : std::noop_coroutine();
                }

                void await_resume() noexcept {}
            };
            return FinalAwaiter{};
        }

        // 1. co_return target: Store final result
        void return_value(value_type v) noexcept {
            if constexpr (std::is_same_v<T,void>) {
                return;
            } else {
                value = std::move(v);
            }
        }

        // Required by standard compiler interface under -fno-exceptions
        void unhandled_exception() noexcept {
            std::abort(); 
        }

        // 2. co_yield target: Store intermediate result & suspend
        auto yield_value(value_type v) noexcept {
            value = std::move(v);

            struct YieldAwaiter {
                bool await_ready() noexcept { return false; }

                std::coroutine_handle<> await_suspend(handle_type h) noexcept {
                    auto parent = h.promise().continuation;
                    return parent ? parent : std::noop_coroutine();
                }

                void await_resume() noexcept {}
            };
            return YieldAwaiter{};
        }

        T result() noexcept {
            if constexpr (std::is_same_v<T,void>) {
                return;
            } else {
                return std::move(value);
            }
        }
    };

    handle_type handle;

    CoTask(handle_type h) noexcept : handle(h) {}
    ~CoTask() {
        if (handle) handle.destroy();
    }

    CoTask(const CoTask&) = delete;
    CoTask& operator=(const CoTask&) = delete;

    CoTask(CoTask&& o) noexcept : handle(std::exchange(o.handle, nullptr)) {}
    CoTask& operator=(CoTask&& o) noexcept {
        if (this != &o) {
            if (handle) handle.destroy();
            handle = std::exchange(o.handle, nullptr);
        }
        return *this;
    }

    // --- AWAITER FOR co_await ---
    auto operator co_await() noexcept {
        struct TaskAwaiter {
            handle_type child_handle;

            bool await_ready() noexcept {
                return !child_handle || child_handle.done();
            }

            std::coroutine_handle<> await_suspend(std::coroutine_handle<> parent) noexcept {
                child_handle.promise().continuation = parent;
                return child_handle;
            }

            T await_resume() noexcept {
                return child_handle.promise().result();
            }
        };
        return TaskAwaiter{ handle };
    }
};

template <typename Fn>
class Task {
    
};
// -------------------------------------------------------------
// Demonstration: Deeply recursive coroutine (1,000,000 depth)
// -------------------------------------------------------------
CoTask<void> deep_recursion(int count) {
    if (count > 0) {
        co_await deep_recursion(count - 1);
    }
    co_return{};
}
CoTask<int> add(int count) {
    int ret = 0;
    if (count < 10) {
       ret = co_await add(count);
    }
    co_return ret;
}

int main() {
    auto t = deep_recursion(1'000'000); // 1 million frames deep!
    
    // Kick off the root coroutine
    t.handle.resume(); 
    
    std::cout << "Successfully processed 1,000,000 nested coroutines without stack overflow!\n";
    auto t2 = add(1); // 1 million frames deep!
    t2.handle.resume();
    std::cout << "add" << t2.handle.promise().value <<"\n";
    return 0;
}