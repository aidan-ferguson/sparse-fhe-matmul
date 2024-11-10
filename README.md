# Sparse Fully Homomorphic Encryption (FHE) Matrix Multiplication

## Introduction

This repository is an accessible open-source implementation of an upcoming paper submission, it will be updated accordingly when the submission is complete.

Our implementation wraps the [Microsoft SEAL](https://github.com/microsoft/SEAL) FHE library and exposes a user friendly C++ API and Python bindings for performing multi-threaded matrix multiplication on matrices with arbitrary levels of unstructured sparsity.


## Installation Instructions
### Installing Microsoft Seal

This library depends on the Microsoft SEAL project. Installation instructions can be found within the README of their [repository](https://github.com/microsoft/SEAL?tab=readme-ov-file#building-microsoft-seal-manually).

Microsoft SEAL 4.1.1 is required for this project. Other versions may work but compatibility is not guaranteed and you will have to change the version requirements in `CMakeLists.txt`.

### Building the Library

Run the following commands in the root directory of the repository. Note, Python bindings are only currently supported on Linux/UNIX based systems for now and require the `libpython3-dev` and `pybind11-dev` packages to be installed.

```bash
cmake -S . -B build -DPYTHON_BINDINGS=ON
```

```bash
cmake --build build
```

### Installing the Library

You may install the library to a custom location is desired, by default it will attempt to install globally:

```bash
sudo cmake --install build
```

The library can now be accessed using the `find_package(fhe_sparse_matmul ...)` function in CMake

### Python Bindings

Python bindings are built by default, if you wish to disable them please generate CMake project files with the following command:

```bash
cmake -S . -B build -DPYTHON_BINDINGS=OFF
```

After building normally a shared object library with python bindings can be found in `./build`. To install this as a pip package so you can access the library elsewhere, run the following:

```bash
python3 -m pip install src/pyfhe_sparse_matmul
```

The library can now be accessed globally:
```python
import pyfhe_sparse_matmul

...
```

## Usage

### Simple C++ Test Project

A C++ example program is provided. To compile and run this, please run the following commands in the `example/cpp/simple` directory:

```bash
cmake -S . -B build
cmake --build build
./build/SparseExample
```

You should see output from the program verifying that the encrypted matrix operations are working.
A template `CMakeLists.txt` can be found in the this directory.

### Advanced End-to-End MNIST DNN example in Python

A more advanced demo can be found in `examples/python/endtoend`. This demonstrates a real-world use case for sparse DNN inference an library features such as serialisation. The [README.md](examples/python/endtoend/README.md) file in the folder contains more details.

## Citing Sparse FHE MatMul

If you use our work in academic papers, please use the following BibTeX entry for citations.

```bibtex
TBC on paper acceptance
```