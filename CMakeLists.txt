cmake_minimum_required(VERSION 3.24)

project(ctd_helper VERSION 0.1.2)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/deps/cyberpunk_cmake")
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/deps/red4ext.sdk/cmake")

include(CyberpunkMod)

configure_mod(
  NAME "CTD Helper"
  SLUG ${PROJECT_NAME}
  PREFIX CTD_HELPER
  AUTHOR "Jack Humbert"
  URL "https://github.com/jackhumbert/ctd_helper"
  LICENSE "Licensed under the MIT license. See the license.md in the root project for details."
)

find_program(ZOLTAN_CLANG_EXE NAMES zoltan-clang.exe PATHS "${MOD_TOOLS_DIR}" CACHE)
find_program(CYBERPUNK_2077_EXE NAMES Cyberpunk2077.exe PATHS "${CYBERPUNK_2077_GAME_DIR}/bin/x64" CACHE DOC "Cyberpunk2077.exe Executable File")

configure_red4ext(src/red4ext)
configure_red4ext_addresses(Main.cpp Addresses.hpp)

find_package(Detours)
find_package(Spdlog)

target_link_libraries(${MOD_SLUG}.dll
  PUBLIC
    Detours
    FMOD
    spdlog
    RedLib
    PhysX3
    input_loader.dll
)

configure_folder_file(readme.md)
configure_folder_file(license.md)

configure_uninstall()
configure_release(${MOD_SLUG}_${MOD_VERSION_STR}.zip)
configure_install()