#pragma once
#include <algorithm>
#include <cmath>
#include <stdexcept>

// Offline candidate only. Not linked to the application or any motor command.
namespace visual_reference_candidate {
struct Sample { double position, velocity, acceleration; };
inline Sample segment(double from, double to, double duration, double time) {
    if (!std::isfinite(from) || !std::isfinite(to) || !std::isfinite(duration)
        || !std::isfinite(time) || duration <= 0)
        throw std::invalid_argument("finite endpoints/time and positive duration required");
    const double u = std::clamp(time / duration, 0.0, 1.0);
    const double d = to - from;
    const double s = u*u*u*(10.0 + u*(-15.0 + 6.0*u));
    return {from + d*s, d/duration*30*u*u*(1-u)*(1-u),
            d/(duration*duration)*60*u*(1-u)*(1-2*u)};
}
// Explicit durations are supplied by the caller; no guessed device clock.
inline Sample cycle(double low, double high, double rise, double top,
                    double fall, double bottom, double time) {
    if (!std::isfinite(time) || time < 0 || !std::isfinite(rise)
        || !std::isfinite(top) || !std::isfinite(fall) || !std::isfinite(bottom)
        || rise <= 0 || fall <= 0 || top < 0 || bottom < 0)
        throw std::invalid_argument("invalid cycle durations or time");
    const double period = rise + top + fall + bottom;
    if (!std::isfinite(period)) throw std::invalid_argument("period overflow");
    double t = std::fmod(time, period);
    if (t < rise) return segment(low, high, rise, t);
    t -= rise;
    if (t < top) return segment(high, high, 1, 0);
    t -= top;
    if (t < fall) return segment(high, low, fall, t);
    return segment(low, low, 1, 0);
}
}
