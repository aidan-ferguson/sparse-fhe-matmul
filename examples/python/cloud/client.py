import argparse
from pathlib import Path
import sys
sys.path.append(str(Path(__file__).parent.parent.parent.parent / "build"))

from pyfhe_sparse_matmul import SealCKKSContext, SealCKKSRuntimeContext, SparseCSRFHE
import numpy as np

a = np.array([
    [1.0, 8.0, 0.0], 
    [0.0, 4.0, 0.0],
    [1.0, 0.0, 3.0]
])

b = np.array([
    [3.0, 0.0, 1.0],
    [0.0, 1.0, 0.0], 
    [0.0, 0.0, 2.0]
])

ckks = SealCKKSContext(8192)

fhe_a = SparseCSRFHE(a, ckks.runtime, 1)
fhe_b = SparseCSRFHE(b, ckks.runtime, 1)

fhe_a_buf = fhe_a.serialize()

buf = ckks.runtime.serialize()
runtime = SealCKKSRuntimeContext(buf)
fhe_a_recon = SparseCSRFHE(fhe_a_buf, runtime)

result = fhe_a.fhe_matmul(fhe_b, runtime, 9)

print(result.decrypt(ckks.secret))

# if __name__ == "__main__":
#     parser = argparse.ArgumentParser(description="Client for connecting to the server.")
#     parser.add_argument("--ip", type=str, required=True, help="IP address of the server")
#     parser.add_argument("--port", type=int, required=True, help="Port number of the server")

#     args = parser.parse_args()

#     print(f"Connecting to server at {args.ip}:{args.port}")