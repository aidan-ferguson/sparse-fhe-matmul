#include <fhe_sparse_matmul/Matrix/SparseNaive.hpp>

int main()
{
    SparseFHE::SealCKKSContext fhe_context(8192);

    const double lhs[3*3] {1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0};
    const double rhs[3*3] {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0};
    double result[3*3] {0.0};

    SparseFHE::SparseNaiveFHE lhs_fhe(3, 3, 1, lhs, fhe_context.runtime);
    SparseFHE::SparseNaiveFHE rhs_fhe(3, 3, 1, rhs, fhe_context.runtime);


    SparseFHE::SparseNaiveFHE dot_fhe = lhs_fhe.fhe_matmul(rhs_fhe, fhe_context.runtime, 1);

    dot_fhe.decrypt(fhe_context.secret, result);
    
    for (uint64_t idx = 0; idx < 3*3; idx++)
    {
        std::cout << result[idx] << ", ";
    }
    std::cout << std::endl;

    return 0;
}