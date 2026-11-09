cmake_minimum_required(VERSION 3.18)
project(quant_engine VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)   # required for the Python extension (.so)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# When built through pip (scikit-build-core) SKBUILD is defined:
# build the Python module and skip tests by default. A plain CMake build does the opposite.
if(DEFINED SKBUILD)
    set(_default_tests OFF)
    set(_default_python ON)
else()
    set(_default_tests ON)
    set(_default_python OFF)
endif()
option(ENGINE_BUILD_TESTS  "Build GoogleTest unit tests" ${_default_tests})
option(ENGINE_BUILD_PYTHON "Build pybind11 Python module" ${_default_python})

# ---------------- Core library ----------------
add_library(quant_core STATIC
    src/pricing.cpp
    src/stats.cpp
    src/backtest.cpp
    src/simulate.cpp
)
target_include_directories(quant_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(quant_core PRIVATE -Wall -Wextra -Wpedantic -Wconversion)
endif()

# ---------------- Unit tests ----------------
if(ENGINE_BUILD_TESTS)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        v1.15.2
        GIT_SHALLOW    TRUE
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    enable_testing()
    add_executable(engine_tests
        tests/test_pricing.cpp
        tests/test_stats.cpp
        tests/test_backtest.cpp
        tests/test_simulate.cpp
    )
    target_link_libraries(engine_tests PRIVATE quant_core GTest::gtest_main)
    include(GoogleTest)
    gtest_discover_tests(engine_tests)
endif()

# ---------------- Python bindings ----------------
if(ENGINE_BUILD_PYTHON)
    find_package(Python 3.10 REQUIRED COMPONENTS Interpreter Development.Module)
    find_package(pybind11 CONFIG REQUIRED)
    pybind11_add_module(quant_engine bindings/module.cpp)
    target_link_libraries(quant_engine PRIVATE quant_core)
    install(TARGETS quant_engine LIBRARY DESTINATION .)
endif()
