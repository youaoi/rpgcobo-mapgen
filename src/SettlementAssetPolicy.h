#pragma once
#include <algorithm>

// The smallest authored houses have a longest edge of 12 blocks. Larger
// buildings keep their front/back axis; compact houses may also face sideways.
inline bool SettlementBuildingFrontOnly(int width,int depth,int category=0) {
    return category==0 && std::max(width,depth)>12;
}
inline bool SettlementBuildingOrientationAllowed(int width,int depth,int rotation,int doorSide,int category=0) {
    (void)rotation;
    return !SettlementBuildingFrontOnly(width,depth,category) || doorSide==0 || doorSide==2;
}
