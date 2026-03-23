cmake_minimum_required(VERSION 3.28)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

project(ArrowVortex)

set(CMAKE_CONFIGURATION_TYPES Debug;Release;MinSizeRel)
# clang-tidy might not be available on AppleClang systems by default
find_program(CLANG_TIDY_EXE NAMES clang-tidy)
if(CLANG_TIDY_EXE)
    if("${CMAKE_BUILD_TYPE}" STREQUAL "MinSizeRel")
        set(tidy_flags --fix-errors --fix-notes)
        set(format_flags -i)
    else()
        set(tidy_flags --warnings-as-errors=*)
        set(format_flags --dry-run)
    endif()
    set(CMAKE_CXX_CLANG_TIDY clang-tidy)
    list(APPEND CMAKE_CXX_CLANG_TIDY ${tidy_flags})
    if(WIN32)
        list(APPEND CMAKE_CXX_CLANG_TIDY -extra-arg=/EHsc --exclude-header-filter=SDL3*)
    endif()
    list(APPEND CMAKE_CXX_CLANG_TIDY -p=build)
    message(STATUS "clang-tidy found: ${CLANG_TIDY_EXE}")
else()
    message(STATUS "clang-tidy not found, skipping static analysis")
endif()

# Sometimes caching will cause CMAKE_TOOLCHAIN_FILE, build presets, and includes to not update/work, so include fallbacks by hand.
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE OR "${CMAKE_TOOLCHAIN_FILE}" STREQUAL "")
    if(DEFINED ENV{VCPKG_ROOT} AND NOT "$ENV{VCPKG_ROOT}" STREQUAL "")
        message(WARNING "CMAKE_TOOLCHAIN_FILE is not defined, manually including.")
        include("$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
    elseif(NOT "${VCPKG_ROOT}" STREQUAL "")
        message(WARNING "CMAKE_TOOLCHAIN_FILE is not defined, manually including.")
        include("${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
    elseif(UNIX AND NOT APPLE)
        message(WARNING "{VCPKG_ROOT} is not defined, attempting fallback generation.")
        include("/opt/vcpkg/scripts/buildsystems/vcpkg.cmake")
    elseif(WIN32)
        message(WARNING "%VCPKG_ROOT% is not defined, attempting fallback generation.")
        include("C:/Program Files/Microsoft Visual Studio/18/Community/VC/vcpkg/scripts/buildsystems/vcpkg.cmake")
    endif()
endif()

if("${toolchain_include}" STREQUAL "")
    if("${YML}" STREQUAL "")
        set(toolchain_include "${PROJECT_SOURCE_DIR}/out/build/${PRESET_NAME}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/include")
    else()
        set(toolchain_include "${PROJECT_SOURCE_DIR}/build/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/include")
    endif()
endif()

message(STATUS "Including toolchain folder: ${toolchain_include}") 
include_directories("${toolchain_include}")
include_directories("${PROJECT_SOURCE_DIR}/src")

find_package(PkgConfig REQUIRED)

# Homebrew provides some libraries that are not available through vcpkg on macOS.
if(APPLE)
    if(CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64")
        set(HOMEBREW_PREFIX "/opt/homebrew")
    else()
        set(HOMEBREW_PREFIX "/usr/local")
    endif()
    set(ENV{PKG_CONFIG_PATH} "${HOMEBREW_PREFIX}/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}")
    message(STATUS "Added Homebrew PKG_CONFIG_PATH: ${HOMEBREW_PREFIX}/lib/pkgconfig")
endif()

find_package(Aubio CONFIG NAMES aubio Aubio QUIET)
if(NOT Aubio_FOUND)
    pkg_check_modules(AUBIO REQUIRED IMPORTED_TARGET aubio)
    add_library(Aubio::aubio ALIAS PkgConfig::AUBIO)
endif()

find_package(FFMPEG QUIET)
if(NOT FFMPEG_FOUND)
    pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET libavcodec libavformat libavutil libswscale libswresample)
endif()
pkg_check_modules(FFMPEG_SWRESAMPLE REQUIRED IMPORTED_TARGET libswresample)

find_package(Freetype REQUIRED)

find_package(iir CONFIG QUIET)
if(NOT iir_FOUND)
    pkg_check_modules(IIR REQUIRED IMPORTED_TARGET iir)
    if(NOT TARGET iir::iir_static)
        add_library(iir::iir_static ALIAS PkgConfig::IIR)
    endif()
endif()

find_package(MAD REQUIRED)

find_package(Ogg CONFIG QUIET)
if(NOT Ogg_FOUND)
    pkg_check_modules(OGG REQUIRED IMPORTED_TARGET ogg)
    if(NOT TARGET Ogg::ogg)
        add_library(Ogg::ogg ALIAS PkgConfig::OGG)
    endif()
endif()

find_package(OpenGL REQUIRED)
find_package(SDL3 CONFIG QUIET)
if(NOT SDL3_FOUND)
    pkg_check_modules(SDL3 REQUIRED IMPORTED_TARGET sdl3)
endif()

find_package(Stb CONFIG QUIET)
if(NOT Stb_FOUND AND NOT TARGET unofficial::stb::stb)
    find_path(STB_INCLUDE_DIR NAMES stb_image.h PATHS
        /usr/local/include
        /opt/homebrew/include
        ${CMAKE_CURRENT_SOURCE_DIR}/external
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party)
    if(STB_INCLUDE_DIR)
        add_library(stb INTERFACE)
        target_include_directories(stb INTERFACE ${STB_INCLUDE_DIR})
        add_library(unofficial::stb::stb ALIAS stb)
    else()
        message(FATAL_ERROR "Could not find stb headers")
    endif()
endif()

find_package(Vorbis CONFIG QUIET)
if(NOT Vorbis_FOUND)
    pkg_check_modules(VORBIS REQUIRED IMPORTED_TARGET vorbis vorbisfile)
    add_library(Vorbis::vorbis ALIAS PkgConfig::VORBIS)
    add_library(Vorbis::vorbisfile ALIAS PkgConfig::VORBIS)
endif()

add_subdirectory(src/Core)
add_subdirectory(src/Dialogs)
add_subdirectory(src/Editor)
add_subdirectory(src/Managers)
add_subdirectory(src/Simfile)
add_subdirectory(src/System)

install(TARGETS ArrowVortex RUNTIME)
install(DIRECTORY bin/assets DESTINATION bin)
install(DIRECTORY bin/noteskins DESTINATION bin)
install(DIRECTORY bin/settings DESTINATION bin)

file(GLOB_RECURSE path_sources "src/*.cpp" "src/*.hpp" "src/*.c" "src/*.h")
# Only prevent format/building on Windows, as Linux's clang-format is slightly different
if(WIN32)
    add_custom_target(format ALL)
    add_custom_command(
        TARGET format
        POST_BUILD
        COMMAND clang-format ${format_flags} -Werror -style=file ${path_sources}
        USES_TERMINAL
    )
    add_dependencies(format ArrowVortex)
endif()
