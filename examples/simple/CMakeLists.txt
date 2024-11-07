cmake_minimum_required(VERSION 3.13)

project(SparseExample VERSION 1.0.0 LANGUAGES CXX)

find_package(SEAL 4.1.1 REQUIRED)
find_package(fhe_sparse_matmul REQUIRED)

add_executable(SparseExample main.cpp)

target_link_libraries(SparseExample PRIVATE SEAL::seal)
target_link_libraries(SparseExample PRIVATE fhe_sparse_matmul)
