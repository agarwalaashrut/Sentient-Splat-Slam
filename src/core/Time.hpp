#pragma once

#include <chrono>

namespace sss {

class Time {
public:
    /**
     * Initialize timing (call once at app start).
     */
    static void init();

    /**
     * Update timing for current frame (call once per frame).
     */
    static void tick();

    /**
     * Get delta time in seconds since last tick.
     */
    static float delta_time() { return s_delta_time; }

    /**
     * Get FPS smoothed over several frames.
     */
    static float fps() { return s_fps; }

    /**
     * Get total elapsed time in seconds.
     */
    static float elapsed() { return s_elapsed; }

private:
    static float s_delta_time;
    static float s_fps;
    static float s_elapsed;
    static std::chrono::high_resolution_clock::time_point s_last_time;
};

}  // namespace sss
