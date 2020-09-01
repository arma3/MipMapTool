#include "TextureFile.hpp"
#include <fstream>

#include <bitset>
#include <limits>
#include <type_traits>

template<typename T>
//static inline  // static if you want to compile with -mpopcnt in one compilation unit but not others
typename std::enable_if<std::is_integral<T>::value, unsigned >::type
popcount(T x)
{
    static_assert(std::numeric_limits<T>::radix == 2, "non-binary type");

    // sizeof(x)*CHAR_BIT
    constexpr int bitwidth = std::numeric_limits<T>::digits + std::numeric_limits<T>::is_signed;
    // std::bitset constructor was only unsigned long before C++11.  Beware if porting to C++03
    static_assert(bitwidth <= std::numeric_limits<unsigned long long>::digits, "arg too wide for std::bitset() constructor");

    typedef typename std::make_unsigned<T>::type UT;        // probably not needed, bitset width chops after sign-extension

    std::bitset<bitwidth> bs(static_cast<UT>(x));
    return bs.count();
}

bool MipMap::readMipmap(std::istream& input, uint32_t expectedDataSize) {

    input.read(reinterpret_cast<char*>(&width), 2);
    input.read(reinterpret_cast<char*>(&height), 2);
    if (!width || !height) return false;

    if (popcount(getRealSize()) != 1 || popcount(height) != 1) //power of 2 can only have one bit set
        std::cout << "\tWARN width or height not power of 2 " << (width & 0x7FFF) << "/" << height << "\n";

    uint32_t length = 0;
    input.read(reinterpret_cast<char*>(&length), 3);
    if (verboseLog)
        std::cout << "\tMipMap data size: " << length << "\n";
    if (length == 0) {
        if (expectedDataSize) {
            std::cout << "\tWARN data size is 0, but expected size is " << expectedDataSize << ". Something is wrong with this mip. Using expected size instead\n";
            length = expectedDataSize;
        } else {
            std::cout << "\tWARN data size is 0, and expected size is also 0. Something is wrong with this mip.\n";
        }
    }
    data.resize(length);
    input.read(data.data(), length);

    return true; //There are mipmaps left
}

void MipMap::writeMipmap(std::ostream& output) {
    output.write(reinterpret_cast<char*>(&width), 2);
    output.write(reinterpret_cast<char*>(&height), 2);

    uint32_t length = data.size();
    output.write(reinterpret_cast<char*>(&length), 3);
    output.write(data.data(), length);
}

void TextureFile::readFromStream(std::istream& input) {
    input.read(reinterpret_cast<char*>(&type), 2);

    while (true) {
        char tagsig[5];
        input.read(tagsig, 4);
        tagsig[4] = 0x00;
        if (std::string_view(tagsig) != "GGAT") {
            input.seekg(-4, std::istream::cur);//This is not a tag
            break;
        }

        char taggname[5];
        input.read(taggname, 4);
        taggname[4] = 0x00;
        std::string_view tagName(taggname);

        uint32_t taglen;
        input.read(reinterpret_cast<char*>(&taglen), 4);
        std::string tagData;
        tagData.resize(taglen);
        input.read(tagData.data(), taglen);
        tags[std::string(tagName)] = tagData;
        if (verboseLog) {
            std::reverse(taggname, &taggname[4]);
            std::cout << "\tGot tag " << std::string_view(taggname, 4) << " of size " << taglen << "\n";
        }
    }


    std::vector<uint32_t> mipmapOffsets;

    if (tags.contains("SFFO")) {
        auto offsetdata = tags["SFFO"];

        auto offsCount = offsetdata.length() / sizeof(uint32_t);

        mipmapOffsets.reserve(offsCount);

        for (int i = 0; i < offsCount; ++i) {
            auto substr = std::string_view(offsetdata).substr(i * 4, 4);
            uint32_t val = *reinterpret_cast<const uint32_t*>(substr.data());

            if (val != 0) //There are 0's in here? There are always 16 elements even though only some of them are filled.
                mipmapOffsets.emplace_back(val); //#TODO these are mipmap offsets, we should store these
        }
    }



    if (tags.contains("CGVA")) {
        auto offsetdata = tags["CGVA"];
        auto substr = std::string_view(offsetdata);
        avgColor = *reinterpret_cast<const uint32_t*>(substr.data());
    }

    if (tags.contains("CXAM")) {
        auto offsetdata = tags["CXAM"];
        auto substr = std::string_view(offsetdata);
        maxColor = *reinterpret_cast<const uint32_t*>(substr.data());
    }

    if (tags.contains("GALF")) {
        auto offsetdata = tags["GALF"];
        auto substr = std::string_view(offsetdata);
        uint32_t flags = *reinterpret_cast<const uint32_t*>(substr.data());
        if (flags & 1) isAlpha = true;
        if (flags & 2) isTransparent = true;
    }




    uint16_t paletteSize;
    input.read(reinterpret_cast<char*>(&paletteSize), sizeof(paletteSize));
    if (verboseLog)
        std::cout << "\tPaletteSize " << paletteSize << "\n";
    paletteData.resize(paletteSize * 3);
    input.read(paletteData.data(), paletteSize * 3);



    auto pos = input.tellg();


    for (auto& mipmap : mipmapOffsets) {
        input.seekg(mipmap, std::ifstream::beg);
        auto newMap = std::make_shared<MipMap>();
        newMap->inputPath = inputPath;
        newMap->verboseLog = verboseLog;

        auto nextMap = std::find(mipmapOffsets.begin(), mipmapOffsets.end(), mipmap) + 1;
        uint32_t expectedSize = 0;
        if (nextMap != mipmapOffsets.end())
            expectedSize = *nextMap - mipmap - 7; //minus width,height,length header

        if (verboseLog)
            std::cout << "\tMipMap Expected size: " << expectedSize << "\n";

        if (doLogging && expectedSize > 8'388'607)
            std::cout << "\tWARN MipMap Expected size too big to fit " << expectedSize << ". This will create problems with DXT compressed textures\n";

        newMap->readMipmap(input, expectedSize);
        auto curPos = input.tellg();
        if (doLogging)
            std::cout << "\tGot mipmap size " << newMap->getRealSize() << "x" << newMap->height << (newMap->width & 0x8000 ? " is compressed" : "") << "\n";
        mipmaps.emplace_back(std::move(newMap));
    }
}

void TextureFile::readFromFile(std::filesystem::path path) {
    inputPath = path;
    std::cout << "Texture read " << inputPath << "\n";
    std::ifstream input(path, std::ifstream::binary);

    readFromStream(input);

    std::cout
        << "\ttype=" << TypeToString(type) << "\n"
        << "\tmipmaps=" << mipmaps.size() << "\n"
        << "\tisAlpha=" << isAlpha << "\n"
        << "\tisTransparent=" << isTransparent << "\n"
        << "\tavgColor=" << std::hex << avgColor << std::dec << "\n"
        << "\tmaxColor=" << std::hex << maxColor << std::dec << "\n";
}

void TextureFile::writeToFile(std::filesystem::path path) {
    std::ofstream output(path, std::ofstream::binary);

    output.write(reinterpret_cast<char*>(&type), 2);

    uint32_t offsetsOffset = 0;
    for (auto& [key,val] : tags) {
        output.write("GGAT", 4);
        output.write(key.c_str(), 4);

        if (key == "SFFO") {
            uint32_t taglen = 16 * 4; //There are always 16 mipmaps, just some are empty...
            output.write(reinterpret_cast<char*>(&taglen), 4);
            offsetsOffset = output.tellp();
            for (int i = 0; i < 16; ++i) {
                output.write("\0\0\0\0", 4);
            }
        } else {
            uint32_t taglen = val.length();
            output.write(reinterpret_cast<char*>(&taglen), 4);
            output.write(val.c_str(), val.length());
        }
    }

    uint16_t paletteSize = paletteData.size() / 3;
    output.write(reinterpret_cast<char*>(&paletteSize), sizeof(paletteSize));
    output.write(paletteData.data(), paletteData.size());

    auto pos = output.tellp();

    //Sort by their max mipmap size
    std::sort(mipmaps.begin(), mipmaps.end(), [](auto& left, auto& right) {
        return left->getRealSize() > right->getRealSize();
    });

    std::vector<uint32_t> mipmapOffsets;

    for (auto& mipmap : mipmaps) {
        mipmapOffsets.push_back(output.tellp());
        mipmap->writeMipmap(output);
    }

    output.write("\0\0\0\0\0\0", 6); //dummy mipmap, 0 width/height

    output.seekp(offsetsOffset);
    for (auto& it : mipmapOffsets)
        output.write(reinterpret_cast<char*>(&it), 4);
}

std::shared_ptr<TextureFile> TextureFile::copyNoMipmap() {
    auto texFile = std::make_shared<TextureFile>();
   
    texFile->isAlpha = isAlpha;
    texFile->isTransparent = isTransparent;
    texFile->type = type;
    texFile->avgColor = avgColor;
    texFile->maxColor = maxColor;
    texFile->tags = tags;
    texFile->paletteData = paletteData;
    return texFile;
}
