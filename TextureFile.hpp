#pragma once
#include <iostream>
#include <vector>
#include <filesystem>
#include <map>

class MipMap {
public:
    std::filesystem::path inputPath;
    uint16_t width{ 0 };
    uint16_t height{ 0 };
    std::vector<char> data;
    bool verboseLog = false;

    uint16_t getRealSize() const {
        return width & 0x7fff;
    }
    bool isCompressed() const {
        return width & 0x8000;
    }

    bool readMipmap(std::istream& input, uint32_t expectedDataSize = 0);
    void writeMipmap(std::ostream& output);
};


enum class PAAType : uint16_t {
    def,
    DXT1 = 0xFF01,
    DXT3 = 0xFF03,
    DXT5 = 0xFF05,
    ARGB4444 = 0x4444,
    ARGB1555 = 0x1555,
    AI88 = 0x8080,
    invalid
};

inline std::string_view TypeToString(PAAType t) {
    switch (t) {
    case PAAType::def: return "default";
    case PAAType::DXT1: return "DXT1";
    case PAAType::DXT3: return "DXT3";
    case PAAType::DXT5: return "DXT5";
    case PAAType::ARGB4444: return "ARGB4444";
    case PAAType::ARGB1555: return "ARGB1555";
    case PAAType::AI88: return "AI88";
    case PAAType::invalid: return "invalid";
    default: return "default";
    }
}


class TextureFile {
public:

    void readFromStream(std::istream& input);
    void readFromFile(std::filesystem::path path);
    void writeToFile(std::filesystem::path path);
    std::shared_ptr<TextureFile> copyNoMipmap();


    std::filesystem::path inputPath;
    bool isAlpha = false;
    bool isTransparent = false;

    PAAType type;
    uint32_t avgColor{ 0xff802020 };
    uint32_t maxColor{ 0xffffffff };
    std::map<std::string, std::string> tags;
    std::vector<char> paletteData;
    std::vector<std::shared_ptr<MipMap>> mipmaps;
    bool verboseLog = false;
    bool doLogging = true;

};

