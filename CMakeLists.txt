cmake_minimum_required(VERSION 3.11)
project(mc-pricer CXX)

# Prefix
if (NOT PREFIX)
    set(PREFIX ${CMAKE_CURRENT_BINARY_DIR})
endif (NOT PREFIX)
set(CMAKE_INSTALL_PREFIX ${PREFIX} CACHE STRING "Installation prefix." FORCE)

if (CXX)
    set(CMAKE_CXX_COMPILER ${CXX})
endif (CXX)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})

# Release or Debug
if (NOT CMAKE_BUILD_TYPE)
    message(STATUS "Setting build type to 'Debug' as none was specified.")
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)
endif ()

# Detect PNL
find_package(Pnl REQUIRED)
set(LIBS ${LIBS} ${PNL_LIBRARIES})
include_directories(${PNL_INCLUDE_DIRS})
if(${CMAKE_BUILD_TYPE} STREQUAL "Release")
    add_definitions(-DPNL_RANGE_CHECK_OFF)
endif()

# Find nlohmann_json to parse json files
# Add nlohmann_json::nlohmann_json to target_link_libraries
find_package(nlohmann_json)
if (NOT nlohmann_json_FOUND)
    if (${CMAKE_VERSION} VERSION_GREATER "3.24.0")
        CMAKE_POLICY(SET CMP0135 NEW)
    endif()
    include(FetchContent)
    FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.tar.xz)
    FetchContent_MakeAvailable(json)
endif()

### Ne rien modifier dans la partie au dessus de cette ligne.

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# Core library: every source file of the pricer is listed here, once.
#   io/       json and market reading, construction of the objects
#   model/    Black-Scholes model
#   options/  payoffs
#   pricing/  Monte Carlo price and delta
#   hedging/  profit and loss of the delta hedging
# utils/ only contains headers (time_grid.hpp).
# ---------------------------------------------------------------------------
add_library(pricer_core STATIC
    src/io/json_helper.cpp
    src/io/pricer_setup.cpp
    src/model/BlackScholesModel.cpp
    src/options/Option.cpp
    src/options/BasketOption.cpp
    src/options/AsianOption.cpp
    src/options/PerformanceOption.cpp
    src/pricing/MonteCarlo.cpp
    src/hedging/Hedging.cpp)

# Headers are included with their folder: #include "model/BlackScholesModel.hpp"
target_include_directories(pricer_core PUBLIC ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(pricer_core PUBLIC ${LIBS} nlohmann_json::nlohmann_json)

# ---------------------------------------------------------------------------
# Executables: each one only needs its main file and the core library.
# ---------------------------------------------------------------------------

# Test the parser (file provided with the project, left untouched)
add_executable(test_json_reader src/io/json_helper.cpp tests/test_json_reader.cpp)
target_include_directories(test_json_reader PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/src/io)
target_link_libraries(test_json_reader
    ${PNL_LIBRARIES}
    nlohmann_json::nlohmann_json
)
pnl_add_postbuild(test_json_reader)

# Price at time 0
add_executable(test_price0 tests/test_price0.cpp)
target_link_libraries(test_price0 pricer_core)

# Price and delta at a time t read on a market trajectory
add_executable(test_pricet tests/test_pricet.cpp)
target_link_libraries(test_pricet pricer_core)

# Profit and loss of the delta hedging along a market trajectory
add_executable(test_hedge tests/test_hedge.cpp)
target_link_libraries(test_hedge pricer_core)

# Check of the market simulation under the historical probability
add_executable(test_simul_market tests/test_simul_market.cpp)
target_link_libraries(test_simul_market pricer_core)


# Print compiler flags
get_directory_property(DirDefs COMPILE_DEFINITIONS)
message(STATUS "COMPILE_DEFINITIONS = ${DirDefs}")
if (${CMAKE_BUILD_TYPE} STREQUAL "Release")
    message(STATUS "C++ Flags: ${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_RELEASE} ${CMAKE_LINKER_EXE_FLAGS}" )
else()
    message(STATUS "C++ Flags: ${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_DEBUG} ${CMAKE_LINKER_EXE_FLAGS}" )
endif()