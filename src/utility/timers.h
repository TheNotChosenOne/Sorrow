#pragma once

#include <chrono>
#include <vector>
#include <functional>

// Average duration of thing, number that could be done per second
class DurationTimer {
    private:
        std::vector< double > timings;
        uint8_t timingIndex;

    public:
        DurationTimer();
        void tick(const std::chrono::duration< double > seconds);
        double average() const;
        double perSecond() const;
};

// Use to run an event every n seconds, when tick returns true
// Can scale time so that the event occurs more or less
//      frequently
class ActionTimer {
    private:
        std::chrono::duration< double > timeLeft;
        const double duration;
        double scale;

    public:
        ActionTimer(double seconds);
        bool tick(const std::chrono::duration< double > seconds);
        // Real seconds to wait before next event
        double estimate() const;
        void setTimeScale(double scale);
};

// Use to accumulate an overall runtime
class AccumulateTimer {
    private:
        std::chrono::duration< double > accum;

    public:
        AccumulateTimer();
        void add(const std::chrono::duration< double > seconds);
        // Times the given function and adds the duration
        std::chrono::duration< double > add(const std::function< void() > &work);
        double empty();
};
