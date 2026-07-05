cmake_minimum_required(VERSION 3.16)

project(DynamicOpsDemo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set(CMAKE_SHARED_LIBRARY_SUFFIX ".so")

add_library(dynamic_ops_registry SHARED
    csrc/registry.cpp
)

target_include_directories(dynamic_ops_registry
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/csrc"
)

target_link_libraries(dynamic_ops_registry
    PRIVATE
        ${CMAKE_DL_LIBS}
)

add_library(custom_ops SHARED
    extension/custom_ops.cpp
)

target_include_directories(custom_ops
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/csrc"
)

target_link_libraries(custom_ops
    PRIVATE
        dynamic_ops_registry
)

set_target_properties(custom_ops PROPERTIES
    BUILD_RPATH "$ORIGIN;@loader_path"
)
