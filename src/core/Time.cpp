#include "Time.hpp"

namespace sss {

float Time::s_delta_time = 0.f;
float Time::s_fps = 0.f;
float Time::s_elapsed = 0.f;
std::chrono::high_resolution_clock::time_point Time::s_last_time;

void Time::init() {
    s_last_time = std::chrono::high_resolution_clock::now();
    s_delta_time = 0.f;
    s_fps = 0.f;
    s_elapsed = 0.f;
}

void Time::tick() {
    auto now = std::chrono::high_resolution_clock::now();
    s_delta_time = std::chrono::duration<float>(now - s_last_time).count();
    s_last_time = now;

    if (s_delta_time > 0.f) {
        float instantaneous_fps = 1.f / s_delta_time;
        s_fps = (s_fps == 0.f) ? instantaneous_fps : (0.9f * s_fps + 0.1f * instantaneous_fps);
    }

    s_elapsed += s_delta_time;
}

}  // namespace sss
