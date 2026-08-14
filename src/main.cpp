#include "MapImageGenerator.h"
#include "CaveMapImageGenerator.h"
#include "DungeonMapImageGenerator.h"
#include "WorldMapImageGenerator.h"
#include "utils.h"

#include <array>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <random>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

struct CliOptions {
    std::string type = "world";
    int width = 256;
    int depth = 256;
    bool widthProvided = false;
    bool depthProvided = false;
    std::uint32_t seed = 0;
    bool seedProvided = false;
    std::unordered_map<std::string, double> extra;
    std::filesystem::path outDir = ".";
    std::string outName = "out";
    bool writePng = true;
    bool writeData = false;
    bool writeReport = false;
};

bool ParseParamKV(const std::string& token, std::string& outKey, double& outValue) {
    const std::size_t eq = token.find('=');
    if (eq == std::string::npos || eq == 0 || eq + 1 >= token.size()) {
        return false;
    }
    outKey = token.substr(0, eq);
    try {
        outValue = std::stod(token.substr(eq + 1));
    } catch (...) {
        return false;
    }
    return true;
}

bool ParseArgs(int argc, char** argv, CliOptions& opt, std::string& outError) {
    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];

        auto requireNext = [&](const char* flag) -> const char* {
            if (i + 1 >= argc) {
                outError = std::string("Missing value for ") + flag;
                return nullptr;
            }
            return argv[++i];
        };

        if (a == "-type") {
            const char* v = requireNext("-type");
            if (v == nullptr) {
                return false;
            }
            opt.type = v;
        } else if (a == "-w") {
            const char* v = requireNext("-w");
            if (v == nullptr) {
                return false;
            }
            opt.width = std::stoi(v);
            opt.widthProvided = true;
        } else if (a == "-d") {
            const char* v = requireNext("-d");
            if (v == nullptr) {
                return false;
            }
            opt.depth = std::stoi(v);
            opt.depthProvided = true;
        } else if (a == "-seed") {
            const char* v = requireNext("-seed");
            if (v == nullptr) {
                return false;
            }
            opt.seed = static_cast<std::uint32_t>(std::stoul(v));
            opt.seedProvided = true;
        } else if (a == "-p") {
            const char* v = requireNext("-p");
            if (v == nullptr) {
                return false;
            }
            std::string key;
            double value = 0.0;
            if (!ParseParamKV(v, key, value)) {
                outError = std::string("Invalid -p format: ") + v + " (expected key=value)";
                return false;
            }
            opt.extra[key] = value;
        } else if (a == "-dir") {
            const char* v = requireNext("-dir");
            if (v == nullptr) {
                return false;
            }
            opt.outDir = v;
        } else if (a == "-out") {
            const char* v = requireNext("-out");
            if (v == nullptr) {
                return false;
            }
            opt.outName = v;
        } else if (a == "--png") {
            opt.writePng = true;
        } else if (a == "--data") {
            opt.writeData = true;
        } else if (a == "--report") {
            opt.writeReport = true;
        } else {
            outError = "Unknown argument: " + a;
            return false;
        }
    }

    if (opt.type != "world" && opt.type != "dungeon" && opt.type != "cave") {
        outError = "Only -type world, -type dungeon, or -type cave is currently implemented";
        return false;
    }

    if (opt.type == "dungeon" || opt.type == "cave") {
        if (!opt.widthProvided) {
            opt.width = 160;
        }
        if (!opt.depthProvided) {
            opt.depth = 160;
        }
    }

    if (opt.width < 64 || opt.width > 512 || opt.depth < 64 || opt.depth > 512) {
        outError = "-w and -d must be in range [64, 512]";
        return false;
    }

    if (!opt.seedProvided) {
        const std::uint64_t t = static_cast<std::uint64_t>(
            std::chrono::high_resolution_clock::now().time_since_epoch().count());
        std::seed_seq seq{static_cast<std::uint32_t>(t & 0xffffffffU), static_cast<std::uint32_t>(t >> 32)};
        std::mt19937 rng(seq);
        opt.seed = rng();
    }

    return true;
}

bool WriteDataFile(const std::filesystem::path& path, int w, int h, const std::vector<std::uint8_t>& biomes, std::string& outError) {
    std::ofstream ofs(path);
    if (!ofs) {
        outError = "Failed to open data output: " + path.string();
        return false;
    }

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            if (x != 0) {
                ofs << ',';
            }
            ofs << static_cast<int>(biomes[static_cast<std::size_t>(y) * static_cast<std::size_t>(w) + static_cast<std::size_t>(x)]);
        }
        ofs << '\n';
    }

    return true;
}

bool WriteReportFile(const std::filesystem::path& path, int w, int h, std::uint32_t seed, const std::string& type, const std::vector<std::uint8_t>& biomes, std::string& outError) {
    std::array<int, 10> count{};
    for (std::uint8_t b : biomes) {
        if (b < count.size()) {
            ++count[static_cast<std::size_t>(b)];
        }
    }

    std::ofstream ofs(path);
    if (!ofs) {
        outError = "Failed to open report output: " + path.string();
        return false;
    }

    const int total = w * h;
    ofs << "seed=" << seed << '\n';
    ofs << "size=" << w << "x" << h << "\n\n";

    auto pct = [total](int v) {
        return (100.0 * static_cast<double>(v) / static_cast<double>(std::max(1, total)));
    };

    if (type == "cave") {
        ofs << "FLOOR_LV1=" << count[FLOOR_LV1] << " (" << pct(count[FLOOR_LV1]) << "%)\n";
        ofs << "FLOOR_LV2=" << count[FLOOR_LV2] << " (" << pct(count[FLOOR_LV2]) << "%)\n";
        ofs << "WALL=" << count[WALL] << " (" << pct(count[WALL]) << "%)\n";
        ofs << "INNERWALL=" << count[INNERWALL] << " (" << pct(count[INNERWALL]) << "%)\n";
        ofs << "LADDER=" << count[LADDER] << " (" << pct(count[LADDER]) << "%)\n";
        ofs << "RIVER=" << count[RIVER] << " (" << pct(count[RIVER]) << "%)\n";
        ofs << "LAKE=" << count[LAKE] << " (" << pct(count[LAKE]) << "%)\n";
        return true;
    }

    ofs << "PLAIN=" << count[PLAIN] << " (" << pct(count[PLAIN]) << "%)\n";
    ofs << "DESERT=" << count[DESERT] << " (" << pct(count[DESERT]) << "%)\n";
    ofs << "FOREST=" << count[FOREST] << " (" << pct(count[FOREST]) << "%)\n";
    ofs << "MOUNTAIN=" << count[MOUNTAIN] << " (" << pct(count[MOUNTAIN]) << "%)\n";
    ofs << "EXMOUNTAIN=" << count[EXMOUNTAIN] << " (" << pct(count[EXMOUNTAIN]) << "%)\n";
    ofs << "ROAD=" << count[ROAD] << " (" << pct(count[ROAD]) << "%)\n";
    ofs << "BRIDGE=" << count[BRIDGE] << " (" << pct(count[BRIDGE]) << "%)\n";
    ofs << "RIVER=" << count[RIVER] << " (" << pct(count[RIVER]) << "%)\n";
    ofs << "LAKE=" << count[LAKE] << " (" << pct(count[LAKE]) << "%)\n";
    ofs << "SEA=" << count[SEA] << " (" << pct(count[SEA]) << "%)\n";

    return true;
}

}  // namespace

int main(int argc, char** argv) {
    CliOptions opt;
    std::string error;

    try {
        if (!ParseArgs(argc, argv, opt, error)) {
            std::cerr << "Argument error: " << error << '\n';
            std::cerr << "Usage: mapimggen.exe -type world|dungeon|cave -w 256 -d 256 -seed 12345 [-p key=value] -dir out -out name [--data --png --report] (dungeon/cave default: 160x160 if -w/-d omitted)\n";
            return 1;
        }
    } catch (const std::exception& ex) {
        std::cerr << "Argument parse exception: " << ex.what() << '\n';
        return 1;
    }

    MapGenParams params;
    params.width = opt.width;
    params.depth = opt.depth;
    params.seed = opt.seed;
    params.extra = opt.extra;

    std::unique_ptr<MapImageGenerator> generator;
    if (opt.type == "dungeon") {
        generator = std::make_unique<DungeonMapImageGenerator>(std::move(params));
    } else if (opt.type == "cave") {
        generator = std::make_unique<CaveMapImageGenerator>(std::move(params));
    } else {
        generator = std::make_unique<WorldMapImageGenerator>(std::move(params));
    }

    std::vector<std::uint8_t> biomes;
    if (!generator->Generate(biomes, error)) {
        std::cerr << "Generation failed: " << error << '\n';
        return 2;
    }

    std::filesystem::create_directories(opt.outDir);

    if (opt.writePng) {
        std::vector<mapgen::Rgb> rgb;
        rgb.reserve(biomes.size());
        for (std::uint8_t b : biomes) {
            const MapGenColor c = generator->BiomeToColor(b);
            rgb.push_back({c.r, c.g, c.b});
        }

        std::filesystem::path pngPath = opt.outDir / (opt.outName + ".png");
        if (!mapgen::WritePngRgb(pngPath.string(), opt.width, opt.depth, rgb, error)) {
            std::cerr << "PNG write failed: " << error << '\n';
            return 3;
        }
        std::cout << "PNG: " << pngPath.string() << '\n';
    }

    if (opt.writeData) {
        std::filesystem::path dataPath = opt.outDir / (opt.outName + "_biome.csv");
        if (!WriteDataFile(dataPath, opt.width, opt.depth, biomes, error)) {
            std::cerr << "Data write failed: " << error << '\n';
            return 4;
        }
        std::cout << "DATA: " << dataPath.string() << '\n';
    }

    if (opt.writeReport) {
        std::filesystem::path reportPath = opt.outDir / (opt.outName + "_report.txt");
        if (!WriteReportFile(reportPath, opt.width, opt.depth, opt.seed, opt.type, biomes, error)) {
            std::cerr << "Report write failed: " << error << '\n';
            return 5;
        }
        std::cout << "REPORT: " << reportPath.string() << '\n';
    }

    std::cout << "Done. seed=" << opt.seed << '\n';
    return 0;
}
