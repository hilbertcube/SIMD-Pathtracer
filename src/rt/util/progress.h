#pragma once

#include <atomic>
#include <chrono>
#include <cstdio>
#include <iostream>
#include <mutex>
#include <string>

#include <unistd.h> // isatty

#include "rt/util/timer.h"

namespace rt
{

inline bool stderr_is_terminal()
{
    return isatty(fileno(stderr)) != 0;
}

/*
 * Progress display for a fixed number of work items, safe to advance from many threads:
 *
 *     [##################------------]  60.0%  elapsed 0:02  remaining 0:01
 *
 * On a terminal the bar is redrawn in place; for a file or pipe it logs a line every 10%
 * instead. Reporting never slows the work: the count is atomic, redraws are rate-limited,
 * and a worker skips drawing rather than wait for another thread that is.
 */
class ProgressBar
{
public:
    explicit ProgressBar(int total_items, std::ostream& output = std::clog,
                         bool interactive = stderr_is_terminal())
        : total_items(total_items > 0 ? total_items : 1), output(output), interactive(interactive)
    {
        if (interactive)
        {
            draw(0);
        }
    }

    ProgressBar(const ProgressBar&)            = delete;
    ProgressBar& operator=(const ProgressBar&) = delete;

    void advance()
    {
        const int done = items_done.fetch_add(1, std::memory_order_relaxed) + 1;

        if (interactive)
        {
            const long long now = stopwatch.elapsed().count();
            if (now - last_draw_time.load(std::memory_order_relaxed) < REDRAW_INTERVAL.count())
            {
                return;
            }
            std::unique_lock<std::mutex> lock(mutex, std::try_to_lock);
            if (!lock.owns_lock())
            {
                return; // another thread is drawing; skipping one frame is fine
            }
            last_draw_time.store(now, std::memory_order_relaxed);
            draw(done);
            return;
        }

        // Log mode: lock only when crossing a 10% step (ten times in total), so lines stay
        // in order
        const int tenth = done * 10 / total_items;
        if (tenth <= last_reported_tenth.load(std::memory_order_relaxed))
        {
            return;
        }
        std::lock_guard<std::mutex> lock(mutex);
        if (tenth > last_reported_tenth.load(std::memory_order_relaxed))
        {
            last_reported_tenth.store(tenth, std::memory_order_relaxed);
            output << "  " << tenth * 10 << "% (" << format_clock(stopwatch.elapsed())
                   << " elapsed)\n";
        }
    }

    /// Erases the bar; call once all items are done.
    void finish()
    {
        std::lock_guard<std::mutex> lock(mutex);
        if (interactive)
        {
            output << "\r\x1b[2K" << std::flush; // carriage return, then erase the line
        }
    }

    [[nodiscard]]
    std::chrono::nanoseconds elapsed() const
    {
        return stopwatch.elapsed();
    }

private:
    static constexpr int BAR_WIDTH = 30;
    static constexpr std::chrono::nanoseconds REDRAW_INTERVAL{100'000'000}; // 10 frames/s

    const int total_items;
    std::ostream& output;
    const bool interactive;

    std::mutex mutex; // serializes output, never the work
    std::atomic<int> items_done{0};
    std::atomic<int> last_reported_tenth{0};
    std::atomic<long long> last_draw_time{0}; // nanoseconds since start
    Stopwatch stopwatch;

    void draw(int done)
    {
        double fraction = static_cast<double>(done) / total_items;
        int filled      = static_cast<int>(fraction * BAR_WIDTH);

        std::string bar(BAR_WIDTH, '-');
        bar.replace(0, filled, filled, '#');

        // Remaining time extrapolated from the average so far; unknown until work is done
        auto elapsed          = stopwatch.elapsed();
        std::string remaining = "-:--";
        if (done > 0)
        {
            remaining = format_clock(elapsed * (total_items - done) / done);
        }

        char percent[16];
        std::snprintf(percent, sizeof(percent), "%5.1f%%", 100.0 * fraction);
        output << "\r[" << bar << "] " << percent << "  elapsed " << format_clock(elapsed)
               << "  remaining " << remaining << std::flush;
    }
};

} // namespace rt
