#pragma once

/*
* @file Matrix.h
* @brief 支持任意元素类型的行优先存储矩阵模板类
*
* 默认元素类型为 Fraction, 保证精确计算, 也可替换为 BigInt / double 等
* 提供元素访问、初等行/列变换、子矩阵、拼接、四则运算、转置、快速幂、对齐输出
* 高阶线性代数 (RREF / det / inv / solve / rank) 由 LinearAlgebra 单独提供
*
* @example
* Matrix<Fraction> A = {{1, 2}, {3, 4}};
* Matrix<Fraction> I = Matrix<Fraction>::identity(3);
* auto B = A.transpose();
* auto C = A * A;
* auto D = A.power(5);
* std::cout << C << '\n';
*/

#include "Fraction.h"
#include "Polynomial.h"

#include <cstddef>
#include <initializer_list>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace algemate::math {

// 矩阵类模板
template<typename T = Fraction>
class Matrix {
public:
    using value_type = T;
    using size_type  = std::size_t;

    Matrix() = default;                                           // 空矩阵 0 行 0 列
    Matrix(size_type rows, size_type cols);                       // rows * cols 矩阵, 默认初始化为 0
    Matrix(size_type rows, size_type cols, const T& fill);        // rows * cols 矩阵, 每个元素初始化为 fill
    Matrix(std::initializer_list<std::initializer_list<T>> init); // 嵌套初始化列表构造 如 {{1,2},{3,4}}

    static Matrix zeros(size_type rows, size_type cols);          // 全 0 矩阵
    static Matrix ones(size_type rows, size_type cols);           // 全 1 矩阵
    static Matrix identity(size_type n);                          // 单位矩阵
    static Matrix diagonal(std::initializer_list<T> diag);        // 对角矩阵

    size_type rows() const { return rows_; }                             // 返回行数
    size_type cols() const { return cols_; }                             // 返回列数
    bool      isEmpty() const  { return rows_ == 0 || cols_ == 0; }      // 判断是否为空矩阵
    bool      isSquare() const { return rows_ == cols_ && rows_ > 0; }   // 判断是否为方阵

    T&       at(size_type r, size_type c);     // 返回指定位置的元素 (带边界检查)
    const T& at(size_type r, size_type c) const;                   
    T&       operator()(size_type r, size_type c)       { return data_[idx_(r, c)]; } // 不带边界检查的元素访问
    const T& operator()(size_type r, size_type c) const { return data_[idx_(r, c)]; }

    // 初等行/列变换
    void swapRows(size_type i, size_type j);                     // 交换 i, j 两行
    void swapCols(size_type i, size_type j);                     // 交换 i, j 两列
    void scaleRow(size_type i, const T& k);                      // 第 i 行所有元素 *k
    void scaleCol(size_type j, const T& k);                      // 第 j 列所有元素 *k
    void addMulRow(size_type i, size_type j, const T& k);        // 第 i 行 += 第 j 行 * k
    void addMulCol(size_type i, size_type j, const T& k);        // 第 i 列 += 第 j 列 * k

    Matrix submatrix(size_type r0, size_type c0, size_type nr, size_type nc) const; // 子矩阵
    Matrix minor(size_type r, size_type c) const;        // 删除第 r 行和第 c 列后的余子式矩阵
    Matrix row(size_type r) const;                       // 返回第 r 行
    Matrix col(size_type c) const;                       // 返回第 c 列

    Matrix augment(const Matrix& right) const;  // 向右拼接矩阵
    Matrix stack(const Matrix& below)   const;  // 向下拼接矩阵

    // 高级构造
    Matrix kron(const Matrix& other) const;                          // 张量积 (Kronecker product)
    static Matrix blockDiag(std::initializer_list<Matrix> blocks);   // 分块对角 (直和)
    static Matrix companion(const Polynomial<T>& monic);             // 首一多项式的伴随矩阵

    // 结构查询
    T    trace() const;                                              // 对角元之和, 非方阵抛 std::invalid_argument
    bool isDiagonal()        const;                                  // 仅对角元可不为 0
    bool isUpperTriangular() const;                                  // 下三角 (包含对角线以下) 均为 0
    bool isLowerTriangular() const;                                  // 上三角 (包含对角线以上) 均为 0

    // 加减乘, 标量乘法
    Matrix operator-() const;
    Matrix operator+(const Matrix& r) const;
    Matrix operator-(const Matrix& r) const;
    Matrix operator*(const Matrix& r) const;
    Matrix operator*(const T& k) const;

    Matrix& operator+=(const Matrix& r);
    Matrix& operator-=(const Matrix& r);
    Matrix& operator*=(const Matrix& r);
    Matrix& operator*=(const T& k);

    // 比较
    bool operator==(const Matrix& r) const;
    bool operator!=(const Matrix& r) const { return !(*this == r); }

    Matrix transpose() const;              // 转置
    Matrix power(unsigned int exp) const;  // 快速幂

    std::string toString() const;          // 转为字符串

private:
    size_type      rows_ = 0;
    size_type      cols_ = 0;
    std::vector<T> data_;

    size_type idx_(size_type r, size_type c) const { return r * cols_ + c; }
    void      checkSameShape_(const Matrix& o, const char* op) const;
};

template<typename T>
Matrix<T>::Matrix(size_type rows, size_type cols)
    : rows_(rows), cols_(cols), data_(rows * cols) {}

template<typename T>
Matrix<T>::Matrix(size_type rows, size_type cols, const T& fill)
    : rows_(rows), cols_(cols), data_(rows * cols, fill) {}

template<typename T>
Matrix<T>::Matrix(std::initializer_list<std::initializer_list<T>> init) {
    rows_ = init.size();
    cols_ = rows_ > 0 ? init.begin()->size() : 0;
    data_.reserve(rows_ * cols_);
    for (const auto& row : init) {
        if (row.size() != cols_)
            throw std::invalid_argument("Matrix: inconsistent row size");
        for (const auto& v : row) data_.push_back(v);
    }
}

template<typename T>
Matrix<T> Matrix<T>::zeros(size_type rows, size_type cols) {
    return Matrix(rows, cols);
}

template<typename T>
Matrix<T> Matrix<T>::ones(size_type rows, size_type cols) {
    return Matrix(rows, cols, T(1));
}

template<typename T>
Matrix<T> Matrix<T>::identity(size_type n) {
    Matrix m(n, n);
    for (size_type i = 0; i < n; ++i) m.data_[i * n + i] = T(1);
    return m;
}

template<typename T>
Matrix<T> Matrix<T>::diagonal(std::initializer_list<T> diag) {
    size_type n = diag.size();
    Matrix m(n, n);
    size_type i = 0;
    for (const auto& v : diag) { m.data_[i * n + i] = v; ++i; }
    return m;
}

template<typename T>
T& Matrix<T>::at(size_type r, size_type c) {
    if (r >= rows_ || c >= cols_) throw std::out_of_range("Matrix::at");
    return data_[idx_(r, c)];
}

template<typename T>
const T& Matrix<T>::at(size_type r, size_type c) const {
    if (r >= rows_ || c >= cols_) throw std::out_of_range("Matrix::at");
    return data_[idx_(r, c)];
}

template<typename T>
void Matrix<T>::checkSameShape_(const Matrix& o, const char* op) const {
    if (rows_ != o.rows_ || cols_ != o.cols_) {
        std::string msg = "Matrix: shape mismatch in ";
        msg += op;
        throw std::invalid_argument(msg);
    }
}

template<typename T>
void Matrix<T>::swapRows(size_type i, size_type j) {
    if (i >= rows_ || j >= rows_) throw std::out_of_range("Matrix::swapRows");
    if (i == j) return;
    for (size_type c = 0; c < cols_; ++c) {
        using std::swap;
        swap(data_[idx_(i, c)], data_[idx_(j, c)]);
    }
}

template<typename T>
void Matrix<T>::swapCols(size_type i, size_type j) {
    if (i >= cols_ || j >= cols_) throw std::out_of_range("Matrix::swapCols");
    if (i == j) return;
    for (size_type r = 0; r < rows_; ++r) {
        using std::swap;
        swap(data_[idx_(r, i)], data_[idx_(r, j)]);
    }
}

template<typename T>
void Matrix<T>::scaleRow(size_type i, const T& k) {
    if (i >= rows_) throw std::out_of_range("Matrix::scaleRow");
    for (size_type c = 0; c < cols_; ++c) data_[idx_(i, c)] = data_[idx_(i, c)] * k;
}

template<typename T>
void Matrix<T>::scaleCol(size_type j, const T& k) {
    if (j >= cols_) throw std::out_of_range("Matrix::scaleCol");
    for (size_type r = 0; r < rows_; ++r) data_[idx_(r, j)] = data_[idx_(r, j)] * k;
}

template<typename T>
void Matrix<T>::addMulRow(size_type i, size_type j, const T& k) {
    if (i >= rows_ || j >= rows_) throw std::out_of_range("Matrix::addMulRow");
    for (size_type c = 0; c < cols_; ++c)
        data_[idx_(i, c)] = data_[idx_(i, c)] + k * data_[idx_(j, c)];
}

template<typename T>
void Matrix<T>::addMulCol(size_type i, size_type j, const T& k) {
    if (i >= cols_ || j >= cols_) throw std::out_of_range("Matrix::addMulCol");
    for (size_type r = 0; r < rows_; ++r)
        data_[idx_(r, i)] = data_[idx_(r, i)] + k * data_[idx_(r, j)];
}

template<typename T>
Matrix<T> Matrix<T>::submatrix(size_type r0, size_type c0, size_type nr, size_type nc) const {
    if (r0 + nr > rows_ || c0 + nc > cols_)
        throw std::out_of_range("Matrix::submatrix");
    Matrix out(nr, nc);
    for (size_type r = 0; r < nr; ++r)
        for (size_type c = 0; c < nc; ++c)
            out.data_[r * nc + c] = data_[idx_(r0 + r, c0 + c)];
    return out;
}

template<typename T>
Matrix<T> Matrix<T>::minor(size_type r, size_type c) const {
    if (rows_ == 0 || cols_ == 0) throw std::invalid_argument("Matrix::minor: empty");
    if (r >= rows_ || c >= cols_) throw std::out_of_range("Matrix::minor");
    Matrix out(rows_ - 1, cols_ - 1);
    size_type oi = 0;
    for (size_type i = 0; i < rows_; ++i) {
        if (i == r) continue;
        size_type oj = 0;
        for (size_type j = 0; j < cols_; ++j) {
            if (j == c) continue;
            out.data_[oi * (cols_ - 1) + oj] = data_[idx_(i, j)];
            ++oj;
        }
        ++oi;
    }
    return out;
}

template<typename T>
Matrix<T> Matrix<T>::row(size_type r) const {
    if (r >= rows_) throw std::out_of_range("Matrix::row");
    Matrix out(1, cols_);
    for (size_type c = 0; c < cols_; ++c) out.data_[c] = data_[idx_(r, c)];
    return out;
}

template<typename T>
Matrix<T> Matrix<T>::col(size_type c) const {
    if (c >= cols_) throw std::out_of_range("Matrix::col");
    Matrix out(rows_, 1);
    for (size_type r = 0; r < rows_; ++r) out.data_[r] = data_[idx_(r, c)];
    return out;
}

template<typename T>
Matrix<T> Matrix<T>::augment(const Matrix& right) const {
    if (rows_ != right.rows_)
        throw std::invalid_argument("Matrix::augment: row count mismatch");
    Matrix out(rows_, cols_ + right.cols_);
    for (size_type r = 0; r < rows_; ++r) {
        for (size_type c = 0; c < cols_; ++c)
            out.data_[r * out.cols_ + c] = data_[idx_(r, c)];
        for (size_type c = 0; c < right.cols_; ++c)
            out.data_[r * out.cols_ + cols_ + c] = right.data_[r * right.cols_ + c];
    }
    return out;
}

template<typename T>
Matrix<T> Matrix<T>::stack(const Matrix& below) const {
    if (cols_ != below.cols_)
        throw std::invalid_argument("Matrix::stack: col count mismatch");
    Matrix out(rows_ + below.rows_, cols_);
    for (size_type r = 0; r < rows_; ++r)
        for (size_type c = 0; c < cols_; ++c)
            out.data_[r * cols_ + c] = data_[idx_(r, c)];
    for (size_type r = 0; r < below.rows_; ++r)
        for (size_type c = 0; c < cols_; ++c)
            out.data_[(rows_ + r) * cols_ + c] = below.data_[r * cols_ + c];
    return out;
}

// 张量积: (A ⊗ B)[i*Br + p, j*Bc + q] = A[i,j] * B[p,q]
template<typename T>
Matrix<T> Matrix<T>::kron(const Matrix& other) const {
    const size_type br = other.rows_;
    const size_type bc = other.cols_;
    Matrix out(rows_ * br, cols_ * bc);
    for (size_type i = 0; i < rows_; ++i) {
        for (size_type j = 0; j < cols_; ++j) {
            const T& a = data_[idx_(i, j)];
            for (size_type p = 0; p < br; ++p) {
                for (size_type q = 0; q < bc; ++q) {
                    out.data_[(i * br + p) * out.cols_ + (j * bc + q)] = a * other.data_[p * bc + q];
                }
            }
        }
    }
    return out;
}

// 分块对角: 每块必须为方阵或至少不空, 非方阵块亦允许
// 输出尺寸 = 所有块行和 x 列和, 其余位置填 0
template<typename T>
Matrix<T> Matrix<T>::blockDiag(std::initializer_list<Matrix> blocks) {
    size_type totalRows = 0, totalCols = 0;
    for (const auto& b : blocks) { totalRows += b.rows_; totalCols += b.cols_; }
    Matrix out(totalRows, totalCols);
    size_type ro = 0, co = 0;
    for (const auto& b : blocks) {
        for (size_type r = 0; r < b.rows_; ++r)
            for (size_type c = 0; c < b.cols_; ++c)
                out.data_[(ro + r) * totalCols + (co + c)] = b.data_[r * b.cols_ + c];
        ro += b.rows_;
        co += b.cols_;
    }
    return out;
}

// 首一多项式 p(x) = x^n + c_{n-1} x^{n-1} + ... + c_0 的伴随矩阵 (n >= 1)
// 友矩阵形式: 最后一列放 -c_i, 次对角 1, 其余 0
template<typename T>
Matrix<T> Matrix<T>::companion(const Polynomial<T>& monic) {
    if (monic.isZero()) throw std::invalid_argument("Matrix::companion: zero polynomial");
    if (!(monic.leading() == T(1))) throw std::invalid_argument("Matrix::companion: not monic");
    int d = monic.degree();
    if (d < 1) throw std::invalid_argument("Matrix::companion: degree must be >= 1");
    size_type n = static_cast<size_type>(d);
    Matrix out(n, n);
    for (size_type i = 0; i + 1 < n; ++i) out.data_[(i + 1) * n + i] = T(1);
    for (size_type i = 0; i < n; ++i) out.data_[i * n + (n - 1)] = -monic[i];
    return out;
}

template<typename T>
T Matrix<T>::trace() const {
    if (!isSquare()) throw std::invalid_argument("Matrix::trace: not square");
    T s = T(0);
    for (size_type i = 0; i < rows_; ++i) s = s + data_[idx_(i, i)];
    return s;
}

template<typename T>
bool Matrix<T>::isDiagonal() const {
    if (!isSquare()) return false;
    for (size_type r = 0; r < rows_; ++r)
        for (size_type c = 0; c < cols_; ++c)
            if (r != c && !(data_[idx_(r, c)] == T(0))) return false;
    return true;
}

template<typename T>
bool Matrix<T>::isUpperTriangular() const {
    if (!isSquare()) return false;
    for (size_type r = 0; r < rows_; ++r)
        for (size_type c = 0; c < r; ++c)
            if (!(data_[idx_(r, c)] == T(0))) return false;
    return true;
}

template<typename T>
bool Matrix<T>::isLowerTriangular() const {
    if (!isSquare()) return false;
    for (size_type r = 0; r < rows_; ++r)
        for (size_type c = r + 1; c < cols_; ++c)
            if (!(data_[idx_(r, c)] == T(0))) return false;
    return true;
}

template<typename T>
Matrix<T> Matrix<T>::operator-() const {
    Matrix out(rows_, cols_);
    for (size_type i = 0; i < data_.size(); ++i) out.data_[i] = -data_[i];
    return out;
}

template<typename T>
Matrix<T> Matrix<T>::operator+(const Matrix& r) const {
    checkSameShape_(r, "operator+");
    Matrix out(rows_, cols_);
    for (size_type i = 0; i < data_.size(); ++i) out.data_[i] = data_[i] + r.data_[i];
    return out;
}

template<typename T>
Matrix<T> Matrix<T>::operator-(const Matrix& r) const {
    checkSameShape_(r, "operator-");
    Matrix out(rows_, cols_);
    for (size_type i = 0; i < data_.size(); ++i) out.data_[i] = data_[i] - r.data_[i];
    return out;
}

template<typename T>
Matrix<T> Matrix<T>::operator*(const Matrix& r) const {
    if (cols_ != r.rows_)
        throw std::invalid_argument("Matrix::operator*: inner dimension mismatch");
    Matrix out(rows_, r.cols_);
    for (size_type i = 0; i < rows_; ++i) {
        for (size_type k = 0; k < cols_; ++k) {
            const T& a = data_[idx_(i, k)];
            for (size_type j = 0; j < r.cols_; ++j)
                out.data_[i * r.cols_ + j] = out.data_[i * r.cols_ + j] + a * r.data_[k * r.cols_ + j];
        }
    }
    return out;
}

template<typename T>
Matrix<T> Matrix<T>::operator*(const T& k) const {
    Matrix out(rows_, cols_);
    for (size_type i = 0; i < data_.size(); ++i) out.data_[i] = data_[i] * k;
    return out;
}

template<typename T>
Matrix<T>& Matrix<T>::operator+=(const Matrix& r) { *this = *this + r; return *this; }

template<typename T>
Matrix<T>& Matrix<T>::operator-=(const Matrix& r) { *this = *this - r; return *this; }

template<typename T>
Matrix<T>& Matrix<T>::operator*=(const Matrix& r) { *this = *this * r; return *this; }

template<typename T>
Matrix<T>& Matrix<T>::operator*=(const T& k) { *this = *this * k; return *this; }

template<typename T>
bool Matrix<T>::operator==(const Matrix& r) const {
    if (rows_ != r.rows_ || cols_ != r.cols_) return false;
    for (size_type i = 0; i < data_.size(); ++i)
        if (!(data_[i] == r.data_[i])) return false;
    return true;
}

template<typename T>
Matrix<T> Matrix<T>::transpose() const {
    Matrix out(cols_, rows_);
    for (size_type r = 0; r < rows_; ++r)
        for (size_type c = 0; c < cols_; ++c)
            out.data_[c * rows_ + r] = data_[idx_(r, c)];
    return out;
}

template<typename T>
Matrix<T> Matrix<T>::power(unsigned int exp) const {
    if (!isSquare()) throw std::invalid_argument("Matrix::power: not square");
    Matrix result = identity(rows_);
    Matrix base = *this;
    while (exp != 0) {
        if (exp & 1u) result = result * base;
        exp >>= 1u;
        if (exp != 0) base = base * base;
    }
    return result;
}

template<typename T>
std::string Matrix<T>::toString() const {
    if (isEmpty()) return "[]";
    std::vector<std::string> cells(rows_ * cols_);
    std::vector<size_type> width(cols_, 0);
    for (size_type r = 0; r < rows_; ++r) {
        for (size_type c = 0; c < cols_; ++c) {
            std::ostringstream oss;
            oss << data_[idx_(r, c)];
            cells[idx_(r, c)] = oss.str();
            if (cells[idx_(r, c)].size() > width[c]) width[c] = cells[idx_(r, c)].size();
        }
    }
    std::ostringstream out;
    for (size_type r = 0; r < rows_; ++r) {
        out << '[';
        for (size_type c = 0; c < cols_; ++c) {
            if (c > 0) out << ' ';
            const std::string& s = cells[idx_(r, c)];
            for (size_type k = s.size(); k < width[c]; ++k) out << ' ';
            out << s;
        }
        out << ']';
        if (r + 1 < rows_) out << '\n';
    }
    return out.str();
}

template<typename T>
Matrix<T> operator*(const T& k, const Matrix<T>& m) { return m * k; }

template<typename T>
std::ostream& operator<<(std::ostream& os, const Matrix<T>& m) { return os << m.toString(); }

}
