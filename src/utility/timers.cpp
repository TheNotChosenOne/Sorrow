#include "utility/timers.h"

DurationTimer::DurationTimer()
    : timingIndex(0) {
}

void DurationTimer::tick(const std::chrono::duration< double > seconds) {
    if (timings.size() < 256) { timings.push_back(seconds.count()); return; }
    timings[timingIndex++ % 256] = seconds.count();
}

double DurationTimer::average() const {
    double total = 0.0;
    for (const auto x : timings) { total += x; }
    return total / std::max(size_t(1), timings.size());
}

double DurationTimer::perSecond() const {
    return 1.0 / average();
}

ActionTimer::ActionTimer(double seconds)
    : timeLeft(0.0)
    , duration(seconds)
    , scale(1.0) {
}

bool ActionTimer::tick(const std::chrono::duration< double > seconds) {
    timeLeft += scale * seconds;
    if (timeLeft.count() >= duration) {
        timeLeft -= std::chrono::duration< double >(duration);
        return true;
    }
    return false;
}

double ActionTimer::estimate() const {
    return (std::chrono::duration< double >(duration) - timeLeft).count() / scale;
}

void ActionTimer::setTimeScale(double ts) {
    scale = ts;
}

AccumulateTimer::AccumulateTimer()
    : accum(0.0) {
}

void AccumulateTimer::add(const std::chrono::duration< double > seconds) {
    accum += seconds;
}

std::chrono::duration< double > AccumulateTimer::add(const std::function< void() > &work) {
    const auto start = std::chrono::high_resolution_clock::now();
    work();
    const auto stop = std::chrono::high_resolution_clock::now();
    const auto time = stop - start;
    accum += time;
    return time;
}

double AccumulateTimer::empty() {
    const double a = accum.count();
    accum = std::chrono::duration< double >(0);
    return a;
}
