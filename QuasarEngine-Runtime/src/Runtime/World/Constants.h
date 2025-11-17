#pragma once

constexpr int CHUNK_SIZE = 32;
constexpr int CHUNK_HEIGHT = 256;
constexpr int CHUNK_AREA = CHUNK_SIZE * CHUNK_SIZE;
constexpr int CHUNK_VOLUME = CHUNK_AREA * CHUNK_HEIGHT;

constexpr int RENDER_DISTANCE = 20;

constexpr int NUMBER_OF_CHUNKS_TO_GENERATE = 2;

constexpr float HEIGHT_SCALE = 256.0f;
constexpr unsigned MAX_HEIGHT = 250;
constexpr unsigned MIN_HEIGHT = 40;

constexpr float LAND_SCALE = 2048.0f;
constexpr float LAND_MIN_MULT = 0.3f; // 0.1f
constexpr float LAND_TRANSITION_SHARPNESS = 1.0f;

constexpr float TERRAIN_INTERP_GRIP = 4.0f;