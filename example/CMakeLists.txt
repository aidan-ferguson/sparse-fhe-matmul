cmake_minimum_required(VERSION 3.13)

project(SparseExample VERSION 1.0.0 LANGUAGES CXX)

# find_package(SEAL 4.1.1 REQUIRED)
find_package(SparseFHEMatmul REQUIRED)

add_executable(ProfileMatMul main.cpp)

target_link_libraries(ProfileMatMul PRIVATE Phantom)
target_link_libraries(ProfileMatMul PRIVATE SparseFHEMatmul)
