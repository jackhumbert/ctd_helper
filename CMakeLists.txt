cmake_minimum_required(VERSION 3.24)

include(CMakeDependentOption)

project(
  ctd_helper
  VERSION 0.1.0
  LANGUAGES CXX
)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# user-configurable options
set(CYBERPUNK_2077_GAME_DIR "C:/Program Files (x86)/Steam/steamapps/common/Cyberpunk 2077" CACHE STRING "Cyberpunk 2077 game directory")

set(MOD_GAME_DIR "${PROJECT_SOURCE_DIR}/game_dir")
set(MOD_TOOLS_DIR "${PROJECT_SOURCE_DIR}/tools")

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

add_compile_definitions(
  # Support Windows 7 and above.
  WINVER=0x0601
  _WIN32_WINNT=0x0601

  # Exclude unnecessary APIs.
  WIN32_LEAN_AND_MEAN

  # Use Unicode charset.
  UNICODE
  _UNICODE

  # https://github.com/microsoft/STL/issues/1934
  _ITERATOR_DEBUG_LEVEL=0

  # for Codeware
  NOMINMAX
)

# Dependencies

include(ConfigureAndIncludeDetours)
include(ConfigureAndIncludeRED4extSdk)
include(ConfigureAndIncludeSpdlog)

# Find Addresses

set(ZOLTAN_USER_SIGNATURES "${PROJECT_SOURCE_DIR}/src/red4ext/Main.cpp")
set(ZOLTAN_USER_ADDRESSES "${PROJECT_SOURCE_DIR}/src/red4ext/Addresses.hpp")

add_custom_command(
  OUTPUT ${ZOLTAN_USER_ADDRESSES}
  DEPENDS ${ZOLTAN_USER_SIGNATURES}
  COMMAND ${MOD_TOOLS_DIR}/zoltan-clang.exe
  ARGS ${ZOLTAN_USER_SIGNATURES} "${CYBERPUNK_2077_GAME_DIR}/bin/x64/Cyberpunk2077.exe" -f "std=c++20" -f "I${PROJECT_SOURCE_DIR}/deps/red4ext.sdk/include" --c-output "${ZOLTAN_USER_ADDRESSES}"
  COMMENT "Finding binary addresses of declared functions in Signatures.hpp"
)

add_custom_target(addresses DEPENDS ${ZOLTAN_USER_ADDRESSES})

# DLL

add_subdirectory(src/red4ext)

# License

set(MOD_SOURCE_LICENSE_FILE "${PROJECT_SOURCE_DIR}/license.md")
set(MOD_GAME_DIR_LICENSE_FILE "${MOD_GAME_DIR}/red4ext/plugins/${CMAKE_PROJECT_NAME}/license.md")

add_custom_command(
  OUTPUT ${MOD_GAME_DIR_LICENSE_FILE}
  DEPENDS ${MOD_SOURCE_LICENSE_FILE}
  COMMAND ${CMAKE_COMMAND} -E copy_if_different ${MOD_SOURCE_LICENSE_FILE} ${MOD_GAME_DIR_LICENSE_FILE}
)

add_custom_target(license DEPENDS ${MOD_GAME_DIR_LICENSE_FILE})

# Readme

set(MOD_SOURCE_README_FILE "${PROJECT_SOURCE_DIR}/readme.md")
set(MOD_GAME_DIR_README_FILE "${MOD_GAME_DIR}/red4ext/plugins/${CMAKE_PROJECT_NAME}/readme.md")

add_custom_command(
  OUTPUT ${MOD_GAME_DIR_README_FILE}
  DEPENDS ${MOD_SOURCE_README_FILE}
  COMMAND ${CMAKE_COMMAND} -E copy_if_different ${MOD_SOURCE_README_FILE} ${MOD_GAME_DIR_README_FILE}
)

add_custom_target(readme DEPENDS ${MOD_GAME_DIR_README_FILE})

# Full mod

add_custom_target(${CMAKE_PROJECT_NAME}
  DEPENDS ${CMAKE_PROJECT_NAME}.dll readme license
  )

# Install

file(GLOB MOD_GAME_DIR_FILES ${MOD_GAME_DIR}/*)

install(DIRECTORY ${MOD_GAME_DIR_FILES}
  DESTINATION ${CYBERPUNK_2077_GAME_DIR})

# Release

set(MOD_ZIP_FILE ${CMAKE_SOURCE_DIR}/${CMAKE_PROJECT_NAME}_v${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}.zip)

add_custom_target(release
  COMMAND ${CMAKE_COMMAND} -E tar "cfv" "${MOD_ZIP_FILE}" --format=zip .
  WORKING_DIRECTORY ${MOD_GAME_DIR}
  DEPENDS ${CMAKE_PROJECT_NAME}
  COMMENT "Zipping game_dir for v${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")