# CMakeList.txt : CMake project for ModernEmuCrossPlat, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

# Add source to this project's executable.
file(GLOB TARGET_SRC_FILES headers/*.h src/*.cpp)
add_executable (ModernEmuCrossPlat ${TARGET_SRC_FILES})

include_directories(headers)
target_precompile_headers(ModernEmuCrossPlat PRIVATE headers/pch.h)

find_package(jsoncpp CONFIG REQUIRED)
target_link_libraries(ModernEmuCrossPlat PRIVATE jsoncpp_lib)


# TODO: Add tests and install targets if needed.
