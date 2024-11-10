"""
This setup.py script will install the pybind library located in ./build
Note, the CMake build process must have already been run as documented in README.md
"""
from setuptools import setup
from setuptools.command.install import install
import shutil
from pathlib import Path
import os

def compute_lib_path():
    build_dir = Path(__file__).parent.parent.parent / 'build'
    lib_paths = list(build_dir.glob("*cpython*.so"))
    assert len(lib_paths) > 0, f"Could not detect pybind shared library in {build_dir}. Make sure you have built python bindings using the CMakeLists.txt provided in the 'build' folder"
    assert len(lib_paths) <= 1, f"Detected more than one candidate shared library file in {build_dir}. Please ensure there is one pybind shared library built according to the provided CMakeLists.txt file"
    lib_path = lib_paths[0]
    return lib_path

class CustomInstall(install):
    def run(self):
        super().run()
        install_dir = self.install_lib
        os.makedirs(install_dir, exist_ok=True)
        target_path = os.path.join(install_dir, 'pyfhe_sparse_matmul')
        
        lib_path = compute_lib_path()
        shutil.copyfile(lib_path, os.path.join(target_path, lib_path.name))

setup(
    name='pyfhe_sparse_matmul',
    version='1.0.0',
    description='Python bindings for the FHE Sparse Matrix multiplication library.',
    author='Aidan Ferguson',
    author_email='aidan.t.ferguson@gmail.com',
    packages=['pyfhe_sparse_matmul'], 
    package_dir={'pyfhe_sparse_matmul': "."},
    package_data={'pyfhe_sparse_matmul': [compute_lib_path().name]},
    include_package_data=True,
    cmdclass={'install': CustomInstall},
)

