import sys
from pathlib import Path
sys.path.append(str(Path(__file__).parent.parent.parent.parent / "build"))

from pyfhe_sparse_matmul import SealCKKSContext, SparseNaiveFHE, SparseCSRFHE, SparseELLPACKFHE
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

for scheme in [SparseNaiveFHE, SparseCSRFHE, SparseELLPACKFHE]:
    fhe_a = scheme(a, ckks.runtime, 1)
    fhe_b = scheme(b, ckks.runtime, 1)

    result = fhe_a.fhe_matmul(fhe_b, ckks.runtime, 9)

    print(result.decrypt(ckks.secret))
