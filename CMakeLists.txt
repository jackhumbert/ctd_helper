cmake_minimum_required(VERSION 3.24)

project(ctd_helper VERSION 0.1.3)

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

function(make_includable input_file output_file)
  file(READ ${input_file} content)
  set(delim "for_c++_include")
  string(LENGTH "${content}" content_length)
  message(STATUS "${input_file}: ${content_length}")
  if (${content_length} GREATER 16380)
    string(SUBSTRING "${content}" 0 16380 content_0)
    string(SUBSTRING "${content}" 16380 16380 content_1)
    set(content_0 "R\"${delim}(${content_0})${delim}\"\n")
    set(content_1 "R\"${delim}(${content_1})${delim}\"\n")
    file(WRITE ${output_file} "${content_0}")
    file(APPEND ${output_file} "${content_1}")
  else()
    set(content "R\"${delim}(${content})${delim}\"\n")
    file(WRITE ${output_file} "${content}")
  endif()
endfunction(make_includable)

make_includable(src/red4ext/style.css src/red4ext/style.css.frag)
make_includable(src/red4ext/highlight.min.js src/red4ext/highlight.min.js.frag)
make_includable(src/red4ext/line-numbers.min.js src/red4ext/line-numbers.min.js.frag)

find_program(ZOLTAN_CLANG_EXE NAMES zoltan-clang.exe PATHS "${MOD_TOOLS_DIR}" CACHE)
find_program(CYBERPUNK_2077_EXE NAMES Cyberpunk2077.exe PATHS "${CYBERPUNK_2077_GAME_DIR}/bin/x64" CACHE DOC "Cyberpunk2077.exe Executable File")

configure_red4ext(src/red4ext)
configure_red4ext_addresses(Main.cpp Addresses.hpp)

find_package(Detours)
find_package(Spdlog)

target_link_libraries(${MOD_SLUG}.dll
  PUBLIC
    Detours
    spdlog
)

configure_folder_file(readme.md)
configure_folder_file(license.md)

configure_uninstall()
configure_release(${MOD_SLUG}_${MOD_VERSION_STR}.zip)
configure_install()