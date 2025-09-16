#pragma once
#include <vector>
#include <string>
#include <numeric>
#include <chrono>
#include <iostream>

// Nested namespace (C++17)
namespace rt::benchmark {

#define WARNING_RED "\033[38;2;255;15;15m" 
#define GREEN "\033[38;2;118;185;0m" // rgb(118, 185, 0)
#define RESET "\033[0m"

// The reusable RAII Timer class
enum class TimeUnit {
    Nanoseconds,
    Microseconds,
    Milliseconds,
    Seconds,
    Minutes,
    Hours
};

class Timer {
private:
    std::string process_name;
    std::chrono::steady_clock::time_point start_time;
    std::ostream& out;
    bool call_once = false;

    // Unit toggles
    bool enableNano     = false;
    bool enableMicro    = false;
    bool enableMilli    = false;
    bool enableSeconds  = false;
    bool enableMinutes  = false;
    bool enableHours    = false;

public:
    explicit Timer(const std::string& name, std::ostream& out = std::clog)
        : process_name(name), out(out)
    {
        start_time = std::chrono::steady_clock::now();
    }

    // Enable/disable methods
    // Set as type Timer& to return reference to the current object
    // Usage: t.showMilli().showSeconds().showMinutes();
    Timer& showNano(bool v = true)    { enableNano = v; return *this; }
    Timer& showMicro(bool v = true)   { enableMicro = v; return *this; }
    Timer& showMilli(bool v = true)   { enableMilli = v; return *this; }
    Timer& showSeconds(bool v = true) { enableSeconds = v; return *this; }
    Timer& showMinutes(bool v = true) { enableMinutes = v; return *this; }
    Timer& showHours(bool v = true)   { enableHours = v; return *this; }

    ~Timer() {
        auto end_time = std::chrono::steady_clock::now();
        auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count();

        out << process_name << " took ";

        bool first = true;
        auto sep = [&](std::string s) {
            if (!first) out << ", ";
            out << s;
            first = false;
        };

        if (enableNano)    sep(std::to_string(ns) + " ns");
        if (enableMicro)   sep(std::to_string(ns / 1'000.0) + " us");
        if (enableMilli)   sep(std::to_string(ns / 1'000'000.0) + " ms");
        if (enableSeconds) sep(std::to_string(ns / 1e9) + " s");
        if (enableMinutes) sep(std::to_string(ns / (60.0 * 1e9)) + " min");
        if (enableHours)   sep(std::to_string(ns / (3600.0 * 1e9)) + " h");

        out << std::endl;
    }
};

struct BenchmarkResult {
    std::string name;
    std::vector<double> times_ns;
    size_t iterations;
    
    double min() const { 
        return *std::min_element(times_ns.begin(), times_ns.end()); 
    }
    
    double max() const { 
        return *std::max_element(times_ns.begin(), times_ns.end()); 
    }
    
    double mean() const {
        return std::accumulate(times_ns.begin(), times_ns.end(), 0.0) / times_ns.size();
    }
    
    double median() const {
        std::vector<double> sorted = times_ns;
        std::sort(sorted.begin(), sorted.end());
        size_t n = sorted.size();
        if (n % 2 == 0) {
            return (sorted[n/2 - 1] + sorted[n/2]) / 2.0;
        } else {
            return sorted[n/2];
        }
    }
    
    double stddev() const {
        double m = mean();
        double sq_sum = 0.0;
        for (double time : times_ns) {
            sq_sum += (time - m) * (time - m);
        }
        return std::sqrt(sq_sum / times_ns.size());
    }
};

class Benchmark {
private:
    std::string benchmark_name;
    std::ostream& out;
    std::vector<BenchmarkResult> results;
    
    // Unit toggles
    bool enableNano     = false;
    bool enableMicro    = false;
    bool enableMilli    = true;  // Default
    bool enableSeconds  = false;
    bool enableMinutes  = false;
    bool enableHours    = false;
    
    // Statistics toggles
    bool showMin        = true;
    bool showMax        = true;
    bool showMean       = true;
    bool showMedian     = false;
    bool showStdDev     = false;
    
    // Formatting
    int precision = 3;

    double convertTime(double ns, TimeUnit unit) const {
        switch (unit) {
            case TimeUnit::Nanoseconds:  return ns;
            case TimeUnit::Microseconds: return ns / 1'000.0;
            case TimeUnit::Milliseconds: return ns / 1'000'000.0;
            case TimeUnit::Seconds:      return ns / 1e9;
            case TimeUnit::Minutes:      return ns / (60.0 * 1e9);
            case TimeUnit::Hours:        return ns / (3600.0 * 1e9);
            default: return ns;
        }
    }
    
    std::string getUnitString(TimeUnit unit) const {
        switch (unit) {
            case TimeUnit::Nanoseconds:  return "ns";
            case TimeUnit::Microseconds: return "us";
            case TimeUnit::Milliseconds: return "ms";
            case TimeUnit::Seconds:      return "s";
            case TimeUnit::Minutes:      return "min";
            case TimeUnit::Hours:        return "h";
            default: return "ns";
        }
    }
    
    void printTime(double ns_value, const std::string& label) const {
        bool first = true;
        auto printUnit = [&](TimeUnit unit, bool enabled) {
            if (!enabled) return;
            if (!first) out << ", ";
            out << std::fixed << std::setprecision(precision) 
                << convertTime(ns_value, unit) << " " << getUnitString(unit);
            first = false;
        };
        
        out << label << ": ";
        printUnit(TimeUnit::Nanoseconds, enableNano);
        printUnit(TimeUnit::Microseconds, enableMicro);
        printUnit(TimeUnit::Milliseconds, enableMilli);
        printUnit(TimeUnit::Seconds, enableSeconds);
        printUnit(TimeUnit::Minutes, enableMinutes);
        printUnit(TimeUnit::Hours, enableHours);
        out << std::endl;
    }

public:
    explicit Benchmark(const std::string& name, std::ostream& out = std::cout)
        : benchmark_name(name), out(out) {}
    
    // Unit configuration methods (chainable)
    Benchmark& showNano(bool v = true)    { enableNano = v; return *this; }
    Benchmark& showMicro(bool v = true)   { enableMicro = v; return *this; }
    Benchmark& showMilli(bool v = true)   { enableMilli = v; return *this; }
    Benchmark& showSeconds(bool v = true) { enableSeconds = v; return *this; }
    Benchmark& showMinutes(bool v = true) { enableMinutes = v; return *this; }
    Benchmark& showHours(bool v = true)   { enableHours = v; return *this; }
    
    // Statistics configuration methods (chainable)
    Benchmark& showMinimum(bool v = true)   { showMin = v; return *this; }
    Benchmark& showMaximum(bool v = true)   { showMax = v; return *this; }
    Benchmark& showAverage(bool v = true)   { showMean = v; return *this; }
    Benchmark& showMedianTime(bool v = true)  { showMedian = v; return *this; }
    Benchmark& showStandardDev(bool v = true) { showStdDev = v; return *this; }
    
    // Formatting configuration
    Benchmark& setPrecision(int p) { precision = p; return *this; }
    
    // Single execution timing
    template<typename Func>
    double timeFunction(const std::string& name, Func&& func) {
        auto start = std::chrono::steady_clock::now();
        func();
        auto end = std::chrono::steady_clock::now();
        
        auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
        
        out << "[" << benchmark_name << "] " << name << " execution time: ";
        printTime(static_cast<double>(ns), "");
        
        return static_cast<double>(ns);
    }
    
    // Multiple iterations benchmarking
    template<typename Func>
    BenchmarkResult run(const std::string& name, Func&& func, size_t iterations = 100) {
        BenchmarkResult result;
        result.name = name;
        result.iterations = iterations;
        result.times_ns.reserve(iterations);
        
        out << "[" << benchmark_name << "] Running \"" << name 
            << "\" benchmark with " << iterations << " iterations..." << std::endl;
        
        // Warm-up run
        func();
        
        // Actual benchmark runs
        for (size_t i = 0; i < iterations; ++i) {
            auto start = std::chrono::steady_clock::now();
            func();
            auto end = std::chrono::steady_clock::now();
            
            auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
            result.times_ns.push_back(static_cast<double>(ns));
        }
        
        results.push_back(result);
        printResult(result);
        
        return result;
    }
    
    // Benchmark with setup and teardown
    template<typename SetupFunc, typename BenchFunc, typename TeardownFunc>
    BenchmarkResult runWithSetup(const std::string& name, 
                                  SetupFunc&& setup, 
                                  BenchFunc&& benchmark, 
                                  TeardownFunc&& teardown,
                                  size_t iterations = 10) {
        BenchmarkResult result;
        result.name = name;
        result.iterations = iterations;
        result.times_ns.reserve(iterations);
        
        out << "[" << benchmark_name << "] Running \"" << name 
            << "\" benchmark with setup/teardown, " << iterations << " iterations..." << std::endl;
        
        for (size_t i = 0; i < iterations; ++i) {
            setup();
            
            auto start = std::chrono::steady_clock::now();
            benchmark();
            auto end = std::chrono::steady_clock::now();
            
            teardown();
            
            auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
            result.times_ns.push_back(static_cast<double>(ns));
        }
        
        results.push_back(result);
        printResult(result);
        
        return result;
    }
    
    // Print results for a specific benchmark
    void printResult(const BenchmarkResult& result) const {
        out << "Results for \"" << result.name << "\" (" << result.iterations << " runs):" << std::endl;
        
        if (showMin)    printTime(result.min(), "  Min");
        if (showMax)    printTime(result.max(), "  Max");
        if (showMean)   printTime(result.mean(), "  Mean");
        if (showMedian) printTime(result.median(), "  Median");
        if (showStdDev) printTime(result.stddev(), "  Std Dev");
        
        out << std::endl;
    }
    
    // summary of all benchmarks
    void printSummary() const {
        if (results.empty()) {
            out << "[" << benchmark_name << "] No benchmarks have been run." << std::endl;
            return;
        }
        
        out << "\n=== " << benchmark_name << " Summary ===" << std::endl;
        for (const auto& result : results) {
            out << result.name << ": ";
            printTime(result.mean(), "avg");
        }
        out << std::endl;
    }
    
    // Compare two benchmark results
    void compare(const std::string& baseline, const std::string& comparison) const {
        auto baseline_it = std::find_if(results.begin(), results.end(),
            [&baseline](const BenchmarkResult& r) { return r.name == baseline; });
        auto comparison_it = std::find_if(results.begin(), results.end(),
            [&comparison](const BenchmarkResult& r) { return r.name == comparison; });
        
        if (baseline_it == results.end() || comparison_it == results.end()) {
            out << "Cannot compare: one or both benchmarks not found." << std::endl;
            return;
        }
        
        double baseline_mean = baseline_it->mean();
        double comparison_mean = comparison_it->mean();
        double ratio = comparison_mean / baseline_mean;
        
        out << "Comparison: " << comparison << " vs " << baseline << std::endl;
        if (ratio < 1.0) {
            out << "  " << comparison << " is " << std::fixed << std::setprecision(2) 
                << (1.0/ratio) << "x faster" << std::endl;
        } else {
            out << "  " << comparison << " is " << std::fixed << std::setprecision(2) 
                << ratio << "x slower" << std::endl;
        }
    }
    
    // Get all results
    const std::vector<BenchmarkResult>& getResults() const {
        return results;
    }
    
    // Clear all results
    void clear() {
        results.clear();
    }
};


}
