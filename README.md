# Sparse Fully Homomorphic Encryption (FHE) Matrix Multiplication

## Installation Instructions

This will walk you through installing the library in your system.



### Installing Microsoft Seal

This library depends on the Microsoft SEAL project. Installation instructions can be found within the README of their [repository](https://github.com/microsoft/SEAL?tab=readme-ov-file#building-microsoft-seal-manually).

Microsoft SEAL 4.1.1 is required for this project. Other versions may work but compatibility is not guaranteed.

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