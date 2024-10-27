#include <fhe_sparse_matmul/Matrix/SparseNaive.hpp>
#include <fhe_sparse_matmul/Matrix/SparseCSR.hpp>
#include <fhe_sparse_matmul/Matrix/SparseELLPACK.hpp>

typedef SparseFHE::SparseNaiveFHE SparseSchemeFHE; 

int main()
{
    // TODO: would be really cool to have a simple test program
    // then a more complex one with serialisation and networking with client/server
    // TODO: could go really crazy by exposing a PyBind interface for it
    SparseFHE::SealCKKSContext fhe_context(8192);

    const double lhs[3*3] {1.0, 8.0, 0.0, 0.0, 4.0, 0.0, 1.0, 0.0, 3.0};
    const double rhs[3*3] {3.0, 0.0, 1.0, 0.0, 1.0, 0.0, 0.0, 0.0, 2.0};
    double result[3*3] {0.0};

    SparseSchemeFHE lhs_fhe(3, 3, 1, lhs, fhe_context.runtime);
    SparseSchemeFHE rhs_fhe(3, 3, 1, rhs, fhe_context.runtime);


    SparseSchemeFHE dot_fhe = lhs_fhe.fhe_matmul(rhs_fhe, fhe_context.runtime, 1);

    dot_fhe.decrypt(fhe_context.secret, result);
    
    for (uint64_t idx = 0; idx < 3*3; idx++)
    {
        std::cout << result[idx] << ", ";
    }
    std::cout << std::endl;

    return 0;
}