# Sparse Fully Homomorphic Encryption (FHE) Matrix Multiplication

## Introduction

This repository is an accessible open-source implementation of an upcoming paper submission, it will be updated accordingly when the submission is complete.

Our implementation wraps the [Microsoft SEAL](https://github.com/microsoft/SEAL) FHE library and exposes a user friendly C++ API for performing multi-threaded matrix multiplication on matrices with arbitrary levels of unstructured sparsity.

A [mini-tutorial](./examples/simple/main.cpp) is available in our `examples/` folder, it provides a walk-through of the available API functionality.

Python bindings may be added in the future.


## Installation Instructions
### Installing Microsoft Seal

This library depends on the Microsoft SEAL project. Installation instructions can be found within the README of their [repository](https://github.com/microsoft/SEAL?tab=readme-ov-file#building-microsoft-seal-manually).

Microsoft SEAL 4.1.1 is required for this project. Other versions may work but compatibility is not guaranteed and you will have to change the version requirements in `CMakeLists.txt`.

### Building the Library

Run the following commands in the root directory of the repository

```
cmake -S . -B build
```

```
cmake --build build
```

### Installing the Library

You may install the library to a custom location is desired, by default it will attempt to install globally:

```
sudo cmake --install build
```

### Compiling Test Project

An example program is provided. To compile and run this, please run the following commands in the `example/` directory:

```
cmake -S . -B build
cmake --build build
./build/SparseExample
```

You should see output from the program verifying that the encrypted matrix operations are working. You are now ready to use this in your own programs!
A template `CMakeLists.txt` can be found in the `example/` directory.

## Citing Sparse FHE MatMul

If you use our work in academic papers, please use the following BibTeX entry for citations.

```
@misc{sparsefhematmul,
    title = {Sparse FHE MatMul},
    howpublished = {\url{https://github.com/aidan-ferguson/sparse-fhe-matmul}},
    month = oct,
    year = 2024,
    author={Aidan Ferguson}
}
```