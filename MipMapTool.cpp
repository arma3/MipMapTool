#include <iostream>
#include <regex>
#include "TextureFile.hpp"
#include <optional>

void printStart() {
    std::cout << "MipMap Tool v8(wrrrm wrrm) OwO\n";
}
#include <windows.h>
void printHelp() {
    HANDLE output_handle = ::GetStdHandle(STD_OUTPUT_HANDLE);
    if (output_handle != INVALID_HANDLE_VALUE) {
        SMALL_RECT rect = {};
        rect.Bottom = static_cast<SHORT>(100);
        rect.Right = static_cast<SHORT>(80);
        SetConsoleWindowInfo(output_handle, TRUE, &rect);

        COORD coord = {};
        coord.X = 100;
        coord.Y = 80;
        
        SetConsoleScreenBufferSize(output_handle, coord);
    }

    HWND console = GetConsoleWindow();
    RECT r;
    GetWindowRect(console, &r); //stores the console's current dimensions

    MoveWindow(console, r.left, r.top, 1000, 800, TRUE); // 800 width, 100 height

    std::cout
    << "Hewo! I'm the MipMap tool and I'll Map Mips for you.\n\n"

    << "I can unpack your textures like so:\n"
    << "  mipmaptool unpack -namestyle0 \"P:/file1_co.paa\" \"P:\\file2_co.paa\"\n"
    << "I can print info about your texture like so:\n"
    << "  mipmaptool info \"P:/file1_co.paa\"\n"
    << "I can combine the best mipmaps like so:\n"
    << "  mipmaptool -namestyle0 \"P:/tex_mip4096_co.paa\" \"P:\\tex_mip2048_co.paa\" \"P:/tex_mip4_co.paa\"\n"
    << " These filenames have to be in a specific format, see -namestyle below\n"
    << "Or I also can combine the best mipmaps like so:\n"
    << "  mipmaptool output \"P:/output_co.paa\" \"P:/file1_co.paa\" \"P:\\file2_co.paa\"\n"
    << " (which writes into specified output file)\n\n"

    << "-namestyle parameter sets he filename format for input/outputs\n"
    << "-namestyle0 (default):\n"
    << " xxx_mip1234_yy.paa, output file will be xxx_yy.paa\n"
    << " (for you nerds out there the regex is (.*)_mip(\\d*)(([^\\d]*)*)\\.paa )\n"
    << "-namestyle1 (default):\n"
    << " xxx.mip1234.paa, output file will be xxx.paa\n"
    << " regex: (.*)\\.mip(\\d*)\\.paa\n\n\n"


    << "File paths have to always be encased in quotes like shown.\n"
    << "Windows does that automatically if you just drag&drop files onto the binary.\n"
    << "As you can see / and \\ are allowed, anything that works in windows should work.\n\n"

    << "I hav been made by dedmen who has a patreon UwU: https://patreon.com/dedmen\n(Just so you know)";
    std::cin.get();
}

enum class NameStyle {
    xx_mip_yy_paa,
    xx_mip_paa,
};

std::filesystem::path makeOutputPath(std::filesystem::path inputFile, std::shared_ptr<MipMap> mipmap, NameStyle style = NameStyle::xx_mip_yy_paa) {
    std::filesystem::path outputPath = (inputFile.parent_path() / (inputFile.filename().stem().string() + "_mip" + std::to_string(mipmap->getRealSize()) + inputFile.extension().string()));
    switch (style) {
        case NameStyle::xx_mip_yy_paa: {
            std::regex reg("(.*)_(.*?)\\.paa");
            std::cmatch cm;
            std::string inputString = inputFile.filename().string();
            std::regex_match(inputString.c_str(), cm, reg);
            if (cm.size() == 3) {
                outputPath = (inputFile.parent_path() / (std::string(cm[1]) + "_mip" + std::to_string(mipmap->getRealSize()) + "_" + std::string(cm[2]) + inputFile.extension().string()));
            }

            return outputPath;
        }
        case NameStyle::xx_mip_paa: {
            std::regex reg("(.*)\\.paa");
            std::cmatch cm;
            std::string inputString = inputFile.filename().string();
            std::regex_match(inputString.c_str(), cm, reg);
            if (cm.size() == 2) {
                outputPath = (inputFile.parent_path() / (std::string(cm[1]) + ".mip" + std::to_string(mipmap->getRealSize()) + inputFile.extension().string()));
            }

            return outputPath;
        }
        default: {
            std::cerr << "ERROR invalid -namestyle parameter provided";
            std::cin.get();
            exit(1);
        }
    }
}

std::optional<std::string> parseOutputFilename(std::filesystem::path inputFile, NameStyle style = NameStyle::xx_mip_yy_paa) {
    auto filename = inputFile.filename().string();

    switch (style) {
        case NameStyle::xx_mip_yy_paa: {
            std::regex reg("(.*)_mip(\\d*)([^\\d]*)\\.paa");
            std::cmatch cm;
            std::regex_match(filename.c_str(), cm, reg);
            if (cm.size() != 4) {
                return {};
            }
            return (std::string(cm[1]) + std::string(cm[3]) + ".paa");
        }
        case NameStyle::xx_mip_paa: {
            std::regex reg("(.*)\\.mip(\\d*)\\.paa");
            std::cmatch cm;
            std::regex_match(filename.c_str(), cm, reg);
            if (cm.size() != 3) {
                return {};
            }
            return std::string(cm[1]) + ".paa";
        }
        default: {
            std::cerr << "ERROR invalid -namestyle parameter provided";
            std::cin.get();
            exit(1);
        }
    }
}

int main(int argc, char* argv[]) {
    printStart();
    if (argc <= 1) {
        printHelp();
        return 0;
    }

    std::string_view firstArg = argv[1];

    if (firstArg == "help") {
        printHelp();
        return 0;
    }

    std::vector<std::string> allArguments;
    for (int i = 1; i < argc; ++i) {
        allArguments.push_back(argv[i]);
    }

    NameStyle namestyle = NameStyle::xx_mip_yy_paa;

    auto namestyleArg = std::find_if(allArguments.begin(), allArguments.end(), [](const std::string& arg) {
        return arg.starts_with("-namestyle");
    });

    if (namestyleArg != allArguments.end()) {
        char namestyleChar = (*namestyleArg)[10];
        namestyle = static_cast<NameStyle>(namestyleChar - '0');

        allArguments.erase(namestyleArg);
    }

    auto highestMIPArg = std::find_if(allArguments.begin(), allArguments.end(), [](const std::string& arg) {
        return arg.starts_with("-onlyHighestMip");
    });


    if (firstArg == "unpack") {
        std::cout << "Unpacking files:\n";
        allArguments.erase(allArguments.begin());
        for (auto& file : allArguments) {
            auto texFile = std::make_shared<TextureFile>();
            std::filesystem::path inputFile(file);//#TODO file exists check
            texFile->readFromFile(inputFile);
            //texFile->writeToFile("P:/outtest.paa");

            for (auto& mipmap : texFile->mipmaps) {
                auto newFile = texFile->copyNoMipmap();
                newFile->mipmaps.push_back(mipmap);

                std::filesystem::path outputPath = makeOutputPath(inputFile, mipmap, namestyle);

                newFile->writeToFile(outputPath);
                if (highestMIPArg != allArguments.end()) return 0;
            }
        }
        return 0;
    }

    if (firstArg == "info") {
        std::cout << "file info:\n";
        allArguments.erase(allArguments.begin());
        for (auto& file : allArguments) {
            auto texFile = std::make_shared<TextureFile>();
            std::filesystem::path inputFile(file);//#TODO file exists check
            texFile->verboseLog = true;
            texFile->readFromFile(inputFile);
        }
        return 0;
    }

    std::filesystem::path output;
    bool hasOutput = false;

    if (firstArg == "output") {
        allArguments.erase(allArguments.begin());
        output = allArguments.front();
        allArguments.erase(allArguments.begin());
        hasOutput = true;
    }

    if (!hasOutput) {

        for (auto& file : allArguments) {
            std::filesystem::path inputFile(file);
            auto filename = inputFile.filename().string();

            auto outputFilename = parseOutputFilename(inputFile, namestyle);

            if (!outputFilename) {
                std::cerr << "ERROR filename " << filename << " doesn't match correct format";
                std::cin.get();
                return 1;
            }

            if (!hasOutput) {
                output = inputFile.parent_path() / *outputFilename;
                hasOutput = true;
            } else {
                if (*outputFilename != output.filename()) {
                    std::cerr << "ERROR filename " << filename << " doesn't match previously determined filename " << output.filename();
                    std::cin.get();
                    return 1;
                }
            }
        }
    }

    std::vector<std::shared_ptr<TextureFile>> textures;
    for (auto& file : allArguments) {
        auto texFile = std::make_shared<TextureFile>();
        std::filesystem::path inputFile(file);
        texFile->readFromFile(inputFile);
        textures.push_back(texFile);
    }

    //Sort by their max mipmap size
    std::sort(textures.begin(), textures.end(), [](auto& left, auto& right) {
        return left->mipmaps.front()->width > right->mipmaps.front()->width;
    });

    std::map<uint16_t, std::shared_ptr<MipMap>> mipmaps;

    for (auto& texture : textures) {
        for (auto& mipmap : texture->mipmaps) {
                mipmaps.insert_or_assign(mipmap->height, mipmap);
        }
    }

    auto result = textures.back()->copyNoMipmap();

    std::cout << "packing mipmaps...\n";

    for (auto& [key, val] : mipmaps) {
        std::cout << "\t" << val->height << "-> \t" << val->inputPath << "\n";

        result->mipmaps.push_back(val);
    }

    if (hasOutput) {
        result->writeToFile(output);
    }
    std::cout << "packed new texture file with " << result->mipmaps.size() << " mipmaps in " << output << "\n";

    return 0;
}
