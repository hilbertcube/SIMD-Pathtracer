#pragma once
#include <string>
#include <chrono>
#include <iostream>

// Nested namespace (C++17)
namespace rt::benchmark
{

// The reusable RAII Timer class
class Timer
{
private:
    std::string process_name;
    std::chrono::steady_clock::time_point start_time;
    std::ostream& out;

    // Unit toggles
    bool enableNano    = false;
    bool enableMicro   = false;
    bool enableMilli   = false;
    bool enableSeconds = false;
    bool enableMinutes = false;
    bool enableHours   = false;

public:
    explicit Timer(const std::string& name, std::ostream& out = std::clog)
        : process_name(name), out(out)
    {
        start_time = std::chrono::steady_clock::now();
    }

    /*
     * Enable/disable methods
     * Set as type Timer& to return reference to the current object
     * Usage: t.showMilli().showSeconds().showMinutes();
     */
    Timer& showNano(bool v = true)
    {
        enableNano = v;
        return *this;
    }

    Timer& showMicro(bool v = true)
    {
        enableMicro = v;
        return *this;
    }

    Timer& showMilli(bool v = true)
    {
        enableMilli = v;
        return *this;
    }

    Timer& showSeconds(bool v = true)
    {
        enableSeconds = v;
        return *this;
    }

    Timer& showMinutes(bool v = true)
    {
        enableMinutes = v;
        return *this;
    }

    Timer& showHours(bool v = true)
    {
        enableHours = v;
        return *this;
    }

    ~Timer()
    {
        auto end_time = std::chrono::steady_clock::now();
        auto ns =
            std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count();

        out << process_name << " took ";

        bool first = true;
        auto sep   = [&](std::string s)
        {
            if (!first)
            {
                out << ", ";
            }
            out << s;
            first = false;
        };

        if (enableNano)
        {
            sep(std::to_string(ns) + " ns");
        }
        if (enableMicro)
        {
            sep(std::to_string(ns / 1'000.0) + " us");
        }
        if (enableMilli)
        {
            sep(std::to_string(ns / 1'000'000.0) + " ms");
        }
        if (enableSeconds)
        {
            sep(std::to_string(ns / 1e9) + " s");
        }
        if (enableMinutes)
        {
            sep(std::to_string(ns / (60.0 * 1e9)) + " min");
        }
        if (enableHours)
        {
            sep(std::to_string(ns / (3600.0 * 1e9)) + " h");
        }

        out << std::endl;
    }
};

} // namespace rt::benchmark
