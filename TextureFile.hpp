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

    uint16_t getRealSize() const {
        return width & 0x7fff;
    }

    bool readMipmap(std::istream& input);
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

class TextureFile {
public:

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

};

