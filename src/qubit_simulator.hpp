#pragma once

#include <iostream>
#include <array>
#include <complex>
#include <cmath>
#include <iomanip>
#include <random>
#include <string>

namespace qbits
{
    using Complex = std::complex<double>;

    class Qubit
    {
    private:
        std::array<Complex, 2> state_;

        void normalize()
        {
            double norm = std::sqrt(std::norm(state_[0]) + std::norm(state_[1]));
            if (norm > 0)
            {
                state_[0] /= norm;
                state_[1] /= norm;
            }
        }

    public:
        // Default: initialize to |0⟩
        Qubit() : state_{Complex(1.0, 0.0), Complex(0.0, 0.0)} {}
        Qubit(Complex alpha, Complex beta) : state_{alpha, beta}
        {
            normalize();
        }
        bool isNormalized(double tol = 1e-10) const
        {
            return std::abs((std::norm(state_[0]) + std::norm(state_[1])) - 1.0) < tol;
        }
        Qubit &applyGate(const std::array<Complex, 4> &gate)
        {
            Complex new0 = gate[0] * state_[0] + gate[1] * state_[1];
            Complex new1 = gate[2] * state_[0] + gate[3] * state_[1];

            state_[0] = new0;
            state_[1] = new1;

            return *this;
        }

        struct Gates
        {
            static constexpr double ISQRT2 = 0.70710678118654752440;

            static constexpr std::array<Complex, 4> H = {
                Complex(ISQRT2, 0.0), Complex(ISQRT2, 0.0),
                Complex(ISQRT2, 0.0), Complex(-ISQRT2, 0.0)};

            static constexpr std::array<Complex, 4> X = {
                Complex(0.0, 0.0), Complex(1.0, 0.0),
                Complex(1.0, 0.0), Complex(0.0, 0.0)};

            static constexpr std::array<Complex, 4> Y = {
                Complex(0.0, 0.0), Complex(0.0, -1.0),
                Complex(0.0, 1.0), Complex(0.0, 0.0)};

            static constexpr std::array<Complex, 4> Z = {
                Complex(1.0, 0.0), Complex(0.0, 0.0),
                Complex(0.0, 0.0), Complex(-1.0, 0.0)};

            static constexpr std::array<Complex, 4> S = {
                Complex(1.0, 0.0), Complex(0.0, 0.0),
                Complex(0.0, 0.0), Complex(0.0, 1.0)};

            static constexpr std::array<Complex, 4> T = {
                Complex(1.0, 0.0), Complex(0.0, 0.0),
                Complex(0.0, 0.0), Complex(ISQRT2, ISQRT2)};
        };

        Qubit &H() { return applyGate(Gates::H); }
        Qubit &X() { return applyGate(Gates::X); }
        Qubit &Y() { return applyGate(Gates::Y); }
        Qubit &Z() { return applyGate(Gates::Z); }
        Qubit &S() { return applyGate(Gates::S); }
        Qubit &T() { return applyGate(Gates::T); }

        Qubit &RX(double theta)
        {
            std::array<Complex, 4> rx = {
                Complex(std::cos(theta / 2), 0.0), Complex(0.0, -std::sin(theta / 2)),
                Complex(0.0, -std::sin(theta / 2)), Complex(std::cos(theta / 2), 0.0)};
            return applyGate(rx);
        }
        Qubit &RY(double theta)
        {
            std::array<Complex, 4> ry = {
                Complex(std::cos(theta / 2), 0.0), Complex(-std::sin(theta / 2), 0.0),
                Complex(std::sin(theta / 2), 0.0), Complex(std::cos(theta / 2), 0.0)};
            return applyGate(ry);
        }
        Qubit &RZ(double theta)
        {
            std::array<Complex, 4> rz = {
                Complex(std::cos(theta / 2), -std::sin(theta / 2)), Complex(0.0, 0.0),
                Complex(0.0, 0.0), Complex(std::cos(theta / 2), std::sin(theta / 2))};
            return applyGate(rz);
        }

        int measure()
        {
            double prob0 = std::norm(state_[0]);

            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_real_distribution<double> dis(0.0, 1.0);

            if (dis(gen) < prob0)
            {
                state_[0] = Complex(1.0, 0.0);
                state_[1] = Complex(0.0, 0.0);
                return 0;
            }
            else
            {
                state_[0] = Complex(0.0, 0.0);
                state_[1] = Complex(1.0, 0.0);
                return 1;
            }
        }

        std::array<int, 2> measureShots(int shots) const
        {
            double prob0 = std::norm(state_[0]);
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_real_distribution<> dis(0.0, 1.0);

            std::array<int, 2> counts = {0, 0};
            for (int i = 0; i < shots; ++i)
            {
                if (dis(gen) < prob0)
                    counts[0]++;
                else
                    counts[1]++;
            }
            return counts;
        }

        // ⟨ψ|Z|ψ⟩ = |α|² - |β|²
        double expectationZ() const
        {
            return std::norm(state_[0]) - std::norm(state_[1]);
        }
        // ⟨ψ|Y|ψ⟩ = 2·Im(α* · β)
        double expectationY() const
        {
            return 2.0 * (state_[0] * std::conj(state_[1])).imag();
        }
        // ⟨ψ|X|ψ⟩ = 2·Re(α* · β)
        double expectationX() const
        {
            return 2.0 * (state_[0] * std::conj(state_[1])).real();
        }

        // Bloch sphere coordinates (x, y, z)
        std::array<double, 3> blochSphere() const
        {
            return {expectationX(), expectationY(), expectationZ()};
        }

        Complex alpha() const { return state_[0]; }
        Complex beta() const { return state_[1]; }
        double prob0() const { return std::norm(state_[0]); }
        double prob1() const { return std::norm(state_[1]); }

        // Pretty print
        void print(const std::string &label = "") const
        {
            if (!label.empty())
                std::cout << "[" << label << "] ";
            std::cout << "|ψ⟩ = " << std::fixed << std::setprecision(4)
                      << "(" << state_[0].real() << (state_[0].imag() >= 0 ? "+" : "") << state_[0].imag() << "i)|0⟩"
                      << " + (" << state_[1].real() << (state_[1].imag() >= 0 ? "+" : "") << state_[1].imag() << "i)|1⟩"
                      << "  [P(0)=" << prob0() << ", P(1)=" << prob1() << "]" << std::endl;
        }
    };

    class TwoQubit
    {
    private:
        std::array<Complex, 4> state_;

    public:
        TwoQubit() : state_{Complex(1.0, 0.0), Complex(0.0, 0.0),
                            Complex(0.0, 0.0), Complex(0.0, 0.0)} {}
        Complex amp(int idx) const { return state_[idx]; }
        TwoQubit &applySingle(const std::array<Complex, 4> &gate, int target)
        {
            std::array<Complex, 4> new_state;

            if (target == 0)
            {
                for (int i = 0; i < 2; ++i)
                {
                    for (int j = 0; j < 2; ++j)
                    {
                        new_state[i * 2 + j] = gate[i * 2 + 0] * state_[0 * 2 + j] + gate[i * 2 + 1] * state_[1 * 2 + j];
                    }
                }
            }
            else
            {
                for (int i = 0; i < 2; ++i)
                {
                    for (int j = 0; j < 2; ++j)
                    {
                        new_state[i * 2 + j] = gate[j * 2 + 0] * state_[i * 2 + 0] + gate[j * 2 + 1] * state_[i * 2 + 1];
                    }
                }
            }
            state_ = new_state;
            return *this;
        }

        TwoQubit &CNOT(int control, int target)
        {
            if (control == 0 && target == 1) {
                std::swap(state_[2], state_[3]);
            } else if (control == 1 && target == 0) {
                std::swap(state_[1], state_[3]);
            }
            return *this;
        }
        TwoQubit &makeBellState()
        {
            *this = TwoQubit();              // Reset to |00⟩
            applySingle(Qubit::Gates::H, 0); // H on first qubit: (|00⟩ + |10⟩)/√2
            CNOT(0, 1);                          // |10⟩ → |11⟩: (|00⟩ + |11⟩)/√2
            return *this;
        }
        double prob(int idx) const
        {
            return std::norm(state_[idx]);
        }
        void print(const std::string &label = "") const
        {
            if (!label.empty())
                std::cout << "[" << label << "] ";
            std::cout << "|ψ⟩ = ";
            const char *basis[] = {"|00⟩", "|01⟩", "|10⟩", "|11⟩"};
            bool first = true;
            for (int i = 0; i < 4; ++i)
            {
                if (std::norm(state_[i]) > 1e-10)
                {
                    if (!first)
                        std::cout << " + ";
                    std::cout << std::fixed << std::setprecision(4)
                              << "(" << state_[i].real()
                              << (state_[i].imag() >= 0 ? "+" : "")
                              << state_[i].imag() << "i)" << basis[i];
                    first = false;
                }
            }
            if (first)
                std::cout << "0";
            std::cout << std::endl;
        }
    };

} // namespace qbits
