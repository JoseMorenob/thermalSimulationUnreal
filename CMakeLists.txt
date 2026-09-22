cmake_minimum_required(VERSION 3.20)

project(ir_simulator VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Núcleo físico independiente de Unreal. El plugin enlaza su propia copia
# compilada para Win64, mientras que este objetivo permite compilar y probar
# exactamente la misma API con CMake.
add_library(ir_core STATIC
    lib/src/thermal_model.cpp
)

target_include_directories(ir_core
    PUBLIC
        lib/include
)

if(MSVC)
    target_compile_options(ir_core PRIVATE /W4)
    set_property(TARGET ir_core PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
else()
    target_compile_options(ir_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

include(CTest)
if(BUILD_TESTING)
    add_executable(ir_core_tests
        tests/test_thermal_model.cpp
    )
    target_link_libraries(ir_core_tests PRIVATE ir_core)
    add_test(NAME ir_core_tests COMMAND ir_core_tests)
endif()
