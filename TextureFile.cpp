#include "TextureFile.hpp"
#include <fstream>

bool MipMap::readMipmap(std::istream& input) {

    input.read(reinterpret_cast<char*>(&width), 2);
    input.read(reinterpret_cast<char*>(&height), 2);
    if (!width || !height) return false;

    uint32_t length = 0;
    input.read(reinterpret_cast<char*>(&length), 3);
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

void TextureFile::readFromFile(std::filesystem::path path) {
    inputPath = path;
    std::cout << "Texture read " << inputPath << "\n";
    std::ifstream input(path, std::ifstream::binary);

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
    paletteData.resize(paletteSize * 3);
    input.read(paletteData.data(), paletteSize * 3);



    auto pos = input.tellg();


    for (auto& mipmap : mipmapOffsets) {
        input.seekg(mipmap, std::ifstream::beg);
        auto newMap = std::make_shared<MipMap>();
        newMap->inputPath = inputPath;
        newMap->readMipmap(input);
        auto curPos = input.tellg();
        std::cout << "\tGot mipmap size " << newMap->getRealSize() << "x" << newMap->height << (newMap->width & 0x8000 ? " is compressed" : "") << "\n";
        mipmaps.emplace_back(std::move(newMap));
    }

    std::cout
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
