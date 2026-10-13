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

# Test the parser
add_executable(test_json_reader src/json_helper.cpp src/test_json_reader.cpp)
target_link_libraries(test_json_reader
    ${PNL_LIBRARIES}
    nlohmann_json::nlohmann_json
)
pnl_add_postbuild(test_json_reader)

add_executable(test_price0
    src/test_price0.cpp
    src/json_helper.cpp
    src/BlackScholesModel.cpp
    src/Option.cpp
    src/BasketOption.cpp
    src/AsianOption.cpp
    src/PerformanceOption.cpp
    src/MonteCarlo.cpp)
target_link_libraries(test_price0 ${LIBS} nlohmann_json::nlohmann_json)

add_executable(test_pricet
    src/test_pricet.cpp
    src/pricer_setup.cpp
    src/json_helper.cpp
    src/BlackScholesModel.cpp
    src/Option.cpp
    src/BasketOption.cpp
    src/AsianOption.cpp
    src/PerformanceOption.cpp
    src/MonteCarlo.cpp)
target_link_libraries(test_pricet ${LIBS} nlohmann_json::nlohmann_json)

add_executable(test_hedge
    src/test_hedge.cpp
    src/Hedging.cpp
    src/pricer_setup.cpp
    src/json_helper.cpp
    src/BlackScholesModel.cpp
    src/Option.cpp
    src/BasketOption.cpp
    src/AsianOption.cpp
    src/PerformanceOption.cpp
    src/MonteCarlo.cpp)
target_link_libraries(test_hedge ${LIBS} nlohmann_json::nlohmann_json)

add_executable(test_simul_market
    src/test_simul_market.cpp
    src/pricer_setup.cpp
    src/json_helper.cpp
    src/BlackScholesModel.cpp
    src/Option.cpp
    src/BasketOption.cpp
    src/AsianOption.cpp
    src/PerformanceOption.cpp
    src/MonteCarlo.cpp)
target_link_libraries(test_simul_market ${LIBS} nlohmann_json::nlohmann_json)

# Print compiler flags
get_directory_property(DirDefs COMPILE_DEFINITIONS)
message(STATUS "COMPILE_DEFINITIONS = ${DirDefs}")
if (${CMAKE_BUILD_TYPE} STREQUAL "Release")
    message(STATUS "C++ Flags: ${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_RELEASE} ${CMAKE_LINKER_EXE_FLAGS}" )
else()
    message(STATUS "C++ Flags: ${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_DEBUG} ${CMAKE_LINKER_EXE_FLAGS}" )
endif()