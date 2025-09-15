// thread_pool.hpp
#pragma once
#include <vector>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>

class ThreadPool {
public:
    explicit ThreadPool(size_t num_threads) {
        start(num_threads);
    }

    ~ThreadPool() {
        stop();
    }

    template <class F, class... Args>
    auto enqueue(F&& f, Args&&... args)
        -> std::future<typename std::invoke_result<F, Args...>::type>
    {
        using return_type = typename std::invoke_result<F, Args...>::type;

        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );

        std::future<return_type> res = task->get_future();
        {
            std::unique_lock<std::mutex> lock(m_event_mutex);
            m_tasks.emplace([task]() { (*task)(); });
        }
        m_event_var.notify_one();
        return res;
    }

private:
    std::vector<std::thread> m_threads;
    std::condition_variable m_event_var;
    std::mutex m_event_mutex;
    bool m_stopping = false;

    std::queue<std::function<void()>> m_tasks;

    void start(size_t num_threads) {
        for (size_t i = 0; i < num_threads; ++i) {
            m_threads.emplace_back([=] {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(m_event_mutex);

                        m_event_var.wait(lock, [=] {
                            return m_stopping || !m_tasks.empty();
                        });

                        if (m_stopping && m_tasks.empty())
                            break;

                        task = std::move(m_tasks.front());
                        m_tasks.pop();
                    }
                    task();
                }
            });
        }
    }

    void stop() {
        {
            std::unique_lock<std::mutex> lock(m_event_mutex);
            m_stopping = true;
        }

        m_event_var.notify_all();
        for (auto& thread : m_threads)
            thread.join();
    }
};
