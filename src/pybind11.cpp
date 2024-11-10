#ifdef BUILD_PYTHON_BINDINGS
#define PYTHON_MODULE_NAME pyfhe_sparse_matmul

#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
namespace py = pybind11;

#include <fhe_sparse_matmul/SealCKKSContext.hpp>
#include <fhe_sparse_matmul/Matrix/SparseNaive.hpp>
#include <fhe_sparse_matmul/Matrix/SparseCSR.hpp>
#include <fhe_sparse_matmul/Matrix/SparseELLPACK.hpp>
namespace fhe = SparseFHE;


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


/// @brief Wrapper for deserializing an object from a buffer
/// @tparam T Object type to be deserialized
/// @param buffer Buffer to deserialize into object
/// @return Instance of T generated from buffer data
template<typename T>
std::unique_ptr<T> deserialize_wrap(py::bytes& buffer)
{
    std::string char_buf = buffer; 
    std::stringstream ss(char_buf);
    return std::make_unique<T>(ss);
}
template<typename T>
std::unique_ptr<T> deserialize_wrap_scheme(py::bytes& buffer, fhe::SealCKKSRuntimeContext& context)
{
    std::string char_buf = buffer; 
    std::stringstream ss(char_buf);
    return std::make_unique<T>(ss, context);
}


/// @brief Wrapper for serializing an object into a buffer
/// @tparam T Object type to be serialized
/// @return Byte buffer containing object serialized data
template<typename T>
py::bytes serialize_wrap(T& self)
{
    std::stringstream ss;
    self.serialize(ss);
    return py::bytes(ss.str());
}

#define SPARSE_BIND(scheme, name) \
    py::class_<scheme>(m, name) \
        .def(py::init<const uint64_t, const uint64_t, const uint64_t>()) \
        .def(py::init(&sparse_init_wrap<scheme>)) \
        .def(py::init(&deserialize_wrap_scheme<scheme>)) \
        .def("fhe_matmul", &scheme::fhe_matmul) \
        .def("decrypt", &sparse_decrypt_wrap<scheme>) \
        .def("sparsity", &scheme::sparsity) \
        .def("square_inplace", [](scheme& self, fhe::SealCKKSRuntimeContext& context) {return self.square_inplace(context);}) \
        .def("serialize", &serialize_wrap<scheme>) \


PYBIND11_MODULE(PYTHON_MODULE_NAME, m) {
    py::class_<fhe::SealCKKSSecretContext>(m, "SealCKKSSecretContext");
    py::class_<fhe::SealCKKSRuntimeContext>(m, "SealCKKSRuntimeContext")
        .def(py::init(&deserialize_wrap<fhe::SealCKKSRuntimeContext>))
        .def("serialize", &serialize_wrap<fhe::SealCKKSRuntimeContext>);

    py::class_<fhe::SealCKKSContext>(m, "SealCKKSContext")
        .def(py::init<const size_t>())
        .def_readwrite("secret", &fhe::SealCKKSContext::secret)
        .def_readwrite("runtime", &fhe::SealCKKSContext::runtime);

    // py::class_<fhe::SparseNaiveFHE>(m, "SparseNaiveFHE")
    //     .def(py::init<const uint64_t, const uint64_t, const uint64_t>())
    //     .def(py::init(&sparse_init_wrap<fhe::SparseNaiveFHE>))
    //     .def(py::init(&deserialize_wrap_scheme<fhe::SparseNaiveFHE>))
    //     .def("fhe_matmul", &fhe::SparseNaiveFHE::fhe_matmul)
    //     .def("decrypt", &sparse_decrypt_wrap<fhe::SparseNaiveFHE>)
    //     .def("sparsity", &fhe::SparseNaiveFHE::sparsity)
    //     .def("square_inplace", [](fhe::SparseNaiveFHE& self, fhe::SealCKKSRuntimeContext& context) {return self.square_inplace(context);})
    //     .def("serialize", &serialize_wrap<fhe::SparseNaiveFHE>);

    SPARSE_BIND(fhe::SparseNaiveFHE, "SparseNaiveFHE");
    SPARSE_BIND(fhe::SparseCSRFHE, "SparseCSRFHE");
    SPARSE_BIND(fhe::SparseELLPACKFHE, "SparseELLPACKFHE");
    
    // py::class_<fhe::SparseELLPACKFHE>(m, "SparseELLPACKFHE")
    //     .def(py::init<const uint64_t, const uint64_t, const uint64_t>())
    //     .def(py::init(&sparse_init_wrap<fhe::SparseELLPACKFHE>))
    //     .def(py::init(&deserialize_wrap_scheme<fhe::SparseELLPACKFHE>))
    //     .def("fhe_matmul", &fhe::SparseELLPACKFHE::fhe_matmul)
    //     .def("decrypt", &sparse_decrypt_wrap<fhe::SparseELLPACKFHE>)
    //     .def("sparsity", &fhe::SparseELLPACKFHE::sparsity)
    //     .def("square_inplace", [](fhe::SparseELLPACKFHE& self, fhe::SealCKKSRuntimeContext& context) {return self.square_inplace(context);})
    //     .def("serialize", &serialize_wrap<fhe::SparseELLPACKFHE>);
}

#endif // ifdef BUILD_PYTHON_BINDINGS