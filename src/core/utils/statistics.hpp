/**
 * @file statistics.hpp
 * @brief Statistical analysis utilities
 */

#ifndef STATISTICS_HPP
#define STATISTICS_HPP

#include <vector>
#include <algorithm>
#include <cmath>
#include <numeric>

namespace memory_engine {

struct BenchmarkResult {
    double mean = 0;
    double median = 0;
    double std_dev = 0;
    double min = 0;
    double max = 0;
    double p95 = 0;
    double p99 = 0;
    size_t sample_count = 0;
};

class Statistics {
public:
    static BenchmarkResult analyze(std::vector<double>& samples) {
        BenchmarkResult result;
        if (samples.empty()) return result;

        result.sample_count = samples.size();
        std::sort(samples.begin(), samples.end());

        result.min = samples.front();
        result.max = samples.back();
        result.mean = std::accumulate(samples.begin(), samples.end(), 0.0) / samples.size();
        
        size_t mid = samples.size() / 2;
        result.median = (samples.size() % 2 == 0) 
            ? (samples[mid - 1] + samples[mid]) / 2.0 
            : samples[mid];

        double variance = 0;
        for (double s : samples) {
            variance += (s - result.mean) * (s - result.mean);
        }
        result.std_dev = std::sqrt(variance / samples.size());

        result.p95 = samples[static_cast<size_t>(samples.size() * 0.95)];
        result.p99 = samples[static_cast<size_t>(samples.size() * 0.99)];

        return result;
    }

    static double throughput(size_t operations, double time_ns) {
        if (time_ns <= 0) return 0;
        return (operations * 1e9) / time_ns;
    }
};

} // namespace memory_engine

#endif // STATISTICS_HPP
