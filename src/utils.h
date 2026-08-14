#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace mapgen {

struct Rgb {
    std::uint8_t r;
    std::uint8_t g;
    std::uint8_t b;
};

float HashNoise2D(int x, int y, std::uint32_t seed);
float ValueNoise2D(float x, float y, std::uint32_t seed);
float Fbm2D(float x, float y, int octaves, float lacunarity, float gain, std::uint32_t seed);
float Worley2D(float x, float y, float cellSize, std::uint32_t seed);

bool WritePngRgb(const std::string& path, int width, int height, const std::vector<Rgb>& rgb, std::string& outError);

}  // namespace mapgen
