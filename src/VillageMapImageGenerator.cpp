#include "VillageMapImageGenerator.h"

#include "TownMapImageGenerator.h"
#include "SettlementSite.h"

#include <utility>
#include <algorithm>

VillageMapImageGenerator::VillageMapImageGenerator(MapGenParams params)
    : MapImageGenerator(std::move(params)) {}

bool VillageMapImageGenerator::Generate(MapPlan& outPlan, std::string& outError) {
    if (GetParam("assetFoundation",0)>0) {
        // Growth candidates operate on the final assets and roads. Do not
        // score and discard a legacy settlement before the actual planning.
        outPlan.Reset(params_.width,params_.depth,params_.seed,"village");
        const auto site=static_cast<SettlementSite>(std::clamp(static_cast<int>(GetParam("settlementSite",params_.seed%7)),0,6));
        const int rotation=std::clamp(static_cast<int>(GetParam("siteRotation",(params_.seed/7)%4)),0,3);
        std::vector<std::uint8_t> footprint,structure;
        outPlan.biomes.assign(params_.width*params_.depth,PLAIN);
        PrepareSettlementSite(outPlan,footprint,structure,site,rotation,
            static_cast<std::uint8_t>(site==SettlementSite::Oasis ? DESERT : PLAIN),true,false,false);
        outPlan.archetype="village"; outPlan.cityType="village";
        return true;
    }
    MapGenParams villageParams = params_;
    // The shared settlement planner still owns the footprint, road graph,
    // typed buildings, furniture, and semantic export. The standalone type
    // selects the rural branch and receives village-specific validation in
    // MapQuality after this method returns.
    villageParams.extra["townMode"] = 1.0;
    villageParams.extra["settlementType"] = 1.0;
    TownMapImageGenerator planner(std::move(villageParams));
    if (!planner.Generate(outPlan, outError)) return false;
    outPlan.type = "village";
    outPlan.archetype = "village";
    outPlan.cityType = "village";
    return true;
}

MapGenColor VillageMapImageGenerator::BiomeToColor(std::uint8_t biome) const {
    TownMapImageGenerator palette(params_);
    return palette.BiomeToColor(biome);
}
