#pragma once

#include "MapImageGenerator.h"

class DungeonMapImageGenerator : public MapImageGenerator {
public:
    explicit DungeonMapImageGenerator(MapGenParams params);

    bool Generate(std::vector<std::uint8_t>& outBiomes, std::string& outError) override;
    MapGenColor BiomeToColor(std::uint8_t biome) const override;
};
