#include "../src/qubit_simulator.hpp"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include <iostream>
#include <chrono>
#include <cmath>

using namespace qbits;

void printSection(const std::string &title)
{
    std::cout << "\n"
              << std::string(60, '=') << "\n";
    std::cout << "  " << title << "\n";
    std::cout << std::string(60, '=') << "\n";
}

int main()
{
    printSection("QBITS SINGLE-QUBIT GATE SIMULATOR");

    // ── Basic Gate Demonstrations ──
    printSection("1. BASIC GATES");

    Qubit q;
    q.print("Initial |0⟩");

    q.H();
    q.print("After H");
    std::cout << "  → Should be (|0⟩+|1⟩)/√2, P(0)=P(1)=0.5\n";

    q.X();
    q.print("After X");
    std::cout << "  → X flips |+⟩ to |+⟩ (verify: still ~0.5/0.5)\n";

    q.Z();
    q.print("After Z");
    std::cout << "  → Z|+⟩ = |-⟩ = (|0⟩-|1⟩)/√2\n";

    q.H();
    q.print("After H again");
    std::cout << "  → H|-⟩ = |1⟩\n";

    // ── Rotation Gates ──
    printSection("2. ROTATION GATES (RZ, RX, RY)");

    Qubit q2;
    q2.RZ(3.14 / 4.0);
    q2.print("RZ(π/4) on |0⟩");
    std::cout << "  → |0⟩ is eigenstate of Z, so RZ only adds global phase\n";

    Qubit q3;
    q3.H().RZ(M_PI / 4.0).H();
    q3.print("H → RZ(π/4) → H");
    std::cout << "  → Equivalent to rotation around X by π/4\n";

    Qubit q4;
    q4.RX(M_PI / 2.0);
    q4.print("RX(π/2) on |0⟩");
    std::cout << "  → Should be ~(|0⟩ - i|1⟩)/√2\n";

    Qubit q5;
    q5.RY(M_PI / 2.0);
    q5.print("RY(π/2) on |0⟩");
    std::cout << "  → Should be ~(|0⟩ + |1⟩)/√2 (same as H up to phase)\n";

    // ── Bloch Sphere Coordinates ──
    printSection("3. BLOCH SPHERE COORDINATES");

    Qubit states[] = {
        Qubit(),
        Qubit().X(),
        Qubit().H(),
        Qubit().H().Z()};
    const char *names[] = {"|0⟩", "|1⟩", "|+⟩", "|−⟩"};

    for (int i = 0; i < 4; ++i)
    {
        auto [x, y, z] = states[i].blochSphere();
        std::cout << names[i] << ": (x=" << x << ", y=" << y << ", z=" << z << ")\n";
    }

    // ── Measurement Statistics ──
    printSection("4. MEASUREMENT STATISTICS (10,000 shots)");

    Qubit q6;
    q6.H();
    auto counts = q6.measureShots(10000);
    std::cout << "H|0⟩ measured 10,000 times:\n";
    std::cout << "  |0⟩: " << counts[0] << " (" << 100.0 * counts[0] / 10000.0 << "%)\n";
    std::cout << "  |1⟩: " << counts[1] << " (" << 100.0 * counts[1] / 10000.0 << "%)\n";

    // ── Single-Qubit Circuit: Quantum Coin Flip ──
    printSection("5. QUANTUM COIN FLIP (H → measure)");

    int heads = 0, tails = 0;
    for (int i = 0; i < 1000; ++i)
    {
        Qubit coin;
        coin.H();
        if (coin.measure() == 0)
            heads++;
        else
            tails++;
    }
    std::cout << "1000 quantum coin flips:\n";
    std::cout << "  Heads (|0⟩): " << heads << "\n";
    std::cout << "  Tails (|1⟩): " << tails << "\n";

    // ── Two-Qubit: Bell State ──
    printSection("6. BELL STATE |Φ⁺⟩ = (|00⟩ + |11⟩)/√2");

    TwoQubit bell;
    bell.makeBellState();
    bell.print("Bell state");
    std::cout << "  P(|00⟩) = " << bell.prob(0) << ", P(|11⟩) = " << bell.prob(3) << "\n";

    // ── Performance Benchmark ──
    printSection("7. PERFORMANCE BENCHMARK");

    const int N = 1000000;
    auto start = std::chrono::high_resolution_clock::now();

    volatile int dummy = 0; // Forces compiler to keep the loop
    for (int i = 0; i < N; ++i)
    {
        Qubit qb;
        qb.H().RZ(0.5).X().RZ(0.3).H();
        dummy += qb.prob0(); // Touch result, prevent optimization
    }
    std::cout << " (checksum: " << dummy << ")\n"; // Print so compiler can't delete it

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    double per_op = static_cast<double>(duration.count()) / (N * 5);

    std::cout << N << " iterations of 5-gate circuit\n";
    std::cout << "Total time: " << duration.count() / 1000.0 << " ms\n";
    std::cout << "Per gate: " << per_op << " µs\n";
    std::cout << "Throughput: " << (N * 5.0) / (duration.count() / 1e6) << " gates/sec\n";

    // ── Verification Tests ──
    printSection("8. VERIFICATION TESTS");

    bool all_pass = true;
    auto check = [&](const std::string &name, bool condition)
    {
        std::cout << (condition ? "[PASS] " : "[FAIL] ") << name << "\n";
        if (!condition)
            all_pass = false;
    };

    Qubit t1;
    t1.H();
    check("H|0⟩ normalization", t1.isNormalized());
    check("H|0⟩ P(0)=0.5", std::abs(t1.prob0() - 0.5) < 1e-10);

    Qubit t2;
    t2.X();
    check("X|0⟩ = |1⟩", std::abs(t2.prob1() - 1.0) < 1e-10);

    Qubit t3;
    t3.X().Z();
    check("Z|1⟩ probabilities", std::abs(t3.prob1() - 1.0) < 1e-10);

    Qubit t4;
    t4.H().H();
    check("H² = I", std::abs(t4.prob0() - 1.0) < 1e-10);

    Qubit t5a, t5b;
    t5a.RX(M_PI);
    t5b.X();
    check("RX(π) ≡ X (probabilities)",
          std::abs(t5a.prob0() - t5b.prob0()) < 1e-10 &&
              std::abs(t5a.prob1() - t5b.prob1()) < 1e-10);

    TwoQubit t6;
    t6.makeBellState();
    check("Bell state P(|00⟩)=0.5", std::abs(t6.prob(0) - 0.5) < 1e-10);
    check("Bell state P(|11⟩)=0.5", std::abs(t6.prob(3) - 0.5) < 1e-10);
    check("Bell state P(|01⟩)=0", std::abs(t6.prob(1)) < 1e-10);
    check("Bell state P(|10⟩)=0", std::abs(t6.prob(2)) < 1e-10);

    std::cout << "\n"
              << (all_pass ? "ALL TESTS PASSED ✓" : "SOME TESTS FAILED ✗") << "\n";

    return all_pass ? 0 : 1;
}