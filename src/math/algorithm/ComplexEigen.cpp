#include "ComplexEigen.h"
#include "LinearAlgebra.h"
#include "PolynomialAlg.h"

#include <cmath>
#include <complex>
#include <stdexcept>
#include <utility>
#include <vector>

namespace algemate::math {

using Poly = Polynomial<Fraction>;

namespace {

using Cmplx = std::complex<double>;

// Horner 求值
Cmplx evalPoly(const std::vector<Cmplx>& c, Cmplx x) {
    Cmplx r(0.0, 0.0);
    for (int i = static_cast<int>(c.size()) - 1; i >= 0; --i)
        r = r * x + c[i];
    return r;
}

// Durand-Kerner 同时求全部复根
std::vector<Cmplx> durandKerner(const std::vector<Cmplx>& coeffs) {
    int n = static_cast<int>(coeffs.size()) - 1;
    if (n <= 0) return {};
    if (n == 1) return {-coeffs[0] / coeffs[1]};
    std::vector<Cmplx> roots(n);
    for (int i = 0; i < n; ++i) {
        double angle = 2.0 * 3.141592653589793 * i / n + 0.4;
        roots[i] = Cmplx(0.4 * std::cos(angle), 0.9 * std::sin(angle));
    }
    for (int iter = 0; iter < 200; ++iter) {
        double maxDelta = 0.0;
        for (int i = 0; i < n; ++i) {
            Cmplx p = evalPoly(coeffs, roots[i]);
            Cmplx denom(1.0, 0.0);
            for (int j = 0; j < n; ++j)
                if (j != i) denom *= (roots[i] - roots[j]);
            if (std::abs(denom) < 1e-60) { roots[i] += Cmplx(1e-8, 1e-8); continue; }
            Cmplx delta = p / denom;
            if (std::abs(delta) > 1e6) delta = delta * (1e6 / std::abs(delta));
            roots[i] -= delta;
            maxDelta = std::max(maxDelta, std::abs(delta));
        }
        if (maxDelta < 1e-12) break;
    }
    return roots;
}

// Polynomial<Fraction> → std::complex<double> 系数 (低位优先)
std::vector<Cmplx> polyToCoeffs(const Poly& p) {
    std::vector<Cmplx> c;
    for (const auto& coeff : p.coeffs())
        c.push_back(Cmplx(coeff.toDouble(), 0.0));
    return c;
}

// 对多项式做数值求根, 返回 Complex 列表
std::vector<Complex> numericalRootsOf(const Poly& f, int mult) {
    if (f.degree() <= 0) return {};
    auto coeffs = polyToCoeffs(f);
    auto roots = durandKerner(coeffs);
    std::vector<Complex> out;
    for (const auto& r : roots) {
        for (int k = 0; k < mult; ++k)
            out.push_back(Complex::fromDouble(r.real(), r.imag()));
    }
    return out;
}

} // anonymous namespace

Matrix<Complex> toComplex(const Matrix<Fraction>& A) {
    Matrix<Complex> B(A.rows(), A.cols());
    for (std::size_t i = 0; i < A.rows(); ++i)
        for (std::size_t j = 0; j < A.cols(); ++j)
            B(i, j) = Complex(A(i, j));
    return B;
}

// Matrix<Complex> 的精确 RREF: 高斯-约当消元, 主元按列扫描
std::size_t rrefComplex(Matrix<Complex>& M) {
    const std::size_t m = M.rows();
    const std::size_t n = M.cols();
    std::size_t r = 0;
    for (std::size_t c = 0; c < n && r < m; ++c) {
        // 在 r..m-1 行中寻找 M(row, c) 非零的行
        std::size_t pivotRow = m;
        for (std::size_t i = r; i < m; ++i) {
            if (!M(i, c).isZero()) { pivotRow = i; break; }
        }
        if (pivotRow == m) continue;
        if (pivotRow != r) M.swapRows(r, pivotRow);
        // 主元归一
        Complex pivot = M(r, c);
        Complex invPivot = Complex(Fraction(1)) / pivot;
        M.scaleRow(r, invPivot);
        // 消去其他行
        for (std::size_t i = 0; i < m; ++i) {
            if (i == r) continue;
            if (M(i, c).isZero()) continue;
            Complex k = -M(i, c);
            M.addMulRow(i, r, k);
        }
        ++r;
    }
    return r;
}

Matrix<Complex> rrefComplexOf(const Matrix<Complex>& M) {
    Matrix<Complex> out = M;
    rrefComplex(out);
    return out;
}

Matrix<Complex> nullspaceComplex(const Matrix<Complex>& Min) {
    const std::size_t m = Min.rows();
    const std::size_t n = Min.cols();
    Matrix<Complex> R = Min;
    rrefComplex(R);
    // 找主元列 (每行第一个非零列)
    std::vector<int> pivotCol(m, -1);
    std::vector<bool> isPivot(n, false);
    for (std::size_t i = 0; i < m; ++i) {
        for (std::size_t j = 0; j < n; ++j) {
            if (!R(i, j).isZero()) {
                pivotCol[i] = static_cast<int>(j);
                isPivot[j] = true;
                break;
            }
        }
    }
    // 自由列
    std::vector<std::size_t> freeCols;
    for (std::size_t j = 0; j < n; ++j) if (!isPivot[j]) freeCols.push_back(j);
    Matrix<Complex> basis(n, freeCols.size());
    for (std::size_t k = 0; k < freeCols.size(); ++k) {
        std::size_t fc = freeCols[k];
        basis(fc, k) = Complex(Fraction(1));
        for (std::size_t i = 0; i < m; ++i) {
            if (pivotCol[i] < 0) continue;
            std::size_t pc = static_cast<std::size_t>(pivotCol[i]);
            basis(pc, k) = -R(i, fc);
        }
    }
    return basis;
}

namespace {

// 剥线性因子 (x - r), 并在 f 中除掉
void peelLinear(Poly& f, const Fraction& r) {
    Poly lin = { -r, Fraction(1) };
    if (!f.isZero() && (f % lin).isZero()) {
        f = f / lin;
    }
}

// 二次不可约因子 f = x^2 + bx + c 的两个复根
std::pair<Complex, Complex> quadraticRoots(const Poly& f) {
    // f monic, degree 2
    Fraction b = f[1];
    Fraction c = f[0];
    Fraction delta = b * b - Fraction(4) * c;
    Complex sqrtDelta = Complex::sqrt(Complex(delta));
    Complex half(Fraction(1) / Fraction(2));
    Complex negBHalf(Fraction(-1) * b / Fraction(2));
    Complex r1 = negBHalf + half * sqrtDelta;
    Complex r2 = negBHalf - half * sqrtDelta;
    return {r1, r2};
}

} // anonymous

ComplexEigenResult complexEigenvalues(const Matrix<Fraction>& A) {
    if (!A.isSquare()) throw std::invalid_argument("complexEigenvalues: not square");
    ComplexEigenResult out;
    Poly cp = charpoly(A);
    auto sqf = squarefreeFactorization(cp);
    // sqf.factors[k] 对应重数 k+1
    for (std::size_t k = 0; k < sqf.factors.size(); ++k) {
        Poly f = sqf.factors[k];
        int mult = static_cast<int>(k + 1);
        if (f.degree() <= 0) continue;
        // 剥全部有理根
        auto roots = rationalRoots(f);
        for (const auto& r : roots) {
            peelLinear(f, r);
            out.eigenvalues.push_back({ Complex(r), mult });
        }
        // 剩余商
        if (f.degree() <= 0) continue;
        if (f.degree() == 1) {
            // 理论上 rationalRoots 应该处理了, 保底
            Fraction r = -f[0] / f[1];
            out.eigenvalues.push_back({ Complex(r), mult });
            continue;
        }
        if (f.degree() == 2) {
            auto pr = quadraticRoots(f.monic());
            out.eigenvalues.push_back({ pr.first, mult });
            out.eigenvalues.push_back({ pr.second, mult });
            continue;
        }
        // 三次及以上: 数值求解全部复根 (Durand-Kerner)
        auto numRoots = numericalRootsOf(f.monic(), mult);
        for (const auto& r : numRoots)
            out.eigenvalues.push_back({ r, mult });
    }
    return out;
}

std::vector<ComplexEigenPair> complexEigenPairs(const Matrix<Fraction>& A) {
    auto res = complexEigenvalues(A);
    std::vector<ComplexEigenPair> out;
    const std::size_t n = A.rows();
    Matrix<Complex> Ac = toComplex(A);
    Matrix<Complex> I  = Matrix<Complex>::identity(n);

    // 合并相同特征值 (相等则累加重数)
    std::vector<ComplexEigenvalue> merged;
    for (const auto& ev : res.eigenvalues) {
        bool found = false;
        for (auto& m : merged) {
            if (m.value == ev.value) {
                m.multiplicity += ev.multiplicity;
                found = true;
                break;
            }
        }
        if (!found) merged.push_back(ev);
    }

    for (const auto& ev : merged) {
        Matrix<Complex> B = Ac - I * ev.value;
        Matrix<Complex> basis = nullspaceComplex(B);
        out.push_back({ ev.value, ev.multiplicity, basis });
    }
    return out;
}

} // namespace algemate::math
