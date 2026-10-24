# It's recommended to set a minimum CMake version.
# If you use CMake features from higher versions, update this to match.
cmake_minimum_required(VERSION 3.21)

if(NOT DEFINED ENV{COMMONLIB_SSE_FOLDER})
  message(FATAL_ERROR "Missing COMMONLIB_SSE_FOLDER environment variable")
endif()

include(cmake/lib/copyOutputs.cmake)
include(cmake/lib/automaticGameFolderOutput.cmake)
include(cmake/lib/autoCollectSources.cmake)

add_src_folder(src)
add_include_folder(include)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

# Set your project name. This will be the name of your SKSE .dll file.
set(AUTHOR_NAME "MoonPhasesHUD")
set(PRODUCT_NAME "MoonPhaseHUD")
project(${PRODUCT_NAME} VERSION 1.0.0.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)

configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/version.rc.in
        ${CMAKE_CURRENT_BINARY_DIR}/version.rc
        @ONLY)

set(RESOURCE_FILE ${CMAKE_CURRENT_BINARY_DIR}/version.rc)
set(RESOURCE_OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/version.res)

add_custom_command(
    OUTPUT ${RESOURCE_OUTPUT}
    COMMAND rc /fo ${RESOURCE_OUTPUT} ${RESOURCE_FILE}
    DEPENDS ${RESOURCE_FILE}
)

# Add a custom target to ensure the resource file is generated
add_custom_target(Resource ALL DEPENDS ${RESOURCE_OUTPUT})

set(ENABLE_SKYRIM_SE ON CACHE BOOL " " FORCE)
set(ENABLE_SKYRIM_AE ON CACHE BOOL " " FORCE)
set(ENABLE_SKYRIM_VR ON CACHE BOOL " " FORCE)
set(BUILD_TESTS OFF CACHE BOOL " " FORCE)

add_subdirectory("$ENV{COMMONLIB_SSE_FOLDER}" ${CMAKE_CURRENT_BINARY_DIR}/_deps/clib-build)
# Don't read SOURCE_DIR from the CommonLibSSE target: it is an IMPORTED target (no SOURCE_DIR)
# when CommonLibSSE-NG resolves a prebuilt bundle (COMMONLIB_PREBUILT / clean release tag in CI).
set(COMMONLIB_SRC_DIR "$ENV{COMMONLIB_SSE_FOLDER}")

find_path(CLIB_UTIL_INCLUDE_DIRS "ClibUtil/utils.hpp")
find_path(CLIB_UTILS_QTR_INCLUDE_DIRS "ClibUtilsQTR/utils.hpp")
find_library(DETOURS_LIBRARY detours REQUIRED)
find_path(SKSE_MCP_INCLUDE_DIRS "SKSEMCP/utils.hpp")


file(
  GLOB_RECURSE
  sources
  ${CMAKE_CURRENT_BINARY_DIR}/version.rc
  ${sources}
)

source_group(
  TREE ${CMAKE_CURRENT_SOURCE_DIR}
  FILES ${headers} ${sources}
)

include(${COMMONLIB_SRC_DIR}/cmake/CommonLibSSE.cmake)

add_commonlibsse_plugin(
  ${PROJECT_NAME}
  AUTHOR "${AUTHOR_NAME}"
  USE_ADDRESS_LIBRARY # Address Library based: covers SE 1.5.97, AE 1.6.x and AE 1.7.x (incl. 1.7.104)
  SOURCES ${headers} ${sources}
)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23) # <--- use C++23 standard
target_precompile_headers(${PROJECT_NAME} PRIVATE include/PCH.h) # <--- PCH.h is required!
target_include_directories(
        ${PROJECT_NAME}
        PRIVATE
        ${CLIB_UTIL_INCLUDE_DIRS}
        ${CLIB_UTILS_QTR_INCLUDE_DIRS}
    ${SKSE_MCP_INCLUDE_DIRS}
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(${PROJECT_NAME} PRIVATE ${DETOURS_LIBRARY})

target_compile_definitions(${PROJECT_NAME} PRIVATE IS_HOST_PLUGIN)

set(wildlander_output false)
set(steam_owrt_output false)
set(steam_mods_output true)
set(lorerim_mods_output false)
set(lorerim3_mods_output true)
automaticGameFolderOutput(wildlander_output, steam_owrt_output, steam_mods_output lorerim_mods_output lorerim3_mods_output)
