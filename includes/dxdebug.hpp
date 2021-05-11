#ifndef DXDEBUG_H
#define DXDEBUG_H

#include <chrono>

#define NOW() std::chrono::high_resolution_clock::now()

typedef std::chrono::time_point<std::chrono::high_resolution_clock> HRCTime;
typedef std::chrono::duration<float, std::milli> MilliD;

class DXDebug {
    public:
    HRCTime last_frame;
    HRCTime current_frame;
    float fps;
    size_t frame_count;
    DXDebug() : last_frame(NOW()), current_frame(NOW()), frame_count(0) {};

    void Update() {
        this->last_frame = current_frame;
        this->current_frame = NOW();

        // Only update fps every 15 frames so its readable in the debug display
        if (this->frame_count % 15 == 0) {
            MilliD millis_passed = this->current_frame - this->last_frame;
            this->fps = 1000.0f / millis_passed.count();
        }

        this->frame_count++;
    }
};

#endif