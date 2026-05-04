#include "ComplexEigen.h"
#include "LinearAlgebra.h"
#include "PolynomialAlg.h"

#include <stdexcept>
#include <utility>

namespace algemate::math {

using Poly = Polynomial<Fraction>;

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
        // 三次及以上: 保留为未分裂因子
        out.unsolvedFactors.emplace_back(f.monic(), mult);
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
