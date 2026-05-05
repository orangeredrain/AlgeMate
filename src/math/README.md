# 数学库

纯 C++17 数学核心库，依赖仅 STL，不依赖 Qt。命名空间 `algemate::math`。

## 目录结构

```
math/
├── core/          # 基础类型：整数、有理数、代数数、复数、多项式、矩阵
├── algorithm/     # 高级算法：线性代数、特征系统、规范形、因式分解、SVD
├── mpoly/         # 多元多项式与对称函数
└── trace/         # 算法步骤追踪（用于分步可视化）
```

## 核心类型（`core/`）

### BigInt
任意精度有符号整数（`uint32_t` limbs）。

```cpp
BigInt a("12345678901234567890");
BigInt b = a * a;
BigInt g = gcd(a, b);
QString s = b.toString();
```

### Fraction
基于 BigInt 的精确有理数，自动约分。

```cpp
Fraction a(2, 3), b(4, 5);
Fraction c = a + b;           // 自动约分为 22/15
double d = c.toDouble();
QString s = c.toLatex();      // "\frac{22}{15}"
```

### AlgReal
代数实数，表示为最小多项式 + Sturm 隔离区间。

```cpp
AlgReal r = AlgReal::sqrt(Fraction(2));    // √2
AlgReal r2 = AlgReal::sqrt(Fraction(3));   // √3
AlgReal sum = r + r2;                      // √2 + √3, 自动构造最小多项式
AlgReal p = AlgReal::cbrt(Fraction(5));    // ³√5
double approx = r.toDouble();              // 1.41421356...
QString s = r.toLatex();                   // "\sqrt{2}"
```

### Complex
以 AlgReal 为实部/虚部的精确复数。

```cpp
Complex z(Fraction(3), Fraction(4));   // 3 + 4i
Complex i = Complex::i();             // i
Complex w = z * z;                    // -7 + 24i
Complex s = Complex::sqrt(z);          // 复数平方根
double n = z.modulus();               // 5.0
```

### Polynomial\<T\>
稠密单变量多项式模板（默认 `T = Fraction`）。

```cpp
Polynomial p({1, 2, 3});   // 1 + 2x + 3x²
Polynomial q = p * p;      // 1 + 4x + 10x² + 12x³ + 9x⁴
Polynomial d = gcd(p, q);  // 1 + 2x + 3x²
Fraction v = p.evaluate(Fraction(1, 2));
Polynomial dp = p.derivative();
```

### PolynomialZp
有限域 `F_p` 上的多项式，支持完整因式分解（Cantor-Zassenhaus）。

```cpp
PolynomialZp p({1, 0, -1}, 7);   // x² - 1 mod 7
auto factors = p.factor();       // [(x-1) * (x+1)]
```

### Matrix\<T\>
稠密行优先矩阵模板（默认 `T = Fraction`）。

```cpp
Matrix m(3, 3, {1,2,3, 4,5,6, 7,8,9});
Matrix id = Matrix<>::identity(3);
Matrix b = m.transpose();
Matrix c = m * b;
Fraction tr = m.trace();
Fraction d = m.det();             // 依赖算法函数
```

### SymbolicExpr
符号表达式 AST，支持构造、求导、代入、求值。

```cpp
SymbolicExpr x("x"), y("y");
SymbolicExpr e = x*x + 2*x*y + y*y;          // x² + 2xy + y²
SymbolicExpr f = e.diff(x);                   // 2x + 2y
SymbolicExpr g = e.subs("x", Fraction(1));    // 1 + 2y + y²
Complex v = g.evaluate();                     // y 不是自由变量则求值
```

### NumberFormatter
分数与十进制的双向转换。

```cpp
Fraction f(1, 3);
QString dec = NumberFormatter::toRepeatingDecimal(f, 10);  // "0.(3)"
Fraction back = NumberFormatter::fromDecimal("0.142857");
```

## 算法（`algorithm/`）

所有算法均为 `algemate::math` 命名空间下的自由函数，大部分提供"无追踪"和"带 StepSequence"两个重载。

### 线性代数

```cpp
Matrix<> rref = rref(A);                              // 行最简形
int r = rank(A);                                      // 秩
Fraction d = det(A);                                  // 行列式 (Faddeev-LeVerrier)
auto [P, L, U] = luDecompose(A);                      // LU 分解
auto [sol, nullspace] = solve(A, b);                 // 求解 Ax=b (特解+零空间)
Matrix<> inv = inverse(A);                            // 逆矩阵
Polynomial<> cp = charpoly(A);                        // 特征多项式
```

### 多项式算法

```cpp
auto factors = squarefreeFactorization(p);            // 无平方分解
Polynomial<> res = resultant(p, q);                  // 结式
auto roots = rationalRoots(p);                        // 有理根
int n = countRealRootsInInterval(p, a, b);           // Sturm 实根计数
bool irreducible = isIrreducibleOverQ(p);             // Q[x] 不可约判定
auto fac = factorOverQ(p);                            // Q[x] 因式分解 (Hensel+Zassenhaus)
```

### Lambda 矩阵、规范形

```cpp
auto [U, S, V] = smithNormalForm(lambdaMatrix);       // Smith 标准形
auto factors = invariantFactors(lambdaMatrix);        // 不变因子
auto [U, V] = equivalentNormalForm(A);                // 相抵标准形 P·A·Q = S

auto [F, inv, minpoly] = rationalCanonicalForm(A);    // Frobenius 标准形
Polynomial<> mp = minimalPolynomial(A);               // 最小多项式

auto [J, Q] = jordanForm(A);                          // Jordan 标准形 (复域)
auto [J, Q] = jordanFormReal(A);                      // Jordan 标准形 (实域, AlgReal)
```

### 特征系统

```cpp
auto vals = rationalEigenvalues(A);                   // 有理特征值
auto pairs = rationalEigenPairs(A);                   // 有理特征对
auto result = complexEigenvalues(A);                  // 复特征值
auto result = realEigenvalues(A);                     // 实特征值 (AlgReal)
auto pairs = realEigenPairs(A);                       // 实特征对
```

### 正交化、二次型、对角化

```cpp
auto basis = gramSchmidt(vectors);                     // Gram-Schmidt 正交化
auto [D, P] = congruenceDiagonalize(A);               // 合同对角化 D = PᵀAP
auto [p, q, r] = quadraticSignature(A);              // 惯性指数
bool pd = isPositiveDefinite(A);                      // 正定性

auto [U, Lambda] = orthogonalDiagonalize(A);          // 正交对角化 UᵀAU = Λ
auto [Q, R] = qrDecompose(A);                         // QR 分解
```

### SVD

```cpp
auto [U, Sigma, V] = svdDecompose(A);                 // A = U Σ Vᵀ (精确 AlgReal)
```

## 多元多项式（`mpoly/`）

命名空间 `algemate::math::mpoly`。

```cpp
Monomial m1({{0, 2}, {1, 1}});    // x²y
Monomial m2({{0, 1}, {1, 2}});    // xy²
MPolynomial p;
p += MPolynomial(Fraction(3), m1);
p += MPolynomial(Fraction(1), m2); // 3x²y + xy²
Fraction leading = p.leadingCoefficient();

// 对称多项式
auto s = powerSumToSym(4);        // 前4个幂和 → 初等对称多项式
auto reduced = reduceSymmetric(f); // 对称多项式降阶
```

## 追踪（`trace/`）

用于算法步骤可视化。`StepSequence` 收集算法执行过程中的每一步（交换行、倍乘、消元等），包含操作描述和矩阵快照。

```cpp
StepSequence trace;
rrefOf(A, trace);                             // 带追踪的 RREF
for (int i = 0; i < trace.size(); ++i) {
    Step step = trace.steps()[i];
    if (step.kind == StepKind::AddMulRow)
        qDebug() << "r" << step.i << "+=" << step.k.toDouble() << "*r" << step.j;
}
```

## 依赖

纯 C++17，仅 STL。无 Qt 依赖。

```
algemate_math (STATIC)
  └── STL only: <vector> <string> <cstdint> <map> <set> <cmath> ...
```
