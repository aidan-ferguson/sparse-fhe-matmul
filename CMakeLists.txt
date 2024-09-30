cmake_minimum_required(VERSION 3.10)
project(SparseFHEMatmul)

# Library version
set(VERSION_MAJOR 1)
set(VERSION_MINOR 0)
set(VERSION_PATCH 0)

# Require SEAL install
find_package(SEAL 4.1.1 REQUIRED)

# Configure version file for CMake package
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/ConfigVersion.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/SparseFHEMatmulConfigVersion.cmake
    @ONLY
)

file(GLOB SRC
    "${PROJECT_SOURCE_DIR}/src/fhe_sparse_matmul/*.cpp"
    "${PROJECT_SOURCE_DIR}/src/fhe_sparse_matmul/Matrix/*.cpp"
)

# Target and include
add_library(SparseFHEMatmul SHARED ${SRC})

target_include_directories(SparseFHEMatmul
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

set_target_properties(SparseFHEMatmul PROPERTIES
    VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}
    SOVERSION ${VERSION_MAJOR}
)

# Link SEAL 
target_link_libraries(SparseFHEMatmul PUBLIC SEAL::seal)

# Install stuff
install(TARGETS SparseFHEMatmul
    EXPORT SparseFHEMatmulTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)
install(DIRECTORY include/ DESTINATION include)
install(EXPORT SparseFHEMatmulTargets
    FILE SparseFHEMatmulTargets.cmake
    NAMESPACE SparseFHEMatmul::
    DESTINATION lib/cmake/SparseFHEMatmul
)

# Configure package file
include(CMakePackageConfigHelpers)

configure_package_config_file(
    cmake/SparseFHEMatmulConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/SparseFHEMatmulConfig.cmake
    INSTALL_DESTINATION lib/cmake/SparseFHEMatmul
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/SparseFHEMatmulConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/SparseFHEMatmulConfigVersion.cmake
    DESTINATION lib/cmake/SparseFHEMatmul
)
