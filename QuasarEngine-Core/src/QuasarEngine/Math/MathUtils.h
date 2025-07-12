#pragma once

#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace QuasarEngine
{
    class MathUtils {
    public:
        static float lerp(float a, float b, float t) {
            return a + (b - a) * t;
        }

        static float distance(float x1, float y1, float x2, float y2) {
            return std::sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
        }

        static float clamp(float value, float min, float max) {
            return std::max(min, std::min(value, max));
        }

        static float toRadians(float degrees) {
            return degrees * (M_PI / 180.0f);
        }

        static float toDegrees(float radians) {
            return radians * (180.0f / M_PI);
        }

        static double fade(double t) {
            return t * t * t * (t * (t * 6 - 15) + 10);
        }

        static double grad(int hash, double x, double y) {
            int h = hash & 15;
            double u = h < 8 ? x : y;
            double v = h < 4 ? y : (h == 12 || h == 14) ? x : 0;
            return ((h & 1 ? -1 : 1) * u + (h & 2 ? -1 : 1) * v);
        }

        static double grad(int hash, double x, double y, double z) {
            int h = hash & 15;
            double u = h < 8 ? x : y;
            double v = h < 4 ? y : (h == 12 || h == 14) ? x : z;
            return ((h & 1 ? -1 : 1) * u + (h & 2 ? -1 : 1) * v);
        }
    };
}