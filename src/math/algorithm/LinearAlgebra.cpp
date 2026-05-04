#include "LinearAlgebra.h"

#include "algorithm/PolynomialAlg.h"

#include <algorithm>
#include <stdexcept>
#include <string>
#include <vector>

namespace algemate::math {

namespace {

// RREF 核心实现: trace 为 nullptr 时为纯计算路径
std::size_t rrefImpl_(Matrix<Fraction>& M, StepSequence* trace) {
    const std::size_t R = M.rows();
    const std::size_t C = M.cols();
    if (trace) trace->pushInitial(M);

    std::size_t r = 0;
    for (std::size_t c = 0; c < C && r < R; ++c) {
        std::size_t pivot = R;
        for (std::size_t i = r; i < R; ++i) {
            if (!M(i, c).isZero()) { pivot = i; break; }
        }
        if (pivot == R) continue;

        if (pivot != r) {
            M.swapRows(r, pivot);
            if (trace) trace->pushSwapRows(r, pivot, M);
        }

        if (trace) trace->pushSelectPivot(r, c, M);

        const Fraction pivVal = M(r, c);
        if (!(pivVal == Fraction(1))) {
            Fraction invK = Fraction(1) / pivVal;
            M.scaleRow(r, invK);
            if (trace) trace->pushScaleRow(r, invK, M);
        }

        for (std::size_t i = 0; i < R; ++i) {
            if (i == r) continue;
            const Fraction factor = M(i, c);
            if (factor.isZero()) continue;
            Fraction neg = -factor;
            M.addMulRow(i, r, neg);
            if (trace) trace->pushAddMulRow(i, r, neg, M);
        }
        ++r;
    }

    if (trace) trace->pushConclude("RREF 完成, 秩 = " + std::to_string(r), M);
    return r;
}

// det 核心实现: 前向消元到上三角, 累积符号与主元乘积
Fraction detImpl_(Matrix<Fraction>& M, StepSequence* trace) {
    if (!M.isSquare()) throw std::invalid_argument("det: matrix is not square");
    const std::size_t n = M.rows();
    if (trace) trace->pushInitial(M);

    int sign = 1;
    for (std::size_t c = 0; c < n; ++c) {
        std::size_t pivot = n;
        for (std::size_t i = c; i < n; ++i) {
            if (!M(i, c).isZero()) { pivot = i; break; }
        }
        if (pivot == n) {
            if (trace) trace->pushConclude("出现全零列, det = 0", M);
            return Fraction(0);
        }
        if (pivot != c) {
            M.swapRows(c, pivot);
            sign = -sign;
            if (trace) trace->pushSwapRows(c, pivot, M);
        }
        if (trace) trace->pushSelectPivot(c, c, M);

        const Fraction pivVal = M(c, c);
        for (std::size_t i = c + 1; i < n; ++i) {
            if (M(i, c).isZero()) continue;
            Fraction k = -M(i, c) / pivVal;
            M.addMulRow(i, c, k);
            if (trace) trace->pushAddMulRow(i, c, k, M);
        }
    }

    Fraction result = (sign == 1) ? Fraction(1) : Fraction(-1);
    for (std::size_t i = 0; i < n; ++i) result = result * M(i, i);
    if (trace) trace->pushConclude("det = " + result.toString(), M);
    return result;
}

// inverse 核心实现: [A | I] -> rref -> [I | A^-1]
Matrix<Fraction> inverseImpl_(const Matrix<Fraction>& M, StepSequence* trace) {
    if (!M.isSquare()) throw std::invalid_argument("inverse: matrix is not square");
    const std::size_t n = M.rows();
    Matrix<Fraction> aug = M.augment(Matrix<Fraction>::identity(n));
    rrefImpl_(aug, trace);

    for (std::size_t r = 0; r < n; ++r) {
        for (std::size_t c = 0; c < n; ++c) {
            Fraction expect = (r == c) ? Fraction(1) : Fraction(0);
            if (!(aug(r, c) == expect)) {
                if (trace) trace->pushConclude("左半不为单位阵, 矩阵奇异不可逆", aug);
                throw std::domain_error("inverse: singular matrix");
            }
        }
    }
    Matrix<Fraction> inv = aug.submatrix(0, n, n, n);
    if (trace) trace->pushConclude("逆矩阵取自右半块", inv);
    return inv;
}

// 扫描 RREF 后的矩阵, 收集每行主元列 (若无主元则为 cols)
void collectPivots_(const Matrix<Fraction>& R,
                    std::vector<std::size_t>& pivotCol,
                    std::vector<bool>& isPivotCol) {
    const std::size_t rows = R.rows();
    const std::size_t cols = R.cols();
    pivotCol.assign(rows, cols);
    isPivotCol.assign(cols, false);
    for (std::size_t r = 0; r < rows; ++r) {
        for (std::size_t c = 0; c < cols; ++c) {
            if (!R(r, c).isZero()) {
                pivotCol[r]   = c;
                isPivotCol[c] = true;
                break;
            }
        }
    }
}

// solve 核心实现: [A | b] -> rref, 分类讨论
SolveResult solveImpl_(const Matrix<Fraction>& A,
                       const Matrix<Fraction>& b,
                       StepSequence* trace) {
    if (A.rows() != b.rows() || b.cols() != 1)
        throw std::invalid_argument("solve: b must be (A.rows() x 1)");
    const std::size_t n = A.cols();

    Matrix<Fraction> M = A.augment(b);
    rrefImpl_(M, trace);

    std::vector<std::size_t> pivotCol;
    std::vector<bool>        isPivotCol;
    collectPivots_(M, pivotCol, isPivotCol);

    SolveResult result;
    // b 所在列 = 索引 n, 若成为主元列则方程组无解
    if (isPivotCol[n]) {
        result.hasSolution = false;
        if (trace) trace->pushConclude("增广列出现主元, 方程组无解", M);
        return result;
    }

    result.hasSolution = true;

    // 特解: 自由变量取 0, 主元行直接读出对应分量
    Matrix<Fraction> x(n, 1);
    for (std::size_t r = 0; r < M.rows(); ++r) {
        if (pivotCol[r] < n) x(pivotCol[r], 0) = M(r, n);
    }
    result.particular = x;

    // 零空间基: 每个自由列 (< n 且非主元列) 对应一个基向量
    std::vector<std::size_t> freeCols;
    for (std::size_t c = 0; c < n; ++c) {
        if (!isPivotCol[c]) freeCols.push_back(c);
    }
    Matrix<Fraction> N(n, freeCols.size());
    for (std::size_t k = 0; k < freeCols.size(); ++k) {
        std::size_t fc = freeCols[k];
        N(fc, k) = Fraction(1);
        for (std::size_t r = 0; r < M.rows(); ++r) {
            if (pivotCol[r] < n) N(pivotCol[r], k) = -M(r, fc);
        }
    }
    result.nullspaceBasis = N;

    if (trace) {
        std::string desc = freeCols.empty()
            ? "方程组有唯一解"
            : "方程组有无穷多解, 含 " + std::to_string(freeCols.size()) + " 个自由变量";
        trace->pushConclude(desc, M);
    }
    return result;
}

// nullspace 核心实现
Matrix<Fraction> nullspaceImpl_(const Matrix<Fraction>& M, StepSequence* trace) {
    const std::size_t n = M.cols();
    Matrix<Fraction> R = M;
    rrefImpl_(R, trace);

    std::vector<std::size_t> pivotCol;
    std::vector<bool>        isPivotCol;
    collectPivots_(R, pivotCol, isPivotCol);

    std::vector<std::size_t> freeCols;
    for (std::size_t c = 0; c < n; ++c) {
        if (!isPivotCol[c]) freeCols.push_back(c);
    }
    Matrix<Fraction> N(n, freeCols.size());
    for (std::size_t k = 0; k < freeCols.size(); ++k) {
        std::size_t fc = freeCols[k];
        N(fc, k) = Fraction(1);
        for (std::size_t r = 0; r < R.rows(); ++r) {
            if (pivotCol[r] < n) N(pivotCol[r], k) = -R(r, fc);
        }
    }
    if (trace) {
        std::string desc = freeCols.empty()
            ? "零空间仅含零向量"
            : "零空间维数 = " + std::to_string(freeCols.size());
        trace->pushConclude(desc, N);
    }
    return N;
}

} // anonymous namespace

std::size_t rref(Matrix<Fraction>& M)                        { return rrefImpl_(M, nullptr); }
std::size_t rref(Matrix<Fraction>& M, StepSequence& trace)   { return rrefImpl_(M, &trace); }

Matrix<Fraction> rrefOf(const Matrix<Fraction>& M) {
    Matrix<Fraction> copy = M;
    rrefImpl_(copy, nullptr);
    return copy;
}
Matrix<Fraction> rrefOf(const Matrix<Fraction>& M, StepSequence& trace) {
    Matrix<Fraction> copy = M;
    rrefImpl_(copy, &trace);
    return copy;
}

std::size_t rank(const Matrix<Fraction>& M) {
    Matrix<Fraction> copy = M;
    return rrefImpl_(copy, nullptr);
}
std::size_t rank(const Matrix<Fraction>& M, StepSequence& trace) {
    Matrix<Fraction> copy = M;
    return rrefImpl_(copy, &trace);
}

Fraction det(const Matrix<Fraction>& M) {
    Matrix<Fraction> copy = M;
    return detImpl_(copy, nullptr);
}
Fraction det(const Matrix<Fraction>& M, StepSequence& trace) {
    Matrix<Fraction> copy = M;
    return detImpl_(copy, &trace);
}

Matrix<Fraction> inverse(const Matrix<Fraction>& M)                      { return inverseImpl_(M, nullptr); }
Matrix<Fraction> inverse(const Matrix<Fraction>& M, StepSequence& trace) { return inverseImpl_(M, &trace); }

SolveResult solve(const Matrix<Fraction>& A, const Matrix<Fraction>& b) {
    return solveImpl_(A, b, nullptr);
}
SolveResult solve(const Matrix<Fraction>& A, const Matrix<Fraction>& b, StepSequence& trace) {
    return solveImpl_(A, b, &trace);
}

Matrix<Fraction> nullspace(const Matrix<Fraction>& M)                      { return nullspaceImpl_(M, nullptr); }
Matrix<Fraction> nullspace(const Matrix<Fraction>& M, StepSequence& trace) { return nullspaceImpl_(M, &trace); }

// charpoly / eigen

// Faddeev-LeVerrier:
// M_1 = A,               c_{n-1} = -tr(M_1)
// M_k = A (M_{k-1} + c_{n-k+1} I),  c_{n-k} = -tr(M_k) / k   (k = 2..n)
// p(x) = x^n + c_{n-1} x^{n-1} + ... + c_0
Polynomial<Fraction> charpoly(const Matrix<Fraction>& A) {
    if (!A.isSquare()) throw std::invalid_argument("charpoly: matrix must be square");
    const std::size_t n = A.rows();
    std::vector<Fraction> coef(n + 1, Fraction(0));
    coef[n] = Fraction(1);
    if (n == 0) return Polynomial<Fraction>(Fraction(1));

    Matrix<Fraction> I = Matrix<Fraction>::identity(n);
    Matrix<Fraction> M = A;

    // k = 1
    Fraction tr(0);
    for (std::size_t i = 0; i < n; ++i) tr = tr + M(i, i);
    coef[n - 1] = -tr;

    for (std::size_t k = 2; k <= n; ++k) {
        // M_k = A * (M + coef[n-k+1] * I)
        Matrix<Fraction> S = M;
        const Fraction& shift = coef[n - k + 1];
        for (std::size_t i = 0; i < n; ++i) S(i, i) = S(i, i) + shift;
        M = A * S;

        Fraction tk(0);
        for (std::size_t i = 0; i < n; ++i) tk = tk + M(i, i);
        coef[n - k] = -tk / Fraction(static_cast<long long>(k));
    }

    // 构造 Polynomial, 低位优先 {c_0, c_1, ..., c_n=1}
    std::vector<Fraction>& low = coef;
    Polynomial<Fraction> p;
    for (std::size_t i = 0; i <= n; ++i) {
        if (low[i].sign() != 0) {
            p = p + Polynomial<Fraction>::monomial(i, low[i]);
        }
    }
    return p;
}

std::vector<Fraction> rationalEigenvalues(const Matrix<Fraction>& A) {
    Polynomial<Fraction> p = charpoly(A);
    return rationalRoots(p);
}

std::vector<EigenPair> rationalEigenPairs(const Matrix<Fraction>& A) {
    if (!A.isSquare()) throw std::invalid_argument("rationalEigenPairs: matrix must be square");
    std::vector<Fraction> lambdas = rationalEigenvalues(A);
    std::vector<EigenPair> out;
    out.reserve(lambdas.size());

    const std::size_t n = A.rows();
    Matrix<Fraction> I = Matrix<Fraction>::identity(n);
    for (const Fraction& lam : lambdas) {
        // A - lam * I
        Matrix<Fraction> B = A;
        for (std::size_t i = 0; i < n; ++i) B(i, i) = B(i, i) - lam;
        Matrix<Fraction> basis = nullspaceImpl_(B, nullptr);
        EigenPair ep;
        ep.value = lam;
        ep.eigenspaceBasis = basis;
        out.push_back(ep);
    }
    return out;
}

// -------------------- Laplace / 余子式 / 伴随 --------------------

Matrix<Fraction> minorMatrix(const Matrix<Fraction>& M, std::size_t i, std::size_t j) {
    const std::size_t R = M.rows();
    const std::size_t C = M.cols();
    if (i >= R || j >= C) throw std::out_of_range("minorMatrix: index out of range");
    Matrix<Fraction> N(R - 1, C - 1);
    for (std::size_t r = 0, rr = 0; r < R; ++r) {
        if (r == i) continue;
        for (std::size_t c = 0, cc = 0; c < C; ++c) {
            if (c == j) continue;
            N(rr, cc) = M(r, c);
            ++cc;
        }
        ++rr;
    }
    return N;
}

Fraction cofactor(const Matrix<Fraction>& M, std::size_t i, std::size_t j) {
    if (!M.isSquare()) throw std::invalid_argument("cofactor: matrix must be square");
    Matrix<Fraction> sub = minorMatrix(M, i, j);
    Fraction d = det(sub);
    return ((i + j) % 2 == 0) ? d : -d;
}

Matrix<Fraction> adjugate(const Matrix<Fraction>& M) {
    if (!M.isSquare()) throw std::invalid_argument("adjugate: matrix must be square");
    const std::size_t n = M.rows();
    Matrix<Fraction> Adj(n, n);
    // adj(M)_{j,i} = cofactor(M, i, j)  (即转置代数余子式矩阵)
    for (std::size_t i = 0; i < n; ++i)
        for (std::size_t j = 0; j < n; ++j)
            Adj(j, i) = cofactor(M, i, j);
    return Adj;
}

static Fraction detLaplace_(const Matrix<Fraction>& M, std::size_t line, bool byRow) {
    if (!M.isSquare()) throw std::invalid_argument("detByExpansion: matrix must be square");
    const std::size_t n = M.rows();
    if (n == 0) return Fraction(1);
    if (n == 1) return M(0, 0);
    if (n == 2) return M(0,0) * M(1,1) - M(0,1) * M(1,0);
    if (line >= n) throw std::out_of_range("detByExpansion: line index out of range");
    Fraction s(0);
    for (std::size_t k = 0; k < n; ++k) {
        std::size_t i = byRow ? line : k;
        std::size_t j = byRow ? k    : line;
        const Fraction& a = M(i, j);
        if (a.isZero()) continue;
        Matrix<Fraction> sub = minorMatrix(M, i, j);
        Fraction d = detLaplace_(sub, 0, true);
        Fraction term = a * d;
        if ((i + j) % 2 == 0) s = s + term;
        else                  s = s - term;
    }
    return s;
}

Fraction detByRowExpansion(const Matrix<Fraction>& M, std::size_t row) {
    return detLaplace_(M, row, true);
}
Fraction detByColExpansion(const Matrix<Fraction>& M, std::size_t col) {
    return detLaplace_(M, col, false);
}

// -------------------- 相抵标准形 --------------------

// 思路:
//   1) 对增广 [A | I_m] 做行变换 → [R | P], R = P·A 为 RREF
//   2) R 是 RREF: 主元位置 (i, p_i), R 的第 p_i 列是 e_i, 非主元列是其线性组合
//   3) 对 R 做列变换 Q:
//      a) 对每个非主元列 c, 加上 -R(i, c) 倍的第 p_i 列 → 第 c 列全为 0
//      b) 将主元列 p_0, p_1, ..., p_{r-1} 依次交换到第 0, 1, ..., r-1 列
//   结果 R·Q = S = [I_r 0; 0 0], 且 P·A·Q = S
EquivalenceResult equivalentNormalForm(const Matrix<Fraction>& A) {
    const std::size_t m = A.rows();
    const std::size_t n = A.cols();

    // (1) 构造 [A | I_m], 做 RREF 后分出 R 和 P
    Matrix<Fraction> aug(m, n + m);
    for (std::size_t i = 0; i < m; ++i) {
        for (std::size_t j = 0; j < n; ++j) aug(i, j) = A(i, j);
        aug(i, n + i) = Fraction(1);
    }
    rrefImpl_(aug, nullptr);

    Matrix<Fraction> R(m, n);
    Matrix<Fraction> P(m, m);
    for (std::size_t i = 0; i < m; ++i) {
        for (std::size_t j = 0; j < n; ++j) R(i, j) = aug(i, j);
        for (std::size_t j = 0; j < m; ++j) P(i, j) = aug(i, n + j);
    }

    // 提取主元列
    std::vector<std::size_t> pivotCol;
    for (std::size_t i = 0; i < m; ++i) {
        for (std::size_t j = 0; j < n; ++j) {
            if (!R(i, j).isZero()) { pivotCol.push_back(j); break; }
        }
    }
    const std::size_t r = pivotCol.size();

    // (3a) 列消元: 对非主元列 c, 减去该列在各主元行上的线性组合
    Matrix<Fraction> Q = Matrix<Fraction>::identity(n);
    std::vector<bool> isPivot(n, false);
    for (std::size_t k = 0; k < r; ++k) isPivot[pivotCol[k]] = true;
    for (std::size_t c = 0; c < n; ++c) {
        if (isPivot[c]) continue;
        for (std::size_t k = 0; k < r; ++k) {
            Fraction f = R(k, c);
            if (f.isZero()) continue;
            // 列 c += -f * 列 pivotCol[k]
            std::size_t pk = pivotCol[k];
            for (std::size_t i = 0; i < m; ++i) R(i, c) = R(i, c) - f * R(i, pk);
            for (std::size_t i = 0; i < n; ++i) Q(i, c) = Q(i, c) - f * Q(i, pk);
        }
    }

    // (3b) 将主元列依次换到前 r 列
    for (std::size_t k = 0; k < r; ++k) {
        std::size_t pk = pivotCol[k];
        if (pk != k) {
            R.swapCols(k, pk);
            Q.swapCols(k, pk);
            // pivotCol 换新的 k 和 pk 映射
            for (std::size_t t = k + 1; t < r; ++t) {
                if (pivotCol[t] == k) { pivotCol[t] = pk; break; }
            }
        }
    }

    EquivalenceResult res;
    res.S    = R;
    res.P    = P;
    res.Q    = Q;
    res.rank = r;
    return res;
}

// ---------------- LU 分解 (Doolittle, 无主元交换) ----------------
LUResult luDecompose(const Matrix<Fraction>& A) {
    if (!A.isSquare())
        throw std::invalid_argument("luDecompose: matrix is not square");
    const std::size_t n = A.rows();
    Matrix<Fraction> L(n, n);
    Matrix<Fraction> U(n, n);
    for (std::size_t i = 0; i < n; ++i) L(i, i) = Fraction(1);
    for (std::size_t j = 0; j < n; ++j) {
        // U 的第 j 列上三角部分 (i = 0..j)
        for (std::size_t i = 0; i <= j; ++i) {
            Fraction s(0);
            for (std::size_t k = 0; k < i; ++k) s = s + L(i, k) * U(k, j);
            U(i, j) = A(i, j) - s;
        }
        if (U(j, j).isZero())
            throw std::domain_error(
                "luDecompose: 需要行交换或矩阵奇异, 当前实现不支持");
        // L 的第 j 列下三角部分 (i = j+1..n-1)
        for (std::size_t i = j + 1; i < n; ++i) {
            Fraction s(0);
            for (std::size_t k = 0; k < j; ++k) s = s + L(i, k) * U(k, j);
            L(i, j) = (A(i, j) - s) / U(j, j);
        }
    }
    return { std::move(L), std::move(U) };
}

}
