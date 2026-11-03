#include "AssetFoundation.h"
#include <iostream>
#include <stdexcept>

void Require(bool ok, const char* text) { if (!ok) throw std::runtime_error(text); }
MapGenParams Params(int wallHeight=0) {
    MapGenParams p;
    p.extra={{"assetCount",4},{"asset0Id",0},{"asset0Width",12},{"asset0Depth",12},
        {"asset1Id",5},{"asset1Width",26},{"asset1Depth",17},
        {"asset2Id",27},{"asset2Width",6},{"asset2Depth",9},{"asset2Category",1},
        {"asset3Id",38},{"asset3Width",13},{"asset3Depth",11},{"asset3Category",2},
        {"wallHeight",wallHeight}};
    return p;
}
MapPlan Terrain() {
    // Castle retains the generic foundation planner. Village and town now
    // use the shared door-aware growth planner tested separately.
    MapPlan p; p.Reset(160,160,42,"castle"); p.biomes.assign(160*160,PLAIN);
    for(int y=76;y<82;++y) for(int x=0;x<160;++x) p.biomes[y*160+x]=RIVER;
    for(int y=76;y<82;++y) for(int x=77;x<83;++x) p.biomes[y*160+x]=BRIDGE;
    for(int i=4;i<156;++i) {
        p.biomes[4*160+i]=WALL; p.biomes[155*160+i]=WALL;
        p.biomes[i*160+4]=WALL; p.biomes[i*160+155]=WALL;
    }
    p.regions.push_back({"wall",0,4,4,155,155});
    p.biomes[20*160+20]=BRIDGE;
    p.markers.push_back({"building_door",20,20,1});
    return p;
}
int main() {
    try {
        for(int walls : {0,6}) for(int seed : {1,42,4242}) for(int roadWidth : {2,4,6}) {
            auto p=Terrain(); auto args=Params(walls); args.seed=seed; std::string error;
            args.extra["assetRoadWidth"]=roadWidth;
            Require(BuildAssetFoundation(p,args,error),error.c_str());
            Require(EvaluateAssetFoundation(p).Passed(),"2x2 route disconnected");
            Require(p.biomes[20*160+20]!=RIVER,"dry doorway became a puddle");
            for(const auto& m:p.markers) for(int dy=0;dy<roadWidth;++dy) for(int dx=0;dx<roadWidth;++dx) {
                Require(m.x+dx<p.width && m.y+dy<p.depth,"road outside map");
                const auto b=p.biomes[(m.y+dy)*p.width+m.x+dx];
                Require(b==ROAD || b==BRIDGE,"requested road width was clipped");
            }
            bool bridge=false,wall=false;
            for(const auto& r:p.regions) {
                const int width=r.x1-r.x0+1, depth=r.y1-r.y0+1;
                if(r.kind=="asset_plot") {
                    Require((r.role=="MA000" && width==12 && depth==12) ||
                        (r.role=="MA005" && width==26 && depth==17),"asset size changed");
                    for(int y=r.y0;y<=r.y1;++y) for(int x=r.x0;x<=r.x1;++x)
                        Require(p.biomes[y*p.width+x]==TOWN_WOOD_FLOOR,"plot is not flat");
                }
                if(r.kind=="asset_bridge") {bridge=true; Require(width==6 && depth==9,"bridge size changed");}
                if(r.kind=="asset_wall") {wall=true; Require(width==13 && depth==11,"wall size changed");}
            }
            Require(bridge,"bridge footprint missing");
            Require(wall==(walls>0),"wall option ignored");
            if(!walls) for(auto b:p.biomes) Require(b!=WALL && b!=INNERWALL,"wall remains at height zero");
            const auto repeatTerrain=Terrain(); auto repeat=repeatTerrain;
            Require(BuildAssetFoundation(repeat,args,error) && repeat.biomes==p.biomes,"not deterministic");
            // Simulate an obstacle at an approach after planning. Validation
            // must catch a blockage for the full 2x2 human, even one cell.
            const auto m=p.markers.front(); p.biomes[m.y*p.width+m.x+1]=WALL;
            Require(!EvaluateAssetFoundation(p).Passed(),"2x2 obstruction undetected");
        }
        auto p=Terrain(); auto args=Params(); args.extra["asset0Width"]=1000; args.extra["asset1Width"]=1000;
        std::string error;
        Require(!BuildAssetFoundation(p,args,error),"oversized assets were silently shrunk");
        std::cout<<"PASS asset footprints, bridge shores, optional walls, determinism and 2x2 clearance\n";
    } catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
}
