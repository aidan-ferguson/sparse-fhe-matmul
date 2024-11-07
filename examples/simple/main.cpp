#include <fhe_sparse_matmul/Matrix/SparseNaive.hpp>
#include <fhe_sparse_matmul/Matrix/SparseCSR.hpp>
#include <fhe_sparse_matmul/Matrix/SparseELLPACK.hpp>

/*
*   This program serves as a bit of a test to ensure the  functionality of the API is working, 
*   but also as an example of how to use it. Follow comments throughout the code as a guide.
*
*   If you have any issues/comments, please open an issue on the GitHub repository!
*   https://github.com/aidan-ferguson/sparse-fhe-matmul
*/

/*
*   For this demo, you can change the sparse scheme you use here, we recommend SparseCSR as it 
*   has the lowest memory requirements and a similar runtime to the other schemes.
*/
typedef SparseFHE::SparseELLPACKFHE SparseSchemeFHE; 

int main(int argc, char** argv)
{
    /*
    *   Initialise the Microsoft SEAL context, if you have an existing context in your program
    *   please manually construct a SparseFHE::SealCKKSContext object.
    * 
    *   The parameter corresponds to the polynomial modulus degree of the CKKS context we construct
    *   it must be an integer power of 2. Larger numbers allow for more complex operations, i.e.
    *   more consecutive multiplications without bootstrapping, at a much higher runtime.
    * 
    *   Our execution contexts are split into two parts that can be accessed as struct members:
    * 
    *   - runtime: this structure is used for most operations (encryption & multiplication)
    *              it is safe to transmit across the network without concern for exposing secrets.
    *   - secret : this structure should not be shared outwith the current process, it contains
    *              the secret key required to decrypt ciphertexts. 
    */
    SparseFHE::SealCKKSContext fhe_context(8192);

    /*
    *   We define our matrices here as C style arrays, they must be in row-major ordering
    *   It is up to the user to ensure enough memory has been allocated for the matrix sizes provided.
    *   No checks are performed in the library.
    */
    const uint64_t ROWS = 3;
    const uint64_t COLS = 3;
    const double lhs[ROWS*COLS] {1.0, 8.0, 0.0, 0.0, 4.0, 0.0, 1.0, 0.0, 3.0};
    const double rhs[ROWS*COLS] {3.0, 0.0, 1.0, 0.0, 1.0, 0.0, 0.0, 0.0, 2.0};

    /*
    *   We initialise both the left and right hand operand of the subsequent matrix multiplication
    *   Note that encryption occurs as part of this process, so it may be slow.
    */
    SparseSchemeFHE lhs_fhe(3, 3, 1, lhs, fhe_context.runtime);
    SparseSchemeFHE rhs_fhe(3, 3, 1, rhs, fhe_context.runtime);

    std::stringstream test;
    lhs_fhe.serialize(test);
    SparseSchemeFHE recon(test, fhe_context.runtime);

    /*
    *   Now, actually perform the multiplication. The statement can be read right to left as in notation
    *   so the statement a.fhe_matmul(b, ...) corresponds to A \dot B.
    * 
    *   Note that the number of threads is passed as the last parameter, here we use 9 as one thread per
    *   element allows for up to 9 threads.
    */
    SparseSchemeFHE dot_fhe = recon.fhe_matmul(rhs_fhe, fhe_context.runtime, 9);

    /*
    *   Here we decrypt the matrix just computed in encrypted space.
    *   Note, it is up to the user to allocate a buffer with enough memory to store the results of the
    *   computation
    */
    double result[dot_fhe.rows() * dot_fhe.cols()] {0.0};
    dot_fhe.decrypt(fhe_context.secret, result);
    
    std::cout << "LHS Matrix has sparsity = " << lhs_fhe.sparsity() << std::endl;
    std::cout << "RHS Matrix has sparsity = " << rhs_fhe.sparsity() << std::endl;
    std::cout << "Result Matrix has sparsity = " << dot_fhe.sparsity() << std::endl;
    
    std::cout << "\nResult Matrix:" << std::endl;
    for (uint64_t row = 0; row < 3; row++) {
        for (uint64_t col = 0; col < 3; col++) {
            std::cout << result[(row*3) + col] << ", ";
        }
        std::cout << std::endl;
    }

    return 0;
}