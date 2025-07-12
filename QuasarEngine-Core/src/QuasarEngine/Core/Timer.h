#pragma once

#include <chrono>
#include <unordered_map>

namespace QuasarEngine
{
    class Timer {
    public:
        Timer() : lastTime(std::chrono::high_resolution_clock::time_point::min()), deltaTime(0) {}

        void update() {
            auto currentTime = std::chrono::high_resolution_clock::now();
            deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
            lastTime = currentTime;
        }

        float getDeltaTime() const { return deltaTime; }

        void startTimer(const std::string& timerName) {
            timers[timerName] = std::chrono::high_resolution_clock::now();
        }

        float getElapsedTime(const std::string& timerName) {
            auto now = std::chrono::high_resolution_clock::now();
            auto start = timers[timerName];
            return std::chrono::duration<float>(now - start).count();
        }

        bool isElapsed(const std::string& timerName, float threshold) {
            return getElapsedTime(timerName) >= threshold;
        }

    private:
        std::unordered_map<std::string, std::chrono::high_resolution_clock::time_point> timers;

        std::chrono::high_resolution_clock::time_point lastTime;

        float deltaTime;
    };
}