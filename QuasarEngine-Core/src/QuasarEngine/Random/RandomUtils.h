#pragma once

#include <random>

namespace QuasarEngine
{
    class RandomUtils {
    public:
        static int getRandomInt(int min, int max) {
            std::uniform_int_distribution<int> distribution(min, max);
            return distribution(generator);
        }

        static float getRandomFloat(float min, float max) {
            std::uniform_real_distribution<float> distribution(min, max);
            return distribution(generator);
        }

    private:
        static std::random_device rd;
        static std::mt19937 generator;
    };

    std::random_device RandomUtils::rd;
    std::mt19937 RandomUtils::generator(rd());
}