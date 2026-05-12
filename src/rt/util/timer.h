#pragma once

#include <chrono>
#include <cstdio>
#include <iostream>
#include <string>
#include <utility>

namespace rt
{

/// Measures elapsed wall-clock time on a monotonic clock (unaffected by system clock changes).
class Stopwatch
{
public:
    using clock = std::chrono::steady_clock;

    Stopwatch() : start_time(clock::now()) {}

    /// Starts measuring again from now.
    void restart() { start_time = clock::now(); }

    /// Time since construction or the last restart().
    [[nodiscard]]
    std::chrono::nanoseconds elapsed() const
    {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(clock::now() - start_time);
    }

    [[nodiscard]]
    double elapsed_seconds() const
    {
        return std::chrono::duration<double>(elapsed()).count();
    }

private:
    clock::time_point start_time;
};

/*
 * Formats a duration for people, in the largest unit that keeps the number readable:
 *   850 ns, 12.4 us, 445.21 ms, 2.45 s, 2 min 27.3 s, 1 h 02 min 03 s
 * Values are rounded to the shown precision before the unit is picked, so 999.996 ms reads
 * "1.00 s", not "1000.00 ms".
 */
inline std::string format_duration(std::chrono::nanoseconds duration)
{
    char text[64];
    const long long nanoseconds = duration.count();

    if (nanoseconds < 1'000)
    {
        std::snprintf(text, sizeof(text), "%lld ns", nanoseconds);
        return text;
    }

    // Count of `step`-nanosecond units, rounded to nearest
    auto rounded = [&](long long step)
    {
        return (nanoseconds + step / 2) / step;
    };

    if (long long tenths_of_microseconds = rounded(100); tenths_of_microseconds < 10'000)
    {
        std::snprintf(text, sizeof(text), "%.1f us", tenths_of_microseconds / 10.0);
    }
    else if (long long hundredths_of_milliseconds = rounded(10'000);
             hundredths_of_milliseconds < 100'000)
    {
        std::snprintf(text, sizeof(text), "%.2f ms", hundredths_of_milliseconds / 100.0);
    }
    else if (long long hundredths_of_seconds = rounded(10'000'000); hundredths_of_seconds < 6'000)
    {
        std::snprintf(text, sizeof(text), "%.2f s", hundredths_of_seconds / 100.0);
    }
    else if (long long tenths_of_seconds = rounded(100'000'000); tenths_of_seconds < 36'000)
    {
        std::snprintf(text, sizeof(text), "%lld min %.1f s", tenths_of_seconds / 600,
                      (tenths_of_seconds % 600) / 10.0);
    }
    else
    {
        long long total_seconds = rounded(1'000'000'000);
        std::snprintf(text, sizeof(text), "%lld h %02lld min %02lld s", total_seconds / 3600,
                      total_seconds / 60 % 60, total_seconds % 60);
    }

    return text;
}

/*
 * Formats a duration as a clock reading, for progress displays where the width shouldn't
 * jump around: 0:07, 2:27, 1:02:03. Rounded to whole seconds.
 */
inline std::string format_clock(std::chrono::nanoseconds duration)
{
    char text[32];
    long long total_seconds = (duration.count() + 500'000'000) / 1'000'000'000;
    if (total_seconds < 3600)
    {
        std::snprintf(text, sizeof(text), "%lld:%02lld", total_seconds / 60, total_seconds % 60);
    }
    else
    {
        std::snprintf(text, sizeof(text), "%lld:%02lld:%02lld", total_seconds / 3600,
                      total_seconds / 60 % 60, total_seconds % 60);
    }
    return text;
}

/*
 * Reports how long its scope took when it ends (RAII):
 *
 *     {
 *         rt::ScopedTimer timer("Total");
 *         render();
 *     } // prints "Total: 2 min 27.3 s"
 */
class ScopedTimer
{
public:
    explicit ScopedTimer(std::string label, std::ostream& output = std::clog)
        : label(std::move(label)), output(output)
    {
    }

    // A copy would report the same scope twice
    ScopedTimer(const ScopedTimer&)            = delete;
    ScopedTimer& operator=(const ScopedTimer&) = delete;

    ~ScopedTimer()
    {
        // Destructors must not throw; losing one log line is better than terminating
        try
        {
            output << label << ": " << format_duration(stopwatch.elapsed()) << std::endl;
        }
        catch (...)
        {
        }
    }

    /// Time since the timer was created.
    [[nodiscard]]
    std::chrono::nanoseconds elapsed() const
    {
        return stopwatch.elapsed();
    }

private:
    std::string label;
    std::ostream& output;
    Stopwatch stopwatch; // declared last so it starts after the other members are set up
};

} // namespace rt
