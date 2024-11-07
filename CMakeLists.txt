cmake_minimum_required(VERSION 3.10)
project(fhe_sparse_matmul)

# Library version
set(VERSION_MAJOR 1)
set(VERSION_MINOR 0)
set(VERSION_PATCH 0)

option(PYTHON_BINDINGS "Build Python bindings" ON)

# Require SEAL install
find_package(SEAL 4.1.1 REQUIRED)

# Configure version file for CMake package
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/ConfigVersion.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}ConfigVersion.cmake
    @ONLY
)

file(GLOB SRC
    "${PROJECT_SOURCE_DIR}/src/fhe_sparse_matmul/*.cpp"
    "${PROJECT_SOURCE_DIR}/src/fhe_sparse_matmul/Matrix/*.cpp"
)

# Target and include
# If we want to build python bindings
if (PYTHON_BINDINGS)
    set(PYBIND11_FINDPYTHON ON)
    find_package(pybind11 CONFIG REQUIRED)
    pybind11_add_module(${CMAKE_PROJECT_NAME} SHARED ${SRC})
    add_compile_definitions(BUILD_PYTHON_BINDINGS)
else()
    add_library(${CMAKE_PROJECT_NAME} SHARED ${SRC})
endif()

# Set compiler flags for warnings
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE -Wpedantic -Wall -Wextra -Werror)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE /W4)  # /W4 is roughly equivalent to -Wall -Wextra
endif()

target_include_directories(${CMAKE_PROJECT_NAME}
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES
    VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}
    SOVERSION ${VERSION_MAJOR}
)

# Link SEAL 
target_link_libraries(${CMAKE_PROJECT_NAME} PUBLIC SEAL::seal)

# Install stuff
install(TARGETS ${CMAKE_PROJECT_NAME}
    EXPORT ${CMAKE_PROJECT_NAME}Targets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)
install(DIRECTORY include/ DESTINATION include)
install(EXPORT ${CMAKE_PROJECT_NAME}Targets
    FILE ${CMAKE_PROJECT_NAME}Targets.cmake
    NAMESPACE ${CMAKE_PROJECT_NAME}::
    DESTINATION lib/cmake/${CMAKE_PROJECT_NAME}
)

# Configure package file
include(CMakePackageConfigHelpers)

configure_package_config_file(
    cmake/${CMAKE_PROJECT_NAME}Config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}Config.cmake
    INSTALL_DESTINATION lib/cmake/${CMAKE_PROJECT_NAME}
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}Config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}ConfigVersion.cmake
    DESTINATION lib/cmake/${CMAKE_PROJECT_NAME}
)
