#pragma once

#include <vector>
#include <cmath>
#include <random>

#include <QuasarEngine/Math/MathUtils.h>

class PerlinNoise {
public:
    PerlinNoise() {
        std::random_device rd;
        std::mt19937 gen(rd());

        std::uniform_int_distribution<> dis(0, 255);

        for (int i = 0; i < 256; ++i) {
            permutation[i] = dis(gen);
        }

        for (int i = 0; i < 256; ++i) {
            permutation[256 + i] = permutation[i];
        }
    }

    PerlinNoise(unsigned int seed = 0) {
        std::mt19937 gen(seed);

        std::uniform_int_distribution<> dis(0, 255);

        for (int i = 0; i < 256; ++i) {
            permutation[i] = dis(gen);
        }

        for (int i = 0; i < 256; ++i) {
            permutation[256 + i] = permutation[i];
        }
    }

    double noise(double x, double y) {
        int xi = static_cast<int>(floor(x)) & 255;
        int yi = static_cast<int>(floor(y)) & 255;
        double xf = x - floor(x);
        double yf = y - floor(y);

        double u = MathUtils::fade(xf);
        double v = MathUtils::fade(yf);

        int aa = permutation[xi + permutation[yi]] & 255;
        int ab = permutation[xi + permutation[yi + 1]] & 255;
        int ba = permutation[xi + 1 + permutation[yi]] & 255;
        int bb = permutation[xi + 1 + permutation[yi + 1]] & 255;

        double gradAA = MathUtils::grad(aa, xf, yf);
        double gradAB = MathUtils::grad(ab, xf, yf - 1);
        double gradBA = MathUtils::grad(ba, xf - 1, yf);
        double gradBB = MathUtils::grad(bb, xf - 1, yf - 1);

        double x1 = MathUtils::lerp(gradAA, gradBA, u);
        double x2 = MathUtils::lerp(gradAB, gradBB, u);
        return MathUtils::lerp(x1, x2, v);
    }

    double noise(double x, double y, double z) {
        int xi = static_cast<int>(floor(x)) & 255;
        int yi = static_cast<int>(floor(y)) & 255;
        int zi = static_cast<int>(floor(z)) & 255;

        double xf = x - floor(x);
        double yf = y - floor(y);
        double zf = z - floor(z);

        double u = MathUtils::fade(xf);
        double v = MathUtils::fade(yf);
        double w = MathUtils::fade(zf);

        int aaa = permutation[xi + permutation[yi + permutation[zi]]] & 255;
        int aab = permutation[xi + permutation[yi + permutation[zi + 1]]] & 255;
        int aba = permutation[xi + permutation[yi + 1 + permutation[zi]]] & 255;
        int abb = permutation[xi + permutation[yi + 1 + permutation[zi + 1]]] & 255;
        int baa = permutation[xi + 1 + permutation[yi + permutation[zi]]] & 255;
        int bab = permutation[xi + 1 + permutation[yi + permutation[zi + 1]]] & 255;
        int bba = permutation[xi + 1 + permutation[yi + 1 + permutation[zi]]] & 255;
        int bbb = permutation[xi + 1 + permutation[yi + 1 + permutation[zi + 1]]] & 255;

        double gradAAA = MathUtils::grad(aaa, xf, yf, zf);
        double gradAAB = MathUtils::grad(aab, xf, yf, zf - 1);
        double gradABA = MathUtils::grad(aba, xf, yf - 1, zf);
        double gradABB = MathUtils::grad(abb, xf, yf - 1, zf - 1);
        double gradBAA = MathUtils::grad(baa, xf - 1, yf, zf);
        double gradBAB = MathUtils::grad(bab, xf - 1, yf, zf - 1);
        double gradBBA = MathUtils::grad(bba, xf - 1, yf - 1, zf);
        double gradBBB = MathUtils::grad(bbb, xf - 1, yf - 1, zf - 1);

        double x1 = MathUtils::lerp(gradAAA, gradBAA, u);
        double x2 = MathUtils::lerp(gradABA, gradBBA, u);
        double y1 = MathUtils::lerp(x1, x2, v);

        x1 = MathUtils::lerp(gradAAB, gradBAB, u);
        x2 = MathUtils::lerp(gradABB, gradBBB, u);
        double y2 = MathUtils::lerp(x1, x2, v);

        return MathUtils::lerp(y1, y2, w);
    }

private:
    int permutation[512];
};