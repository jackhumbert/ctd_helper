add_library(${CMAKE_PROJECT_NAME}.dll SHARED)

set_target_properties(${CMAKE_PROJECT_NAME}.dll PROPERTIES OUTPUT_NAME ${CMAKE_PROJECT_NAME})
set_target_properties(${CMAKE_PROJECT_NAME}.dll PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
set_target_properties(${CMAKE_PROJECT_NAME}.dll PROPERTIES ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

configure_file(Version.hpp.in Version.hpp @ONLY)

file(GLOB_RECURSE HEADER_FILES *.hpp)
file(GLOB_RECURSE SOURCE_FILES *.cpp)
file(GLOB_RECURSE RC_FILES *.rc)

source_group(_CMake REGULAR_EXPRESSION cmake_pch.*)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${HEADER_FILES} ${SOURCE_FILES} ${RC_FILES})

target_include_directories(${CMAKE_PROJECT_NAME}.dll PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_sources(${CMAKE_PROJECT_NAME}.dll PRIVATE ${HEADER_FILES} ${SOURCE_FILES} ${RC_FILES})

target_precompile_headers(${CMAKE_PROJECT_NAME}.dll PUBLIC stdafx.hpp)
target_link_libraries(${CMAKE_PROJECT_NAME}.dll
  PUBLIC
    Detours
    RED4ext.SDK
    spdlog
)

add_dependencies(${CMAKE_PROJECT_NAME}.dll addresses)

add_custom_command(
  TARGET ${CMAKE_PROJECT_NAME}.dll POST_BUILD
  DEPENDS ${CMAKE_PROJECT_NAME}.dll
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
  ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}.dll
  ${MOD_GAME_DIR}/red4ext/plugins/${CMAKE_PROJECT_NAME}/${CMAKE_PROJECT_NAME}.dll
  COMMENT "${CMAKE_PROJECT_NAME}.dll -> ${MOD_GAME_DIR}/red4ext/plugins/${CMAKE_PROJECT_NAME}/${CMAKE_PROJECT_NAME}.dll")