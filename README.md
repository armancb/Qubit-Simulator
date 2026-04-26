
# Qubit Simulator

A 3D quantum circuit visualizer and high-performance physics simulator built in C++.

## Overview
Qubit Simulator is a lightweight, visually-driven tool designed to simulate and display the mechanics of quantum circuits. It features a robust mathematical core to accurately calculate quantum states, alongside a graphical frontend powered by Raylib to visualize these states in 3D (ex: mapped onto a Bloch Sphere).

### Key Features
- **Quantum Mechanics Core**: Supports fundamental single and two-qubit operations, including the rigorous calculation of `CNOT` interactions and Phase Kickbacks.
- **Accurate Entanglement Tracking**: Utilizes advanced metrics like _Concurrence_ to accurately calculate and map multi-state entanglement (e.g., verifying true Bell States vs. separable states).
- **3D Visualization (Raylib)**: Projects qubit states visually in 3D space, translating quantum mechanics onto easily interpretable graphical representations.
- **High Performance**: Developed in C++17 and optimized for performance.

## Prerequisites
- **C++ Compiler**: A compiler supporting C++17 (e.g., GCC, Clang, or MSVC).
- **CMake**: Version 3.14 or higher.
- **Raylib**: Necessary for the 3D visualizer module.

## Building and Running
The project uses CMake for building. From the root directory:

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

To run the main simulator executable (after successful compilation):
```bash
./qbits_sim
```

## Project Structure
- **/src**: Contains the core mathematical engine (`qubit_simulator.hpp`). This is a custom-built implementation from scratch utilizing standard C++ `std::complex` math. It handles single-qubit gates (H, X, Y, Z, phase rotations) and two-qubit explicit tensor logic (CNOT, Bell State generations).
- **/sim**: Contains the Raylib 3D graphics implementation (`main_raylib.cpp`) for visualizing Bloch sphere vectors.
- **/benchmark**: Scripts for performance testing the library (`main.cpp`) alongside comparative benchmarks written in Python (`main.py`).

## Benchmarking
The repository includes a dedicated `benchmark` suite where our custom C++ simulator engine is tested to measure simulation scaling and operations per second against a baseline Python counterpart.
- **Methodology:** The standard test (`benchmark/main.cpp`) triggers 1,000,000 iterations of a complex 5-gate rotation sequence (`H -> RZ -> X -> RZ -> H`). The exact same routine is executed via NumPy matrix dot products (`benchmark/main.py`) for an apples-to-apples load comparison.
- **Data Integrity:** The Python script automatically verifies the quantum mathematical correctness of the C++ module's rotational states and probability distributions against standard NumPy references.

### Live Metrics (1M Iterations)
Performance overhead has been virtually eliminated by dropping dense interpreted matrix libraries for our hardcoded, native `std::complex` C++ logic:

| Engine / Metric | Total Circuit Runtime | Throughput (Gates/sec) | Latency Per Gate |
|-----------------|-----------------------|------------------------|------------------|
| **Python (NumPy)** | ~9,777 milliseconds | ~511,390 | 1.9554 µs |
| **C++ (Custom)**  | ~3.89 milliseconds | ~1.28 Billion | 0.0008 µs (0.8 ns) |

**Conclusion:** The custom built C++ engine yields a colossal **~2,444x Speedup** mapping raw tensor mechanics natively compared to executing equivalent mathematics over high-performance Python frameworks.

## From-Scratch Philosophy
`qbit_simulator`'s math engine is built exclusively from scratch without relying on dense external calculation libraries (like Blas, Eigen, etc.). 
- It relies purely on the standard C++ library (heavily leveraging `std::complex` and `std::array`) retaining transparent measurement and math operations. 
- Normalization routines, probability projections (calculating the probabilities of zero `|0⟩` or one `|1⟩` measurements via random distributions), explicit target tensor matrices (`CNOT`, `RX`, `RY`, `RZ`), and Bloch Sphere local axes projections are meticulously hardcoded to guarantee zero hidden abstraction cost and complete mechanical clarity.


