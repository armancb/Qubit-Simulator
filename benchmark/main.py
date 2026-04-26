#!/usr/bin/env python3
"""
NumPy reference benchmark for qBITS Single-Qubit Simulator
"""

import numpy as np
import time
import subprocess
import sys

# Gate matrices
H = np.array([[1, 1], [1, -1]], dtype=complex) / np.sqrt(2)
X = np.array([[0, 1], [1, 0]], dtype=complex)

def RZ(theta):
    return np.array([
        [np.exp(-1j * theta / 2), 0],
        [0, np.exp(1j * theta / 2)]
    ], dtype=complex)

def apply_gate(state, gate):
    return gate @ state

def benchmark_numpy(n_iterations=1_000_000):
    state = np.array([1, 0], dtype=complex)
    dummy = 0
    
    start = time.perf_counter()
    for _ in range(n_iterations):
        s = state.copy()
        s = apply_gate(s, H)
        s = apply_gate(s, RZ(0.5))
        s = apply_gate(s, X)
        s = apply_gate(s, RZ(0.3))
        s = apply_gate(s, H)
        dummy += 1 if abs(s[0])**2 > 0.5 else 0  # Use result
    end = time.perf_counter()
    
    total_time = (end - start) * 1e6
    per_gate = total_time / (n_iterations * 5)
    throughput = (n_iterations * 5) / (end - start)
    return total_time, per_gate, throughput, dummy

def verify_correctness():
    print("=" * 60)
    print("CORRECTNESS VERIFICATION (NumPy reference)")
    print("=" * 60)
    tests = []
    
    s = apply_gate(np.array([1, 0], dtype=complex), H)
    tests.append(("H|0⟩ P(0)=0.5", abs(abs(s[0])**2 - 0.5) < 1e-10))
    
    s = apply_gate(np.array([1, 0], dtype=complex), X)
    tests.append(("X|0⟩ = |1⟩", abs(abs(s[1])**2 - 1.0) < 1e-10))
    
    s = apply_gate(apply_gate(np.array([1, 0], dtype=complex), H), H)
    tests.append(("H² = I", abs(abs(s[0])**2 - 1.0) < 1e-10))
    
    for name, passed in tests:
        print(f"{'[PASS]' if passed else '[FAIL]'} {name}")
    return all(t[1] for t in tests)

def run_cpp_benchmark():
    print("\n" + "=" * 60)
    print("C++ SIMULATOR OUTPUT")
    print("=" * 60)
    try:
        result = subprocess.run(["./qbits_sim"], capture_output=True, text=True, timeout=30)
        print(result.stdout)
        return result.stdout
    except FileNotFoundError:
        print("ERROR: ./qbits_sim not found. Build it first.")
        return None

if __name__ == "__main__":
    print("qBITS Single-Qubit Simulator — NumPy Reference Benchmark")
    print("=" * 60)
    
    if not verify_correctness():
        print("\nNumPy reference has errors! Aborting.")
        sys.exit(1)
    
    print("\n" + "=" * 60)
    print("NUMPY BENCHMARK")
    print("=" * 60)
    
    N = 1_000_000
    total_us, per_gate_us, throughput, dummy = benchmark_numpy(N)
    
    print(f"Iterations: {N:,}")
    print(f"Total time: {total_us/1000:.2f} ms")
    print(f"Per gate:   {per_gate_us:.4f} µs")
    print(f"Throughput: {throughput:,.0f} gates/sec")
    print(f"Checksum:   {dummy} (prevents optimization)")
    
    cpp_output = run_cpp_benchmark()
    
    if cpp_output:
        print("\n" + "=" * 60)
        print("COMPARISON")
        print("=" * 60)
        print(f"NumPy per gate: {per_gate_us:.4f} µs")
       