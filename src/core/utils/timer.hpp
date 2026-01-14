/**
 * @file timer.hpp
 * @brief High-resolution timer for benchmarking
 */

#ifndef TIMER_HPP
#define TIMER_HPP

#include <chrono>

namespace memory_engine {

class Timer {
public:
    using Clock = std::chrono::high_resolution_clock;
    using TimePoint = Clock::time_point;
    using Duration = std::chrono::nanoseconds;

    Timer() : m_running(false), m_elapsed(0) {}

    void start() {
        if (!m_running) {
            m_start = Clock::now();
            m_running = true;
        }
    }

    void stop() {
        if (m_running) {
            auto end = Clock::now();
            m_elapsed += std::chrono::duration_cast<Duration>(end - m_start).count();
            m_running = false;
        }
    }

    void reset() { m_running = false; m_elapsed = 0; }
    void restart() { reset(); start(); }

    double elapsed_ns() const {
        if (m_running) {
            auto now = Clock::now();
            return static_cast<double>(m_elapsed + 
                std::chrono::duration_cast<Duration>(now - m_start).count());
        }
        return static_cast<double>(m_elapsed);
    }

    double elapsed_us() const { return elapsed_ns() / 1000.0; }
    double elapsed_ms() const { return elapsed_ns() / 1000000.0; }
    double elapsed_sec() const { return elapsed_ns() / 1000000000.0; }
    bool is_running() const { return m_running; }

private:
    TimePoint m_start;
    bool m_running;
    int64_t m_elapsed;
};

class ScopedTimer {
public:
    explicit ScopedTimer(double& out_elapsed) : m_out_elapsed(out_elapsed) { m_timer.start(); }
    ~ScopedTimer() { m_timer.stop(); m_out_elapsed = m_timer.elapsed_ns(); }
    ScopedTimer(const ScopedTimer&) = delete;
    ScopedTimer& operator=(const ScopedTimer&) = delete;
private:
    Timer m_timer;
    double& m_out_elapsed;
};

} // namespace memory_engine

#endif // TIMER_HPP
