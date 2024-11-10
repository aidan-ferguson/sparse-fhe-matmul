import importlib.util
import sys
from pathlib import Path

# Dynamically load the pybind shared object file
module_paths = list(Path(__file__).parent.glob("*.so"))
assert len(module_paths) > 0, f"Cannot find a shared object file for {__package__}, aborting import"
module_path = module_paths[0]
spec = importlib.util.spec_from_file_location("pyfhe_sparse_matmul", module_path)
module = importlib.util.module_from_spec(spec)
sys.modules["pyfhe_sparse_matmul"] = module
spec.loader.exec_module(module)