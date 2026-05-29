#include "QuestionBank.h"

QVector<Question> QuestionBank::getAllQuestions() {
    QVector<Question> list;

    // ==========================================
    // 第 1 章 多项式 (ch01_1 ~ ch01_5)
    // ==========================================

    // -- 1.1 整除与带余除法 --
    list.append(Question{111, "ch01_1.md", "【单选题】设多项式 $f(x) = x^4 + 3x^3 + x^2 - 1$ 与 $g(x) = x^2 + 2x - 1$，用 $g(x)$ 除 $f(x)$ 得到的余式为：", QuestionType::Single, 5, "2", {"$2x + 1$", "$-x + 3$", "$x - 1$", "$0$"}, 0, "", false});
    list.append(Question{112, "ch01_1.md", "【填空题】若多项式 $g(x) = x - 2$ 能整除 $f(x) = x^3 - kx^2 + 4$，则常数 $k$ 的值为：", QuestionType::Fill, 5, "3", {}, 0, "", false});
    list.append(Question{113, "ch01_1.md", "【主观题】已知 $f(x), g(x) \\in \\mathbb{F}[x]$ 且 $g(x) \\neq 0$。证明：带余除法 $f(x) = q(x)g(x) + r(x)$ 中的商与余式是唯一确定的。", QuestionType::Subjective, 10, "设存在两组解，相减得 $(q_1-q_2)g = r_2-r_1$。若商不相等，左边次数 $\\ge \\deg(g)$，而右边次数 $< \\deg(g)$，产生矛盾。故唯一性成立。", {}, 0, "", false});

    // -- 1.2 最大公因式 --
    list.append(Question{121, "ch01_2.md", "【单选题】若 $\\gcd(f(x), g(x)) = 1$，则下列关于公因式的命题中错误的是：", QuestionType::Single, 5, "2", {"存在 $u(x),v(x)$ 使得 $u(x)f(x)+v(x)g(x)=1$", "$\\gcd(f(x)^2, g(x)) = 1$", "$\\gcd(f(x)+g(x), f(x)-g(x)) = 1$ 恒成立", "$\\gcd(f(x), f(x)+g(x)) = 1$"}, 0, "", false});
    list.append(Question{122, "ch01_2.md", "【填空题】多项式 $f(x) = x^3 - 3x - 2$ 与 $g(x) = x^2 - 4$ 的首一最大公因式为 $x - a$，其中 $a$ 是：", QuestionType::Fill, 5, "2", {}, 0, "", false});
    list.append(Question{123, "ch01_2.md", "【主观题】试用辗转相除法（欧几里得算法）求 $f(x)=x^3-1$ 与 $g(x)=x^2-1$ 的最大公因式。", QuestionType::Subjective, 10, "因 $x^3-1 = x(x^2-1) + (x-1)$，且 $x^2-1 = (x+1)(x-1)$。最后一项非零余式首一化即为 $x-1$。", {}, 0, "", false});

    // -- 1.3 不可约多项式与唯一分解定理 --
    list.append(Question{131, "ch01_3.md", "【单选题】多项式 $x^2 + 1$ 在下列哪一个数域上是可约的：", QuestionType::Single, 5, "1", {"有理数域 $\\mathbb{Q}$", "复数域 $\\mathbb{C}$", "实数域 $\\mathbb{R}$"}, 0, "", false});
    list.append(Question{132, "ch01_3.md", "【填空题】根据唯一分解定理，多项式 $f(x) = x^4 - 1$ 在实数域 $\\mathbb{R}$ 上的不可约因式个数为：", QuestionType::Fill, 5, "3", {}, 0, "", false});
    list.append(Question{133, "ch01_3.md", "【主观题】证明：若 $p(x)$ 是不可约多项式，且 $p(x) \\mid f(x)g(x)$，则 $p(x) \\mid f(x)$ 或 $p(x) \\mid g(x)$。", QuestionType::Subjective, 10, "若 $p(x) \\nmid f(x)$，因 $p(x)$ 不可约，必有 $\\gcd(p(x), f(x))=1$。由裴蜀定理存在 $u(x)p(x)+v(x)f(x)=1$，两边同乘 $g(x)$ 易证 $p(x) \\mid g(x)$。", {}, 0, "", false});

    // -- 1.4 重因式 --
    list.append(Question{141, "ch01_4.md", "【单选题】若 $f(x)$ 有 $k$ 重因式 $p(x)$，则其导数多项式 $f'(x)$ 含有 $p(x)$ 的重数为：", QuestionType::Single, 5, "1", {"$k$ 重", "$k-1$ 重", "$k+1$ 重", "不含有该因式"}, 0, "", false});
    list.append(Question{142, "ch01_4.md", "【填空题】已知多项式 $f(x) = x^3 - 3x^2 + 4$ 存在一个双重根，则该重根的数值为：", QuestionType::Fill, 5, "2", {}, 0, "", false});
    list.append(Question{143, "ch01_4.md", "【主观题】证明：多项式 $f(x)$ 没有重因式的充分必要条件是 $\\gcd(f(x), f'(x)) = 1$。", QuestionType::Subjective, 10, "若有重因式 $p(x)$，则 $p^{k-1}$ 为其公因式，公因式次数 $\\ge 1$，矛盾。反之若有非数公因式，其不可约因式必为重因式。", {}, 0, "", false});

    // -- 1.5 n元多项式环与对称多项式 --
    list.append(Question{151, "ch01_5.md", "【单选题】下列关于三元多项式的函数中，哪一个是初等对称多项式 $\\sigma_2(x_1, x_2, x_3)$ 的定义式：", QuestionType::Single, 5, "2", {"$x_1+x_2+x_3$", "$x_1x_2x_3$", "$x_1x_2+x_2x_3+x_3x_1$", "$x_1^2+x_2^2+x_3^2$"}, 0, "", false});
    list.append(Question{152, "ch01_5.md", "【填空题】将对称多项式 $f(x,y) = x^2 + y^2$ 表示为初等对称多项式的组合 $\\sigma_1^2 - k\\sigma_2$，则常数 $k$ 为：", QuestionType::Fill, 5, "2", {}, 0, "", false});
    list.append(Question{153, "ch01_5.md", "【主观题】简述对称多项式基本定理的核心结论及其理论应用价值。", QuestionType::Subjective, 10, "核心结论：任一数域上的对称多项式均可唯一表为初等对称多项式的多项式。应用价值在于建立了方程的根与系数的通用纽带（韦达定理）。", {}, 0, "", false});


    // ==========================================
    // 第 2 章 行列式 (ch02_1 ~ ch02_2)
    // ==========================================

    // -- 2.1 行列式的定义 --
    list.append(Question{211, "ch02_1.md", "【单选题】在 4 阶行列式展开式中，项 $a_{13}a_{24}a_{31}a_{42}$ 前面的符号为：", QuestionType::Single, 5, "0", {"正号 (+)", "负号 (-)"}, 0, "", false});
    list.append(Question{212, "ch02_1.md", "【填空题】排列 $4, 3, 1, 2$ 的全局逆序数大小为：", QuestionType::Fill, 5, "5", {}, 0, "", false});
    list.append(Question{213, "ch02_1.md", "【主观题】利用行列式的行对换取反性质，证明：若行列式中有两行完全相同，则该行列式的值必为 0。", QuestionType::Subjective, 10, "对换两张相同的行，行列式符号变反（$D = -D$）。但由于两行无区别，行列式本身未变，因此 $2D = 0 \\implies D = 0$。", {}, 0, "", false});

    // -- 2.2 克拉默法则与拉普拉斯定理 --
    list.append(Question{221, "ch02_2.md", "【单选题】关于包含 $n$ 个方程的非齐次线性方程组 $Ax = b$，若系数行列式 $|A| \\neq 0$，则：", QuestionType::Single, 5, "0", {"方程组有唯一解，且可用克拉默法则表示", "方程组有无穷多解", "方程组必定无解"}, 0, "", false});
    list.append(Question{222, "ch02_2.md", "【填空题】计算四阶斜对角行列式：$\\begin{vmatrix} 0&0&0&2\\\\0&0&3&0\\\\0&4&0&0\\\\5&0&0&0 \\end{vmatrix}$ 的数值结果为：", QuestionType::Fill, 5, "120", {}, 0, "", false});
    list.append(Question{223, "ch02_2.md", "【主观题】利用拉普拉斯定理计算如下分块矩阵的行列式值：$$\\det(A) = \\begin{vmatrix} 1 & 2 & 0 & 0 \\\\ 3 & 4 & 0 & 0 \\\\ 5 & 6 & 7 & 8 \\\\ 9 & 1 & 2 & 3 \\end{vmatrix}$$", QuestionType::Subjective, 10, "按前两行展开：$\\det(A) = \\begin{vmatrix} 1&2\\\\3&4 \\end{vmatrix} \\cdot \\begin{vmatrix} 7&8\\\\2&3 \\end{vmatrix} = (4-6)\\times(21-16) = -2 \\times 5 = -10$。", {}, 0, "", false});


    // ==========================================
    // 第 3 章 n维向量与向量空间 (ch03_1 ~ ch03_5)
    // ==========================================

    // -- 3.1 n维向量与向量空间 --
    list.append(Question{311, "ch03_1.md", "【单选题】设 $V$ 构成实数域上的向量空间，下列哪项不属于线性空间运算律：", QuestionType::Single, 5, "3", {"加法交换律", "数乘结合律", "存在零元", "向量乘法的交换律"}, 0, "", false});
    list.append(Question{312, "ch03_1.md", "【填空题】已知 $\\alpha = (1, 2, 3)^T, \\beta = (2, -1, 4)^T$，则线性组合 $2\\alpha - \\beta$ 的第三分量值为：", QuestionType::Fill, 5, "2", {}, 0, "", false});
    list.append(Question{313, "ch03_1.md", "【主观题】简述向量空间子空间的判定准则。", QuestionType::Subjective, 10, "非空子集 $W \\subseteq V$ 构成子空间，只需对加法和数乘两项基本运算封闭即可。", {}, 0, "", false});

    // -- 3.2 极大线性无关组 --
    list.append(Question{321, "ch03_2.md", "【单选题】已知向量组 $\\alpha_1 = (1, 1, 0)^T, \\alpha_2 = (0, 1, 1)^T, \\alpha_3 = (1, 2, 1)^T$，则该向量组的极大线性无关组：", QuestionType::Single, 5, "1", {"是 $\\{\\alpha_1, \\alpha_2, \\alpha_3\\}$", "可以是 $\\{\\alpha_1, \\alpha_2\\}$", "只能是 $\\{\\alpha_1, \\alpha_3\\}$"}, 0, "", false});
    list.append(Question{322, "ch03_2.md", "【填空题】设向量组可由包含 2 个向量的组线性表出，则该向量组的极大无关组大小上限为：", QuestionType::Fill, 5, "2", {}, 0, "", false});
    list.append(Question{323, "ch03_2.md", "【主观题】论述向量组极大线性无关组与向量组的秩之间的关系。", QuestionType::Subjective, 10, "一个向量组的极大线性无关组所包含的向量数目，被定义为该向量组的秩。极大无关组与原向量组等价。", {}, 0, "", false});

    // -- 3.3 向量组的秩 --
    list.append(Question{331, "ch03_3.md", "【单选题】两个向量组 $\\text{I}$ 和 $\\text{II}$ 等价的充要条件是：", QuestionType::Single, 5, "2", {"向量个数相同", "两者的秩相等", "它们可以互相线性表出"}, 0, "", false});
    list.append(Question{332, "ch03_3.md", "【填空题】设 $\\alpha_1, \\alpha_2, \\alpha_3$ 线性无关，而 $\\alpha_1, \\alpha_2, \\alpha_3, \\beta$ 线性相关，则扩充组的秩为：", QuestionType::Fill, 5, "3", {}, 0, "", false});
    list.append(Question{333, "ch03_3.md", "【主观题】证明：若向量组 $\\text{I}$ 可由向量组 $\\text{II}$ 线性表出，则 $\\text{rank}(\\text{I}) \\le \\text{rank}(\\text{II})$。", QuestionType::Subjective, 10, "设其极大无关组分别为 $A, B$。因 $\\text{I}$ 可由 $\\text{II}$ 表出，则 $A$ 也可由 $B$ 表出。由无关组个数受表出组个数限制定理，$\\deg(A) \\le \\deg(B)$，即秩的大小关系成立。", {}, 0, "", false});

    // -- 3.4 矩阵的秩 --
    list.append(Question{341, "ch03_4.md", "【单选题】设 $A$ 为 $m \\times n$ 矩阵，下列关于矩阵的秩的性质中，错误的是：", QuestionType::Single, 5, "2", {"$\\text{rank}(A) \\le \\min(m, n)$", "$\\text{rank}(A) = \\text{rank}(A^T)$", "$\\text{rank}(AB) = \\text{rank}(A)\\text{rank}(B)$"}, 0, "", false});
    list.append(Question{342, "ch03_4.md", "【填空题】设矩阵 $A = \\begin{pmatrix} 1&-2&3\\\\2&-4&6\\\\3&-6&9 \\end{pmatrix}$，其矩阵的秩 $\\text{rank}(A)$ 为：", QuestionType::Fill, 5, "1", {}, 0, "", false});
    list.append(Question{343, "ch03_4.md", "【主观题】论述通过初等行变换求矩阵的秩以及列向量组极大无关组的通用方法。", QuestionType::Subjective, 10, "将矩阵通过初等行变换化为阶梯形，阶梯形中非零行的行数即为矩阵的秩；各非零行主元所在的原始列即构成列向量组的一个极大无关组。", {}, 0, "", false});

    // -- 3.5 线性方程组的解 --
    list.append(Question{351, "ch03_5.md", "【单选题】包含 $n$ 个未知数的齐次线性方程组 $Ax = 0$ 有非零解的充要条件是：", QuestionType::Single, 5, "1", {"$\\text{rank}(A) = n$", "$\\text{rank}(A) < n$", "$\\text{rank}(A) > n$"}, 0, "", false});
    list.append(Question{352, "ch03_5.md", "【填空题】已知 3 未知数线性方程组的系数矩阵秩与增广矩阵秩均为 2，则其基础解系中线性无关的解向量个数为：", QuestionType::Fill, 5, "1", {}, 0, "", false});
    list.append(Question{353, "ch03_5.md", "【主观题】用系数矩阵的秩 $\\text{rank}(A)$ 与增广矩阵的秩 $\\text{rank}(\\tilde{A})$ 的关系，判定非齐次方程组解的结构。", QuestionType::Subjective, 10, "若 $\\text{rank}(A) \\neq \\text{rank}(\\tilde{A})$ 则无解；若两者相等且等于未知数个数 $n$ 则有唯一解；若两者相等且小于 $n$ 则有无穷多解。", {}, 0, "", false});


    // ==========================================
    // 第 4 章 矩阵的运算 (ch04_1 ~ ch04_3)
    // ==========================================

    // -- 4.1 矩阵的加法、数乘与乘法 --
    list.append(Question{411, "ch04_1.md", "【单选题】设 $A, B$ 均为 $n$ 阶方阵，下列公式必然成立的是：", QuestionType::Single, 5, "3", {"$(A+B)^2=A^2+2AB+B^2$", "$AB=BA$", "$(AB)^2=A^2B^2$", "$(A+B)(A-B)=A^2-AB+BA-B^2$"}, 0, "", false});
    list.append(Question{412, "ch04_1.md", "【填空题】设方阵 $A$ 满足 $A^2 = A$，若 $B = 2A - I$，则 $B^2$ 等于单位矩阵的 $k$ 倍，其中 $k=$：", QuestionType::Fill, 5, "1", {}, 0, "", false});
    list.append(Question{413, "ch04_1.md", "【主观题】举反例证明：方阵乘法不满足消去律（即由 $AB=AC$ 且 $A \\neq 0$ 不能推出 $B=C$）。", QuestionType::Subjective, 10, "令 $A=\\begin{pmatrix}1&0\\\\0&0\\end{pmatrix}, B=\\begin{pmatrix}0&0\\\\0&1\\end{pmatrix}, C=\\begin{pmatrix}0&0\\\\1&0\\end{pmatrix}$。此时 $AB=0, AC=0$，但 $B \\neq C$。", {}, 0, "", false});

    // -- 4.2 可逆矩阵 --
    list.append(Question{421, "ch04_2.md", "【单选题】方阵 $A$ 可逆的充分必要条件是：", QuestionType::Single, 5, "1", {"$A$ 为非零矩阵", "$A$ 的行列式 $|A| \\neq 0$", "$A$ 的迹不为 0"}, 0, "", false});
    list.append(Question{422, "ch04_2.md", "【填空题】设 3 阶方阵 $A$ 的行列式 $|A|=2$，则其伴随矩阵的行列式 $|A^*|$ 值为：", QuestionType::Fill, 5, "4", {}, 0, "", false});
    list.append(Question{423, "ch04_2.md", "【主观题】利用初等行变换（伴随矩阵法或方阵扩充行变换）求矩阵 $A=\\begin{pmatrix}1&2\\\\0&1\\end{pmatrix}$ 的逆矩阵。", QuestionType::Subjective, 10, "构造 $[A \\mid I] = \\begin{pmatrix}1&2&\\mid&1&0\\\\0&1&\\mid&0&1\\end{pmatrix}$，第一行减去第二行的2倍得 $\\begin{pmatrix}1&0&\\mid&1&-2\\\\0&1&\\mid&0&1\\end{pmatrix}$，故 $A^{-1}=\\begin{pmatrix}1&-2\\\\0&1\\end{pmatrix}$。", {}, 0, "", false});

    // -- 4.3 分块矩阵 --
    list.append(Question{431, "ch04_3.md", "【单选题】若分块对角方阵 $M = \\begin{pmatrix} A&0\\\\0&B \\end{pmatrix}$ 可逆，则其逆矩阵为：", QuestionType::Single, 5, "0", {"$\\begin{pmatrix} A^{-1}&0\\\\0&B^{-1} \\end{pmatrix}$", "$\\begin{pmatrix} 0&B^{-1}\\\\A^{-1}&0 \\end{pmatrix}$", "$\\begin{pmatrix} B^{-1}&0\\\\0&A^{-1} \\end{pmatrix}$"}, 0, "", false});
    list.append(Question{432, "ch04_3.md", "【填空题】已知分块矩阵 $M=\\begin{pmatrix} I&A\\\\0&I \\end{pmatrix}$，其逆矩阵形如 $\\begin{pmatrix} I&kA\\\\0&I \\end{pmatrix}$，则常数 $k=$：", QuestionType::Fill, 5, "-1", {}, 0, "", false});
    list.append(Question{433, "ch04_3.md", "【主观题】推导分块乘法公式：若 $A$ 是 $m \\times n$ 矩阵，$B$ 是 $n \\times p$ 矩阵，详述其分块列乘法的形式。", QuestionType::Subjective, 10, "将 $B$ 按列分块为 $[\\beta_1, \\beta_2, \\dots, \\beta_p]$，则分块乘法结果矩阵可直接表述为 $AB = [A\\beta_1, A\\beta_2, \\dots, A\\beta_p]$。", {}, 0, "", false});


    // ==========================================
    // 第 5 章 矩阵的相抵与相似 (ch05_1 ~ ch05_4)
    // ==========================================

    // -- 5.1 矩阵的相抵 --
    list.append(Question{511, "ch05_1.md", "【单选题】同型矩阵 $A$ 与 $B$ 相抵的充要条件是：", QuestionType::Single, 5, "2", {"行列式相等", "特征值相同", "矩阵的秩相等"}, 0, "", false});
    list.append(Question{512, "ch05_1.md", "【填空题】若 $A$ 与相抵标准形 $\\begin{pmatrix} 1&0&0\\\\0&1&0\\\\0&0&0 \\end{pmatrix}$ 相抵，则 $\\text{rank}(A)=$：", QuestionType::Fill, 5, "2", {}, 0, "", false});
    list.append(Question{513, "ch05_1.md", "【主观题】简述矩阵相抵（Equivalent）的算子定义及初等变换解释。", QuestionType::Subjective, 10, "$A$ 与 $B$ 相抵是指存在可逆方阵 $P, Q$ 使得 $PAQ = B$。在几何上对应于通过一系列初等行列变换互相转化。", {}, 0, "", false});

    // -- 5.2 矩阵的相似 --
    list.append(Question{521, "ch05_2.md", "【单选题】若方阵 $A$ 与 $B$ 相似，则下列哪项不一定相等：", QuestionType::Single, 5, "2", {"矩阵的迹 $\\text{tr}(A) = \\text{tr}(B)$", "特征多项式", "对应的特征向量"}, 0, "", false});
    list.append(Question{522, "ch05_2.md", "【填空题】已知 $A$ 与 $B$ 相似，且 $A$ 的特征值为 $1, 2, 3$，则 $B$ 的行列式 $|B|$ 值为：", QuestionType::Fill, 5, "6", {}, 0, "", false});
    list.append(Question{523, "ch05_2.md", "【主观题】证明：若 $A$ 相似于 $B$，则 $A^k$ 相似于 $B^k$。", QuestionType::Subjective, 10, "因 $A$ 相似于 $B$，存在可逆阵 $P$ 满足 $P^{-1}AP = B$。则 $B^k = (P^{-1}AP)^k = P^{-1}A(PP^{-1})A\\dots AP = P^{-1}A^kP$，故相似关系成立。", {}, 0, "", false});

    // -- 5.3 特征向量与矩阵可对角化 --
    list.append(Question{531, "ch05_3.md", "【单选题】$n$ 阶方阵 $A$ 可相似对角化的充要条件是：", QuestionType::Single, 5, "1", {"拥有 $n$ 个不同的特征值", "拥有 $n$ 个线性无关的特征向量", "矩阵的秩等于 $n$"}, 0, "", false});
    list.append(Question{532, "ch05_3.md", "【填空题】设矩阵 $A = \\begin{pmatrix} 2&0\\\\1&3 \\end{pmatrix}$，其全部特征值中最大的一项是：", QuestionType::Fill, 5, "3", {}, 0, "", false});
    list.append(Question{533, "ch05_3.md", "【主观题】已知特征值存在重根时，如何通过几何重数与代数重数判别矩阵能否相似对角化？", QuestionType::Subjective, 10, "$A$ 可对角化的充要条件是：对其每一个特征值，其几何重数（即属于该特征值的线性无关特征向量个数，$\\dim(\\text{Nul}(\\lambda I - A))$）等于其代数重数（重根数）。", {}, 0, "", false});

    // -- 5.4 实对称矩阵的正交对角化 --
    list.append(Question{541, "ch05_4.md", "【单选题】关于实对称矩阵的性质，下列叙述错误的是：", QuestionType::Single, 5, "2", {"特征值全为实数", "属于不同特征值的特征向量必然正交", "不一定能正交对角化"}, 0, "", false});
    list.append(Question{542, "ch05_4.md", "【填空题】已知实对称矩阵 $A$ 相似于对角方阵 $\\text{diag}(1, 1, 5)$，则 $\\text{rank}(A-I)$ 等于：", QuestionType::Fill, 5, "1", {}, 0, "", false});
    list.append(Question{543, "ch05_4.md", "【主观题】论述将实对称矩阵 $A$ 化为正交对角化的施密特（Schmidt）正交化步骤要点。", QuestionType::Subjective, 10, "首先求出全部特征值并解出基础解系；对属于相同特征值的多个特征向量运用施密特正交化、单位化；不同特征值的特征向量本身正交，拼装即得正交矩阵 $P$。", {}, 0, "", false});


    // ==========================================
    // 第 6 章 二次型 (ch06_1 ~ ch06_2)
    // ==========================================

    // -- 6.1 二次型的定义、规范形 --
    list.append(Question{611, "ch06_1.md", "【单选题】二次型 $f(x_1,x_2) = x_1^2 - 4x_1x_2 + 3x_2^2$ 对应的实对称矩阵为：", QuestionType::Single, 5, "1", {"$\\begin{pmatrix}1&-4\\\\-4&3\\end{pmatrix}$", "$\\begin{pmatrix}1&-2\\\\-2&3\\end{pmatrix}$", "$\\begin{pmatrix}1&0\\\\0&3\\end{pmatrix}$"}, 0, "", false});
    list.append(Question{612, "ch06_1.md", "【填空题】二次型经满秩线性变换化为标准形后，其正惯性指数为 2，负惯性指数为 1，则其矩阵的秩为：", QuestionType::Fill, 5, "3", {}, 0, "", false});
    list.append(Question{613, "ch06_1.md", "【主观题】详述用配方法（Lagrange配方法）将二次型 $f(x,y) = x^2 + 4xy + 5y^2$ 化为标准形的步骤。", QuestionType::Subjective, 10, "原式配方：$f(x,y) = (x+2y)^2 - 4y^2 + 5y^2 = (x+2y)^2 + y^2$。令 $y_1 = x+2y, y_2 = y$，即化为标准形 $y_1^2 + y_2^2$。", {}, 0, "", false});

    // -- 6.2 正定二次型与正定矩阵 --
    list.append(Question{621, "ch06_2.md", "【单选题】实对称矩阵正定的充要条件是：", QuestionType::Single, 5, "0", {"正惯性指数等于未知数个数 $n$", "迹大于 0", "行列式大于 0"}, 0, "", false});
    list.append(Question{622, "ch06_2.md", "【填空题】若对称矩阵 $A = \\begin{pmatrix} 1&t\\\\t&4 \\end{pmatrix}$ 正定，则 $t$ 必须满足开区间 $(-a, a)$，边界 $a=$：", QuestionType::Fill, 5, "2", {}, 0, "", false});
    list.append(Question{623, "ch06_2.md", "【主观题】写出利用霍尔维茨定理（Hurwitz顺序主子式判别法）判断矩阵正定性的标准描述。", QuestionType::Subjective, 10, "实对称矩阵 $A$ 正定的充分必要条件是它的所有顺序主子式全大于 0，即 $\\Delta_1 > 0, \\Delta_2 > 0, \\dots, \\Delta_n > 0$。", {}, 0, "", false});


    // ==========================================
    // 第 7 章 线性空间 (ch07_1 ~ ch07_4)
    // ==========================================

    // -- 7.1 基与维数 --
    list.append(Question{711, "ch07_1.md", "【单选题】全空间 $\\mathbb{R}^3$ 中，由向量组成的基包含的向量个数固定为：", QuestionType::Single, 5, "1", {"2 个", "3 个", "4 个", "任意个"}, 0, "", false});
    list.append(Question{712, "ch07_1.md", "【填空题】在全体 $2 \\times 2$ 实对称矩阵构成的实线性空间中，其空间的维数大小为：", QuestionType::Fill, 5, "3", {}, 0, "", false});
    list.append(Question{713, "ch07_1.md", "【主观题】给出线性空间中“基”与“维数”的标准数学描述定义。", QuestionType::Subjective, 10, "若线性空间 $V$ 中存在 $n$ 个线性无关向量，且 $V$ 中任一向量均可由它们线性表出，则称其为 $V$ 的一组基，$n$ 称为 $V$ 的维数。", {}, 0, "", false});

    // -- 7.2 子空间的交、和与直和 --
    list.append(Question{721, "ch07_2.md", "【单选题】设 $V_1, V_2$ 为 $V$ 的子空间，下列哪项维数公式描述永远成立：", QuestionType::Single, 5, "0", {"$\\dim(V_1+V_2) = \\dim(V_1) + \\dim(V_2) - \\dim(V_1 \\cap V_2)$", "$\\dim(V_1+V_2) = \\dim(V_1) + \\dim(V_2)$", "和等于乘积关系"}, 0, "", false});
    list.append(Question{722, "ch07_2.md", "【填空题】设 $\\dim(V_1)=3, \\dim(V_2)=2$，若和空间为直和 $V_1 \\oplus V_2$，则 $\\dim(V_1 \\oplus V_2)$ 值为：", QuestionType::Fill, 5, "5", {}, 0, "", false});
    list.append(Question{723, "ch07_2.md", "【主观题】证明子空间的和 $V_1 + V_2$ 为直和的充要条件是 $V_1 \\cap V_2 = \\{0\\}$。", QuestionType::Subjective, 10, "若交集非零，非零元可表为 $v+(-v)=0$ 破坏零向量表出唯一性；反之，若零向量表出唯一，设 $v_1+v_2=0 \\implies v_1=-v_2 \\in V_1 \\cap V_2 = \\{0\\}$，故唯一性成立。", {}, 0, "", false});

    // -- 7.3 线性空间的同构 --
    list.append(Question{731, "ch07_3.md", "【单选题】两个数域相同的有限维线性空间同构的充要条件是：", QuestionType::Single, 5, "1", {"包含零向量的个数相同", "两者的维数相等", "基向量完全相同"}, 0, "", false});
    list.append(Question{732, "ch07_3.md", "【填空题】所有次数小于 4 的实系数一元多项式空间 $P_4[x]$ 同构于实向量空间 $\\mathbb{R}^k$，则 $k=$：", QuestionType::Fill, 5, "4", {}, 0, "", false});
    list.append(Question{733, "ch07_3.md", "【主观题】简述线性空间同构（Isomorphism）映射必须满足的两个保持算子。", QuestionType::Subjective, 10, "同构双射 $\\phi$ 必须保持加法和数乘结构，即：$\\phi(\\alpha+\\beta) = \\phi(\\alpha)+\\phi(\\beta)$ 且 $\\phi(k\\alpha) = k\\phi(\\alpha)$。", {}, 0, "", false});

    // -- 7.4 商空间 --
    list.append(Question{741, "ch07_4.md", "【单选题】设 $W$ 是 $n$ 维空间 $V$ 的 $m$ 维子空间，则商空间 $V/W$ 的维数为：", QuestionType::Single, 5, "1", {"$n+m$", "$n-m$", "$m$", "$n$"}, 0, "", false});
    list.append(Question{742, "ch07_4.md", "【填空题】设 $\\dim(V)=4$，子空间 $W$ 的维数为 1，则其商空间 $V/W$ 的基包含向量个数为：", QuestionType::Fill, 5, "3", {}, 0, "", false});
    list.append(Question{743, "ch07_4.md", "【主观题】给出商空间（Quotient Space）陪集加法与数乘的基本数学构造公式。", QuestionType::Subjective, 10, "商空间元素为陪集 $v+W$。加法定义：$(v_1+W) + (v_2+W) = (v_1+v_2)+W$；数乘定义：$k(v+W) = (kv)+W$。", {}, 0, "", false});


    // ==========================================
    // 第 8 章 线性映射 (ch08_1 ~ ch08_5)
    // ==========================================

    // -- 8.1 线性映射的定义 --
    list.append(Question{811, "ch08_1.md", "【单选题】映射 $\\mathcal{A}: \\mathbb{R}^2 \\to \\mathbb{R}^2$ 中，属于线性变换的是：", QuestionType::Single, 5, "0", {"$\\mathcal{A}(x,y) = (2x+y, x-y)$", "$\\mathcal{A}(x,y) = (x^2, y)$", "$\\mathcal{A}(x,y) = (x+1, y)$"}, 0, "", false});
    list.append(Question{812, "ch08_1.md", "【填空题】已知线性映射满足 $\\mathcal{A}(\\alpha_1)=(1,0)^T, \\mathcal{A}(\\alpha_2)=(0,2)^T$，则 $\\mathcal{A}(3\\alpha_1 + 2\\alpha_2)$ 的第二分量值为：", QuestionType::Fill, 5, "4", {}, 0, "", false});
    list.append(Question{813, "ch08_1.md", "【主观题】举反例证明：映射 $\\mathcal{A}(x) = |x|$ 不构成线性变换。", QuestionType::Subjective, 10, "取 $k=-1, x=1$。则 $\\mathcal{A}(-1 \\cdot 1) = |-1| = 1$。而 $-1 \\cdot \\mathcal{A}(1) = -1 \\cdot 1 = -1$。两者不相等，破坏数乘齐次性。", {}, 0, "", false});

    // -- 8.2 核与像 --
    list.append(Question{821, "ch08_2.md", "【单选题】设线性变换 $\\mathcal{A}$ 的核空间维数为 2，整个空间维数维 5，则其像空间 $\\text{Im}(\\mathcal{A})$ 的维数为：", QuestionType::Single, 5, "1", {"2", "3", "5", "7"}, 0, "", false});
    list.append(Question{822, "ch08_2.md", "【填空题】设矩阵 $\\begin{pmatrix}1&2\\\\2&4\\end{pmatrix}$ 代表线性变换，其像空间 $\\text{Im}(\\mathcal{A})$ 的维数大小为：", QuestionType::Fill, 5, "1", {}, 0, "", false});
    list.append(Question{823, "ch08_2.md", "【主观题】给出线性映射中核空间（Kernel）与像空间（Image）的集合构造定义公式。", QuestionType::Subjective, 10, "$\\text{Ker}(\\mathcal{A}) = \\{v \\in V \\mid \\mathcal{A}(v) = 0\\}$；$\\text{Im}(\\mathcal{A}) = \\{\\mathcal{A}(v) \\mid v \\in V\\}$。", {}, 0, "", false});

    // -- 8.3 线性映射的矩阵表示 --
    list.append(Question{831, "ch08_3.md", "【单选题】线性变换在两组不同的基下的矩阵 $A$ 与 $B$ 的内在关系是：", QuestionType::Single, 5, "1", {"它们一定相抵", "它们一定相似", "它们完全相等"}, 0, "", false});
    list.append(Question{832, "ch08_3.md", "【填空题】若线性变换在基底下的方阵为 $A$，基过渡矩阵为 $P$，则新基下对应的矩阵公式为 $P^{-1}AP$。若 $|A|=4$，新方阵的行列式值为：", QuestionType::Fill, 5, "4", {}, 0, "", false});
    list.append(Question{833, "ch08_3.md", "【主观题】论述如何利用过渡矩阵实现变换方阵的跨基底坐标转换坐标推导。", QuestionType::Subjective, 10, "设新旧基过渡公式为 $Y = PX$，变换方程为 $y = Ax, y' = Bx'$。联立代入可得 $B = P^{-1}AP$。这就是变换矩阵的相似变换根源。", {}, 0, "", false});

    // -- 8.4 不变子空间与 Cayley–Hamilton定理 --
    list.append(Question{841, "ch08_4.md", "【单选题】凯莱-哈密顿定理（Cayley–Hamilton Theorem）指出，方阵 $A$ 是其哪个多项式的根：", QuestionType::Single, 5, "1", {"极小多项式", "特征多项式", "任意零多项式"}, 0, "", false});
    list.append(Question{842, "ch08_4.md", "【填空题】方阵 $A$ 的特征多项式为 $f(\\lambda)=\\lambda^2 - 3\\lambda + 2$，根据定理，计算矩阵 $A^2 - 3A + 2I$ 对应零方阵的元素和为：", QuestionType::Fill, 5, "0", {}, 0, "", false});
    list.append(Question{843, "ch08_4.md", "【主观题】给出不变子空间（Invariant Subspace）的定义，并阐述其对变换矩阵分块简化的价值。", QuestionType::Subjective, 10, "若子空间 $W$ 满足对任意 $w \\in W$ 均有 $\\mathcal{A}(w) \\in W$，则称 $W$ 为不变子空间。选取其基底扩充可使变换方阵化为分块上三角矩阵实现简化。", {}, 0, "", false});

    // -- 8.5 Jordan标准形 --
    list.append(Question{851, "ch08_5.md", "【单选题】一个 $3 \\times 3$ 阶的单特征值 Jordan 块，其对角线上方次对角线上的元素 1 的个数为：", QuestionType::Single, 5, "1", {"1 个", "2 个", "3 个", "0 个"}, 0, "", false});
    list.append(Question{852, "ch08_5.md", "【填空题】方阵 $A$ 的初等因子为 $\\lambda^2, \\lambda, \\lambda-1$，则其 Jordan 标准形中属于特征值 0 的 Jordan 块个数为：", QuestionType::Fill, 5, "2", {}, 0, "", false});
    list.append(Question{853, "ch08_5.md", "【主观题】简述初等因子理论与矩阵 Jordan 标准形唯一性的判定依赖逻辑。", QuestionType::Subjective, 10, "任一复方阵的初等因子组被初等变换唯一确定。初等因子组中的每一个因子 $(\\lambda-\\lambda_0)^k$ 唯一对应一个 $k$ 阶 Jordan 块，从而决定了 Jordan 标准形的唯一性。", {}, 0, "", false});


    // ==========================================
    // 第 9 章 lambda-矩阵 (ch09_1 ~ ch09_3)
    // ==========================================

    // -- 9.1 lambda-矩阵的定义 --
    list.append(Question{911, "ch09_1.md", "【单选题】下列算子操作中，不属于 $\\lambda$-矩阵可逆初等变换的是：", QuestionType::Single, 5, "1", {"交换两行", "某行乘以包含 $\\lambda$ 的非零多项式", "某行加上另一行乘以多项式 $g(\\lambda)$"}, 0, "", false});
    list.append(Question{912, "ch09_1.md", "【填空题】$\\lambda$-矩阵 $\\begin{pmatrix}\\lambda&1\\\\0&\\lambda\\end{pmatrix}$ 的行列式多项式最高次数为：", QuestionType::Fill, 5, "2", {}, 0, "", false});
    list.append(Question{913, "ch09_1.md", "【主观题】给出 $\\lambda$-矩阵可逆（行列式为非零常数）的等价判定充要条件描述。", QuestionType::Subjective, 10, "$\\lambda$-矩阵可逆的充要条件是其行列式是一个与 $\\lambda$ 无关的非零常数，或者说它能通过初等变换化为单位矩阵。", {}, 0, "", false});

    // -- 9.2 Smith 标准形 --
    list.append(Question{921, "ch09_2.md", "【单选题】关于 $\\lambda$-矩阵的 Smith 标准形，对角线上的不变因子满足的核心整除链特性是：", QuestionType::Single, 5, "0", {"$d_1(\\lambda) \\mid d_2(\\lambda) \\mid \\dots \\mid d_r(\\lambda)$", "$d_r(\\lambda) \\mid \\dots \\mid d_1(\\lambda)$", "无任何整除依赖"}, 0, "", false});
    list.append(Question{922, "ch09_2.md", "【填空题】已知 2 阶行列式因子 $D_2(\\lambda)=\\lambda-1$，1 阶行列式因子 $D_1(\\lambda)=1$，则第二个不变因子 $d_2(\\lambda)$ 在 $\\lambda=6$ 时的值为：", QuestionType::Fill, 5, "5", {}, 0, "", false});
    list.append(Question{923, "ch09_2.md", "【主观题】如何利用行列式因子（Greatest Common Divisors of Minors）推导计算不变因子？", QuestionType::Subjective, 10, "不变因子可由相邻两级行列式因子相除得到，公式为 $d_i(\\lambda) = D_i(\\lambda) / D_{i-1}(\\lambda)$（约定 $D_0(\\lambda)=1$）。", {}, 0, "", false});

    // -- 9.3 不变因子与 Jordan 标准形 --
    list.append(Question{931, "ch09_3.md", "【单选题】两复方阵 $A$ 与 $B$ 相似的充要条件是它们的特征矩阵 $\\lambda I - A$ 与 $\\lambda I - B$ 满足：", QuestionType::Single, 5, "2", {"行列式相等", "迹相等", "具有相同的不变因子（或 Smith 标准形）"}, 0, "", false});
    list.append(Question{932, "ch09_3.md", "【填空题】若 $\\lambda I - A$ 的最后一个不变因子为 $\\lambda^2(\\lambda-1)$，则方阵 $A$ 的极小多项式次数为：", QuestionType::Fill, 5, "3", {}, 0, "", false});
    list.append(Question{933, "ch09_3.md", "【主观题】论述如何从不变因子组（Invariant Factors）通过因式分解提取出初等因子组。", QuestionType::Subjective, 10, "将每一个次数大子 0 的不变因子 $d_i(\\lambda)$ 在复数域上分解为互素的一阶线性因子的幂次项，这些幂次项组成的集合即为该矩阵的全部初等因子。", {}, 0, "", false});


    // ==========================================
    // 第 10 章 具有度量的线性空间 (ch10_1 ~ ch10_4)
    // ==========================================

    // -- 10.1 内积与欧几里得空间 --
    list.append(Question{1011, "ch10_1.md", "【单选题】在欧几里得空间中，施瓦茨（Schwarz）不等式正确的形式为：", QuestionType::Single, 5, "0", {"$(\\alpha, \\beta)^2 \\le (\\alpha, \\alpha)(\\beta, \\beta)$", "$(\\alpha, \\beta) \\ge \\|\\alpha\\| \\|\\beta\\|$", "加法三角不等式"}, 0, "", false});
    list.append(Question{1012, "ch10_1.md", "【填空题】在标准内积空间中，$\\alpha=(1,1)^T, \\beta=(1,-1)^T$，则它们的内积 $(\\alpha, \\beta)$ 计算结果为：", QuestionType::Fill, 5, "0", {}, 0, "", false});
    list.append(Question{1013, "ch10_1.md", "【主观题】写出实线性空间上内积（Inner Product）定义的四条核心公理。", QuestionType::Subjective, 10, "内积须满足：1.对称性 $(\\alpha,\\beta)=(\\beta,\\alpha)$；2.齐次性 $(k\\alpha,\\beta)=k(\\alpha,\\beta)$；3.分配律 $(\\alpha+\\beta,\\gamma)=(\\alpha,\\gamma)+(\\beta,\\gamma)$；4.正定性 $(\\alpha,\\alpha) \\ge 0$ 且等于0当且仅当为零向量。", {}, 0, "", false});

    // -- 10.2 正交变换与对称变换 --
    list.append(Question{1021, "ch10_2.md", "【单选题】有限维实内积空间中的线性变换为正交变换的充要条件是它的矩阵为：", QuestionType::Single, 5, "1", {"对称矩阵", "正交矩阵", "伴随矩阵", "逆矩阵"}, 0, "", false});
    list.append(Question{1022, "ch10_2.md", "【填空题】若正交矩阵 $Q$ 满足 $Q^TQ=I$ 且满足 $\\det(Q) > 0$，则该行列式值确定为：", QuestionType::Fill, 5, "1", {}, 0, "", false});
    list.append(Question{1023, "ch10_2.md", "【主观题】给出对称变换（Symmetric Operator）的定义，并简述其与实对称矩阵的关系。", QuestionType::Subjective, 10, "对称变换指满足 $(\\mathcal{A}\\alpha, \\beta) = (\\alpha, \\mathcal{A}\\beta)$ 的变换。它在标准正交基下的表示矩阵必然是实对称矩阵。", {}, 0, "", false});

    // -- 10.3 酉空间与 Hermite 矩阵 --
    list.append(Question{1031, "ch10_3.md", "【单选题】酉空间（Unitary Space）内积定义与欧氏空间最大的区别在于其满足：", QuestionType::Single, 5, "1", {"对称性", "共轭对称性 $(\\alpha, \\beta) = \\overline{(\\beta, \\alpha)}$", "反对称性"}, 0, "", false});
    list.append(Question{1032, "ch10_3.md", "【填空题】Hermite 矩阵满足 $A^H = A$。已知对角线上某元素为复数 $a + bi$，则虚部 $b$ 的值必须等于：", QuestionType::Fill, 5, "0", {}, 0, "", false});
    list.append(Question{1033, "ch10_3.md", "【主观题】简述酉矩阵（Unitary Matrix）的定义以及特征值的模长特性。", QuestionType::Subjective, 10, "满足 $U^HU = UU^H = I$ 的复方阵称为酉矩阵。由于酉变换保持向量模长不变，其特征值的模长必然全部等于 1。", {}, 0, "", false});

    // -- 10.4 最小二乘法 --
    list.append(Question{1041, "ch10_4.md", "【单选题】在线性最小二乘超定方程组 $Ax = b$ 中，法方程（Normal Equation）的标准形式为：", QuestionType::Single, 5, "0", {"$A^TAx = A^Tb$", "$AA^Tx = b$", "$Ax = A^Tb$"}, 0, "", false});
    list.append(Question{1042, "ch10_4.md", "【填空题】设最小二乘拟合得到的残差向量为 $e = (1, -1, 1, -1)^T$，则误差平方和 $\\|e\\|^2$ 的大小为：", QuestionType::Fill, 5, "4", {}, 0, "", false});
    list.append(Question{1043, "ch10_4.md", "【主观题】简述最小二乘法（Least Squares Method）在欧氏空间投影理论下的几何解释。", QuestionType::Subjective, 10, "当 $Ax=b$ 无解时，最小二乘解实际上是将向量 $b$ 正交投影到 $A$ 的列空间上，使得投影残差向量 $e = b - Ax$ 的欧氏范数（模长）达到全局最小。", {}, 0, "", false});

    return list;
}

QVector<Question> QuestionBank::getQuestionsByChapter(const QString& chapterPath) {
    QVector<Question> all = getAllQuestions();
    QVector<Question> filtered;

    QString cleanPath = chapterPath;
    if (cleanPath.contains('/')) {
        cleanPath = cleanPath.split('/').last();
    }

    for (const auto& q : all) {
        if (q.contentPath == cleanPath) {
            filtered.append(q);
        }
    }
    return filtered;
}