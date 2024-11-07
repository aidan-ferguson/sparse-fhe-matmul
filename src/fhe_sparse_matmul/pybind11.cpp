#ifdef BUILD_PYTHON_BINDINGS

#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
namespace py = pybind11;

#include <fhe_sparse_matmul/SealCKKSContext.hpp>
#include <fhe_sparse_matmul/Matrix/SparseNaive.hpp>
#include <fhe_sparse_matmul/Matrix/SparseCSR.hpp>
#include <fhe_sparse_matmul/Matrix/SparseELLPACK.hpp>
namespace fhe = SparseFHE;

#define PYTHON_MODULE_NAME fhe_sparse_matmul


/// @brief Wrapper for initialising a sparse scheme with matrix data
/// @tparam scheme FHE scheme type to define init wrapper for
/// @return a pointer to the newly allocated FHE sparse object
template<typename scheme>
std::unique_ptr<scheme> sparse_init_wrap(py::array_t<double> data, fhe::SealCKKSRuntimeContext& runtime, uint64_t chunk_size)
{
    py::buffer_info buf = data.request();
    if (buf.ndim != 2)
        throw std::runtime_error("Input data must be a two dimensional matrix");
    
    uint64_t rows = buf.shape.at(0);
    uint64_t cols = buf.shape.at(1);
    
    return std::make_unique<scheme>(rows, cols, chunk_size, static_cast<double*>(buf.ptr), runtime);
}


/// @brief Wrapper for decrypting data held by a sparse scheme
/// @tparam scheme FHE scheme type to define init wrapper for
/// @return a numpy array holding the computed matrix
template<typename scheme>
py::array_t<double> sparse_decrypt_wrap(scheme& self, const fhe::SealCKKSSecretContext& context)
{
    auto decrypted = std::shared_ptr<double>(new double[self.rows()*self.cols()]);
    self.decrypt(context, decrypted.get());

    std::vector<uint64_t> dims = { self.rows(), self.cols() };
    
    return py::array_t<double>(dims, decrypted.get());
}


PYBIND11_MODULE(PYTHON_MODULE_NAME, m) {

    py::class_<fhe::SealCKKSSecretContext>(m, "SealCKKSSecretContext");
    py::class_<fhe::SealCKKSRuntimeContext>(m, "SealCKKSRuntimeContext");

    py::class_<fhe::SealCKKSContext>(m, "SealCKKSContext")
        .def(py::init<const size_t>())
        .def_readwrite("secret", &fhe::SealCKKSContext::secret)
        .def_readwrite("runtime", &fhe::SealCKKSContext::runtime);

    py::class_<fhe::SparseNaiveFHE>(m, "SparseNaiveFHE")
        .def(py::init<const uint64_t, const uint64_t, const uint64_t>())
        .def(py::init(&sparse_init_wrap<fhe::SparseNaiveFHE>))
        .def("fhe_matmul", &fhe::SparseNaiveFHE::fhe_matmul)
        .def("decrypt", &sparse_decrypt_wrap<fhe::SparseNaiveFHE>)
        .def("sparsity", &fhe::SparseNaiveFHE::sparsity);

    py::class_<fhe::SparseCSRFHE>(m, "SparseCSRFHE")
        .def(py::init<const uint64_t, const uint64_t, const uint64_t>())
        .def(py::init(&sparse_init_wrap<fhe::SparseCSRFHE>))
        .def("fhe_matmul", &fhe::SparseCSRFHE::fhe_matmul)
        .def("decrypt", &sparse_decrypt_wrap<fhe::SparseCSRFHE>)
        .def("sparsity", &fhe::SparseCSRFHE::sparsity);

    py::class_<fhe::SparseELLPACKFHE>(m, "SparseELLPACKFHE")
        .def(py::init<const uint64_t, const uint64_t, const uint64_t>())
        .def(py::init(&sparse_init_wrap<fhe::SparseELLPACKFHE>))
        .def("fhe_matmul", &fhe::SparseELLPACKFHE::fhe_matmul)
        .def("decrypt", &sparse_decrypt_wrap<fhe::SparseELLPACKFHE>)
        .def("sparsity", &fhe::SparseELLPACKFHE::sparsity);
}

#endif // ifdef BUILD_PYTHON_BINDINGS