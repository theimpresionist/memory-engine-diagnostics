/**
 * @file concurrency_benchmark.hpp
 * @brief Threading and concurrency benchmarks
 */

#ifndef CONCURRENCY_BENCHMARK_HPP
#define CONCURRENCY_BENCHMARK_HPP

#include "../utils/timer.hpp"
#include "../utils/statistics.hpp"
#include <atomic>
#include <mutex>
#include <thread>
#include <vector>
#include <functional>
#include <condition_variable>
#include <queue>

namespace memory_engine {

struct ConcurrencyConfig {
    size_t thread_count = 4;
    size_t iterations = 1000;
    size_t work_size = 100;
};

struct ConcurrencyMetrics {
    double total_time_ms = 0;
    double contention_time_ms = 0;
    double throughput = 0;
    double thread_efficiency = 0;
    std::string test_name;
};

class ConcurrencyBenchmark {
public:
    // Mutex contention test
    ConcurrencyMetrics run_mutex_contention(const ConcurrencyConfig& config) {
        ConcurrencyMetrics metrics;
        metrics.test_name = "Mutex Contention";

        std::mutex mtx;
        std::atomic<size_t> counter{0};
        std::atomic<double> wait_time{0};

        Timer total_timer;
        total_timer.start();

        std::vector<std::thread> threads;
        for (size_t t = 0; t < config.thread_count; ++t) {
            threads.emplace_back([&]() {
                for (size_t i = 0; i < config.iterations; ++i) {
                    Timer wait_timer;
                    wait_timer.start();
                    
                    std::lock_guard<std::mutex> lock(mtx);
                    
                    wait_timer.stop();
                    wait_time.fetch_add(wait_timer.elapsed_ns());

                    // Simulate work
                    volatile size_t work = 0;
                    for (size_t w = 0; w < config.work_size; ++w) {
                        work += w;
                    }
                    counter++;
                }
            });
        }

        for (auto& t : threads) t.join();

        total_timer.stop();
        metrics.total_time_ms = total_timer.elapsed_ms();
        metrics.contention_time_ms = wait_time.load() / 1000000.0;
        metrics.throughput = Statistics::throughput(counter.load(), total_timer.elapsed_ns());
        metrics.thread_efficiency = (config.iterations * config.thread_count) / 
            (metrics.total_time_ms * config.thread_count);

        return metrics;
    }

    // Atomic operations test
    ConcurrencyMetrics run_atomic_performance(const ConcurrencyConfig& config) {
        ConcurrencyMetrics metrics;
        metrics.test_name = "Atomic Performance";

        std::atomic<size_t> counter{0};

        Timer total_timer;
        total_timer.start();

        std::vector<std::thread> threads;
        for (size_t t = 0; t < config.thread_count; ++t) {
            threads.emplace_back([&]() {
                for (size_t i = 0; i < config.iterations; ++i) {
                    counter.fetch_add(1, std::memory_order_relaxed);
                    
                    // Simulate additional atomic ops
                    for (size_t w = 0; w < config.work_size / 10; ++w) {
                        counter.fetch_add(1, std::memory_order_seq_cst);
                    }
                }
            });
        }

        for (auto& t : threads) t.join();

        total_timer.stop();
        metrics.total_time_ms = total_timer.elapsed_ms();
        metrics.contention_time_ms = 0;
        metrics.throughput = Statistics::throughput(counter.load(), total_timer.elapsed_ns());

        return metrics;
    }

    // Producer-Consumer test
    ConcurrencyMetrics run_producer_consumer(const ConcurrencyConfig& config) {
        ConcurrencyMetrics metrics;
        metrics.test_name = "Producer-Consumer";

        std::queue<int> queue;
        std::mutex queue_mutex;
        std::condition_variable cv;
        std::atomic<bool> done{false};
        std::atomic<size_t> items_processed{0};

        Timer total_timer;
        total_timer.start();

        // Producers
        std::vector<std::thread> producers;
        for (size_t t = 0; t < config.thread_count / 2; ++t) {
            producers.emplace_back([&]() {
                for (size_t i = 0; i < config.iterations; ++i) {
                    std::lock_guard<std::mutex> lock(queue_mutex);
                    queue.push(static_cast<int>(i));
                    cv.notify_one();
                }
            });
        }

        // Consumers
        std::vector<std::thread> consumers;
        for (size_t t = 0; t < config.thread_count / 2; ++t) {
            consumers.emplace_back([&]() {
                while (!done.load() || !queue.empty()) {
                    std::unique_lock<std::mutex> lock(queue_mutex);
                    if (cv.wait_for(lock, std::chrono::milliseconds(1), 
                        [&]() { return !queue.empty() || done.load(); })) {
                        if (!queue.empty()) {
                            queue.pop();
                            items_processed++;
                        }
                    }
                }
            });
        }

        for (auto& t : producers) t.join();
        done.store(true);
        cv.notify_all();
        for (auto& t : consumers) t.join();

        total_timer.stop();
        metrics.total_time_ms = total_timer.elapsed_ms();
        metrics.throughput = Statistics::throughput(items_processed.load(), total_timer.elapsed_ns());

        return metrics;
    }

    // Thread creation overhead test
    ConcurrencyMetrics run_thread_creation(const ConcurrencyConfig& config) {
        ConcurrencyMetrics metrics;
        metrics.test_name = "Thread Creation";

        Timer total_timer;
        total_timer.start();

        for (size_t i = 0; i < config.iterations; ++i) {
            std::vector<std::thread> threads;
            for (size_t t = 0; t < config.thread_count; ++t) {
                threads.emplace_back([]() {
                    volatile int x = 0;
                    for (int i = 0; i < 100; ++i) x += i;
                });
            }
            for (auto& t : threads) t.join();
        }

        total_timer.stop();
        metrics.total_time_ms = total_timer.elapsed_ms();
        metrics.throughput = (config.iterations * config.thread_count) / (metrics.total_time_ms / 1000.0);

        return metrics;
    }
};

} // namespace memory_engine

#endif // CONCURRENCY_BENCHMARK_HPP
