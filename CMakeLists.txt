cmake_minimum_required(VERSION 3.13)
include(CMakeDependentOption)

# IMPORTANT! This CMakeList is only to include TextureFile as a library

project(MipMapToolLib CXX C)


set(MMT_HEADERS TextureFile.hpp)
set(MMT_SOURCES TextureFile.cpp)


add_library(MipMapToolLib ${MMT_SOURCES} ${MMT_HEADERS})
add_library(MipMapToolLib::MipMapToolLib ALIAS MipMapToolLib)

target_compile_features(MipMapToolLib INTERFACE cxx_std_17)




