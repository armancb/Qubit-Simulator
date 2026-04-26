# Qubit Simulator Bug Fixes

Here is a summary of the changes made to resolve the rendering and math engine bugs in the 3D quantum circuit visualizer.

## 1. Missed Entanglement Fix (Bell State)
**Files Modified:** `sim/main_raylib.cpp`
**Change details:** 
The Bloch vectors were not shrinking when a Bell State was formed. The `RecomputeState` function was updated to calculate a scalar factor based on the state's actual entanglement (using the `GetEntanglement` function). 
The vector coordinates (`app.blochEnd` and `app.blochEnd2`) are scaled by `1.0f - GetEntanglement(app.q2)`. This ensures that when the entanglement is maximal (metric hits 1.0), the vector length correctly shrinks to 0.0 (the origin).

## 2. False Entanglement Fix (Phase Kickback)
**Files Modified:** `src/qubit_simulator.hpp`, `sim/main_raylib.cpp`
**Change details:** 
- **Physics Core (`CNOT` method):** The `TwoQubit::CNOT()` method was strictly hardcoded to use Qubit 0 as control and Qubit 1 as target. This disrupted routines that mapped Phase Kickbacks. It was updated to `CNOT(int control, int target)` to respect explicit target and control parameters, properly facilitating interactions where Qubit 1 is the control.
- **Entanglement Metric:** `GetEntanglement` was upgraded from a naive probability distribution metric ($p_{00} p_{11} - p_{01} p_{10}$) to **Concurrence** ($2 \times |ad - bc|$). This prevents separable Phase Kickback states from generating false positive entanglement strengths, effectively keeping the vectors anchored to the sphere surface upon measurement.

## 3. Coordinate System Fix
**Files Modified:** `sim/main_raylib.cpp`
**Change details:** 
Physics conventions dictate that `Z` is altitude, but Raylib maps altitude to `Y`. To correct the layout and ensure vertical states (like $|1\rangle$) point towards the South Pole, a matrix transform was implemented in the `Draw3DViz` function. 
Vectors `curr` are mapped locally via `{curr.x, curr.z, -curr.y}` to ensure alignment with standard 3D depth buffers while preserving correct chirality.
