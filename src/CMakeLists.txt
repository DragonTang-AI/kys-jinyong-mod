cmake_minimum_required(VERSION 3.5)
project(kys)

set(CMAKE_VERBOSE_MAKEFILE on)
set(CMAKE_CXX_COMPILER $ENV{CXX})
set(CMAKE_CXX_FLAGS "-O2 -std=c++23 -pthread -DNDEBUG")

# Homebrew base path on Apple Silicon
set(BREW_PREFIX /opt/homebrew)

include_directories(
    ../include
    .
    ../cifa
    ../mlcc
    ../others
    ../local/include
    ${BREW_PREFIX}/include
    ${BREW_PREFIX}/opt/sqlite/include
    ${BREW_PREFIX}/opt/libzip/include
    ${BREW_PREFIX}/opt/lua/include
)

link_directories(
    .
    ../lib
    ${BREW_PREFIX}/lib
    ${BREW_PREFIX}/opt/sqlite/lib
    ${BREW_PREFIX}/opt/lua/lib
)

file(GLOB SRC_LIST
    ./*.cpp
    ../cifa/Cifa.cpp
    ../mlcc/strfunc.cpp
    ../mlcc/filefunc.cpp
    ../mlcc/PotConv.cpp
    ../mlcc/SQLite3Wrapper.cpp
    ../mlcc/ZipFile.cpp
    ../mlcc/SimpleCC.cpp
    ../others/Hanz2Piny.cpp    
    )

set(LINK_LIBRARIES SDL3 SDL3_ttf SDL3_image SDL3_mixer lua opencc sqlite3 yaml-cpp zip)
if (APPLE)
    set(LINK_LIBRARIES ${LINK_LIBRARIES} iconv)
endif ()

add_executable(kys ${SRC_LIST})
target_link_libraries(kys ${LINK_LIBRARIES})
