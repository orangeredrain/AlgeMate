#include "PastExams.h"

namespace AlgeMate::Learning {

QVector<QString> PastExams::getExamList() {
    return {
        QStringLiteral("北京大学 2021-2022 高等代数 (II) 期末考_tqc"),
        QStringLiteral("北京大学 2022-2023 高等代数 (II) 期末考_lww"),
        QStringLiteral("北京大学 2022-2023 高等代数 (II) 期末考_wfz"),
        // QStringLiteral("北京大学 2024-2025 高等代数 (II) 期中考_lww")
    };
}

QVector<Question> PastExams::getExamPaper(int examIndex) {
    QVector<Question> list;

    // ==========================================
    // 试卷 0: 北京大学 2021-2022 高等代数 (II) 期末考_tqc
    // ==========================================
    if (examIndex == 0) {
        QString source = "北京大学 2021-2022 高等代数 (II) 期末考_tqc";

        list.append(Question{202201, source,
                             R"(【解答题（20分）】设 $P = \begin{pmatrix} 1-a & b \\ a & 1-b \end{pmatrix}$，其中 $0 < a \le 1, 0 < b \le 1, a+b < 2$。令 $x^{(0)} \in \mathbb{R}^2$，定义 $x^{(k)} = P^k x^{(0)}$。
(1) 求出 $x^{(k)}$ 的通项公式；
(2) 对给定的初始向量，是否存在 2 维向量 $x^*$ 使得 $\lim_{k\to\infty} x^{(k)} = x^*$？如果存在如何求出 $x^*$？)",
                             QuestionType::Subjective, 20,
                             R"($x^{(k)}$ 的极限总是存在，极限 $x^*$ 与初始状态有关，$x^* = \frac{x_1+x_2}{a+b} \begin{pmatrix} b \\ a \end{pmatrix}$。
\n【详细解析】
1. 矩阵 $P$ 的特征值为 $\lambda_1 = 1, \lambda_2 = 1-a-b$。因为 $a,b>0$ 且 $a+b<2$，所以 $|\lambda_2| < 1$。
2. 对应特征向量：$\lambda_1=1$ 对应 $v_1 = (b, a)^T$；$\lambda_2=1-a-b$ 对应 $v_2 = (1, -1)^T$。
3. 分解 $x^{(0)} = c_1 v_1 + c_2 v_2$，可解得 $c_1 = \frac{x_1+x_2}{a+b}$。
4. $x^{(k)} = c_1 \cdot 1^k v_1 + c_2 (1-a-b)^k v_2$。当 $k \to \infty$ 时，第二项趋于 0，极限即为 $c_1 v_1$。)",
                             {}, 0, "", false});

        list.append(Question{202202, source,
                             R"(【解答题（20分）】设 $A \in M_{m \times n}(\mathbb{R})$，$\beta \in \mathbb{R}^m$。$W$ 是 $\mathbb{R}^m$ 中由 $A$ 列向量生成的子空间，$p$ 是 $\mathbb{R}^m$ 在 $W$ 上的正交投影。假设 $AX = \beta$ 无解。
(1) 证明 $AX = p(\beta)$ 有解；
(2) 证明 $\gamma$ 是最小二乘解的充要条件是 $\gamma$ 是方程 $A^T AX = A^T \beta$ 的解。)",
                             QuestionType::Subjective, 20,
                             R"(利用正交投影定义与核空间、像空间的正交关系证明。
\n【详细解析】
(1) 由于 $p(\beta)$ 投影到了 $W$ 也就是 $\text{Im}(A)$ 中，根据像空间的定义，必然存在 $X$ 使得 $AX = p(\beta)$。
(2) $AX = p(\beta)$ 等价于 $\beta - AX = \beta - p(\beta)$。根据正交投影的性质，$\beta - p(\beta) \perp W = \text{Im}(A)$。
因此，$\beta - AX$ 属于 $\text{Im}(A)$ 的正交补，即 $\text{Ker}(A^T)$。这意味着 $A^T(\beta - AX) = 0$，展开即得到 $A^T AX = A^T \beta$。)",
                             {}, 0, "", false});

        list.append(Question{202203, source,
                             R"(【解答题（15分）】设 $A \in M_7(\mathbb{Q})$，其最小多项式为 $f(\lambda) = (\lambda-1)^4(\lambda-2)^2$。
(1) 写出矩阵 $A$ 可能的特征多项式；
(2) 写出矩阵 $A$ 可能的 Jordan 标准型；
(3) 对每一种情形，写出它的所有 1 维不变子空间。)",
                             QuestionType::Subjective, 15,
                             R"(特征多项式有两种可能：$(\lambda-1)^5(\lambda-2)^2$ 或 $(\lambda-1)^4(\lambda-2)^3$。分别对应两套 Jordan 块组合。
\n【详细解析】
特征多项式的根与最小多项式相同，且次数为7。
情形1：特征多项式为 $(\lambda-1)^5(\lambda-2)^2$。Jordan 块为 $J_4(1) \oplus J_1(1) \oplus J_2(2)$。
此时对应特征值1的几何重数为2，特征值2的几何重数为1，共3个一维不变子空间（即这三个 Jordan 块对应的 3 个特征向量）。
情形2：特征多项式为 $(\lambda-1)^4(\lambda-2)^3$。Jordan 块为 $J_4(1) \oplus J_2(2) \oplus J_1(2)$。
此时对应特征值1的几何重数为1，特征值2的几何重数为2，也是共3个一维不变子空间。)",
                             {}, 0, "", false});

        list.append(Question{202204, source,
                             R"(【解答题（10分）】设 $\phi: M_n(F) \to M_n(F), A \mapsto A^T$ 为线性映射。求 $\phi$ 的特征多项式和最小多项式 ($n \ge 2$)。)",
                             QuestionType::Subjective, 10,
                             R"(最小多项式为 $\lambda^2 - 1$，特征多项式为 $(\lambda-1)^{\frac{n(n+1)}{2}} (\lambda+1)^{\frac{n(n-1)}{2}}$。
\n【详细解析】
$\phi^2(A) = (A^T)^T = A$，因此 $\phi^2 - I = 0$。由于 $n \ge 2$，$\phi$ 不是纯量矩阵，其最小多项式只能是 $\lambda^2 - 1$。特征值只能为 1（对应对称矩阵，维数 $\frac{n(n+1)}{2}$）和 -1（对应反对称矩阵，维数 $\frac{n(n-1)}{2}$）。)",
                             {}, 0, "", false});

        list.append(Question{202205, source,
                             R"(【解答题（15分）】设 $V$ 是 $n$ 维欧氏空间，$T$ 是正交变换，$W = \text{Ker}(I-T)$，$p$ 是到 $W$ 上的正交投影。证明：对于任意 $\alpha \in V$，都有：
$$\lim_{k \to \infty} \frac{1}{k} \sum_{i=0}^{k-1} T^i(\alpha) = p(\alpha)$$)",
                             QuestionType::Subjective, 15,
                             R"(利用 $V = \text{Ker}(I-T) \oplus \text{Im}(I-T)$ 进行正交分解证明。
\n【详细解析】
对于正交变换有 $V = \text{Ker}(I-T) \oplus \text{Im}(I-T)$ 且两者相互正交。
设 $\alpha = \alpha_1 + \alpha_2$。
若 $\alpha_1 \in \text{Ker}(I-T)$，则 $T(\alpha_1) = \alpha_1$，平均值恒为 $\alpha_1 = p(\alpha)$。
若 $\alpha_2 \in \text{Im}(I-T)$，存在 $\beta$ 使得 $\alpha_2 = \beta - T(\beta)$。累加得 $\sum T^i(\alpha_2) = \beta - T^k(\beta)$。由于 $T$ 保范数，该项有界。除以 $k$ 后极限为 0。得证。)",
                             {}, 0, "", false});

        list.append(Question{202206, source,
                             R"(【解答题（10分）】设 $V$ 是有限维欧氏空间，$\phi \in \text{End}_{\mathbb{R}}(V)$ 是对称线性变换。证明：函数
$$f(\alpha) = \frac{(\alpha, \phi(\alpha))}{(\alpha, \alpha)}$$
是 $V \setminus \{0\}$ 上的有界函数，其中 $\alpha \in V \setminus \{0\}$, $(\alpha, \alpha)$ 是 $V$ 的内积。)",
                             QuestionType::Subjective, 10,
                             R"(利用实对称变换的谱定理及瑞利商的性质证明有界性。
\n【详细解析】
1. 因为 $\phi$ 是欧氏空间上的实对称变换，根据谱定理，$V$ 中存在一组标准正交基 $e_1, \dots, e_n$，它们都是 $\phi$ 的特征向量。设对应的特征值为 $\lambda_1, \dots, \lambda_n$。
2. 对任意非零向量 $\alpha = \sum_{i=1}^n x_i e_i$，有 $(\alpha, \alpha) = \sum x_i^2$。
3. 计算 $\phi(\alpha) = \sum \lambda_i x_i e_i$，因此 $(\alpha, \phi(\alpha)) = \sum \lambda_i x_i^2$。
4. 代入函数得 $f(\alpha) = \frac{\sum \lambda_i x_i^2}{\sum x_i^2}$。
5. 设 $\lambda_{\max}$ 和 $\lambda_{\min}$ 分别为最大和最小的特征值，显然有 $\lambda_{\min} \sum x_i^2 \le \sum \lambda_i x_i^2 \le \lambda_{\max} \sum x_i^2$。
6. 因此 $\lambda_{\min} \le f(\alpha) \le \lambda_{\max}$，即 $f(\alpha)$ 是有界函数。得证。)",
                             {}, 0, "", false});

        list.append(Question{202207, source,
                             R"(【解答题（10分）】设 $\alpha$ 是欧氏空间 $V$ 中的一个非零向量，$\alpha_1, \dots, \alpha_n$ 是 $V$ 中 $n$ 个向量，满足：
$$(\alpha_i, \alpha_j) \le 0, \quad (\alpha, \alpha_i) > 0, \quad i, j = 1, \dots, n, i \neq j$$
(1) 求证 $\alpha_1, \dots, \alpha_n$ 线性无关。
(2) 设 $V$ 是 2022 维欧式空间，$\alpha_1, \dots, \alpha_n \in V$。若这 $n$ 个向量中任意两个向量的夹角是钝角，求 $n$ 的最大值。)",
                             QuestionType::Subjective, 10,
                             R"((1) 通过反证法和正负系数分组，利用内积的非负性得出矛盾。
(2) $n$ 的最大值为 2023。
\n【详细解析】
(1) 证明线性无关：
设存在实数 $c_i$ 使得 $\sum c_i \alpha_i = 0$。将系数分为正数和非正数两组，设 $I = \{i \mid c_i > 0\}$, $J = \{j \mid c_j \le 0\}$。
令 $x = \sum_{i \in I} c_i \alpha_i = \sum_{j \in J} (-c_j) \alpha_j$。由于 $i \in I, j \in J$ 时 $i \neq j$，根据已知条件 $(\alpha_i, \alpha_j) \le 0$。
计算范数：$(x, x) = (\sum_{i \in I} c_i \alpha_i, \sum_{j \in J} (-c_j) \alpha_j) = \sum_{i \in I} \sum_{j \in J} c_i (-c_j) (\alpha_i, \alpha_j)$。
因为 $c_i > 0, -c_j \ge 0, (\alpha_i, \alpha_j) \le 0$，所以上式 $(x, x) \le 0$。又因为内积 $(x, x) \ge 0$，故必有 $(x, x) = 0$，即 $x = 0$。
于是 $0 = (x, \alpha) = (\sum_{i \in I} c_i \alpha_i, \alpha) = \sum_{i \in I} c_i (\alpha_i, \alpha)$。
因为 $c_i > 0$ 且 $(\alpha_i, \alpha) > 0$，要使其和为0，必须集合 $I$ 为空。同理 $J$ 中也只能全为0。故所有 $c_i = 0$，向量组线性无关。

(2) 求最大值：
任意两向量夹角为钝角，意味着 $(\alpha_i, \alpha_j) < 0 \le 0$。
在 $m$ 维空间中，最多可以放置 $m+1$ 个两两夹角为钝角的向量（构成正单纯形）。根据(1)的结论变体，若存在 $m+2$ 个，必然存在线性相关性，导致矛盾。因此在 2022 维空间中，$n$ 的最大值为 $2022 + 1 = 2023$。)",
                             {}, 0, "", false});
    }
    // ==========================================
    // 试卷 1: 北京大学 2022-2023 高等代数 (II) 期末考_lww
    // ==========================================
    else if (examIndex == 1) {
        QString source = "北京大学 2022-2023 高等代数 (II) 期末考_lww";

        list.append(Question{202301, source,
                             R"(【解答题（20分）】在有理数域 $\mathbb{Q}$ 上求以下矩阵的有理标准形：
$$ \begin{pmatrix} 3 & 0 & 8 \\ 3 & -1 & 6 \\ -2 & 0 & -5 \end{pmatrix} $$)",
                             QuestionType::Subjective, 20,
                             R"(对角块矩阵 $\text{diag}\left(-1, \begin{pmatrix} 0 & -1 \\ 1 & -2 \end{pmatrix}\right)$。
\n【详细解析】
计算特征多项式 $\det(\lambda I - A) = (\lambda+1)^3$。
计算 $A+I$ 可知 $(A+I)^2 = 0$ 且 $A+I \neq 0$。因此最小多项式为 $(\lambda+1)^2 = \lambda^2+2\lambda+1$。
不变因子为 $1, \lambda+1, (\lambda+1)^2$。对应有理标准形由一个 1x1 的块 $(-1)$ 和一个 2x2 的伴随阵块 $\begin{pmatrix} 0 & -1 \\ 1 & -2 \end{pmatrix}$ 组成。)",
                             {}, 0, "", false});

        list.append(Question{202302, source,
                             R"(【解答题（15分）】求以下三个矩阵的 Jordan 标准形：
(1) 伴随形式的幂零阵
(2) 含有右上方 2x2 个 1 的 4 阶块对角阵
(3) $E_{31}$ 与 $E_{32}$ 有值的特殊阵)",
                             QuestionType::Subjective, 15,
                             R"(分别为：$J_2(0) \oplus J_1(0) \oplus J_1(0)$；$J_2(0) \oplus J_2(0)$；$J_3(0) \oplus J_1(0)$。
\n【详细解析】
均为幂零矩阵，考察其幂次的秩：
(1) 秩为1，平方为0，推导得一个2阶块和两个1阶块。
(2) 秩为2，平方为0，推导得两个2阶块。
(3) 秩为1，立方为0，推导得一个3阶块和一个1阶块。)",
                             {}, 0, "", false});

        list.append(Question{202303, source,
                             R"(【解答题（20分）】设 $A \in M_n(\mathbb{C})$ 的特征值全为 1，而 $k \ge 1$。证明 $A^k$ 与 $A$ 共轭。)",
                             QuestionType::Subjective, 20,
                             R"(利用幂零矩阵及 Jordan 标准形结构证明。
\n【详细解析】
$A$ 可写为 $I + N$，其中 $N$ 为幂零阵。
$A^k = (I+N)^k = I + kN + \dots$。由于复数域上 $k \neq 0$，多项式 $kN + \dots$ 的秩与 $N$ 的秩完全相同，从而 $N$ 的 Jordan 块大小分布在经过该多项式映射后保持不变。因此 $A^k$ 的 Jordan 标准形与 $A$ 相同，两者相似（共轭）。)",
                             {}, 0, "", false});

        list.append(Question{202304, source,
                             R"(【解答题（15分）】设 $A \in \text{GL}(n, \mathbb{C})$。证明存在唯一的 $S, U \in \text{GL}(n, \mathbb{C})$ 使得 $A = SU = US$，而且 $S$ 可对角化，$U - I$ 幂零。)",
                             QuestionType::Subjective, 15,
                             R"(使用乘法型 Jordan-Chevalley 分解。
\n【详细解析】
由加法型分解知存在唯一的可对角化矩阵 $S_0$ 和幂零矩阵 $N_0$ 使得 $A = S_0 + N_0$ 且 $S_0 N_0 = N_0 S_0$。
由于 $A$ 可逆，$S_0$ 必须可逆（特征值全非0）。
令 $S = S_0, U = I + S_0^{-1} N_0$。显然 $U-I$ 是幂零的，且 $SU = US = A$。唯一性由多项式互素构造得出。)",
                             {}, 0, "", false});

        list.append(Question{202305, source,
                             R"(【解答题（10分）】设 $V$ 和 $W$ 为实数域 $\mathbb{R}$ 上的有限维向量空间，维数都 $> 1$。说明 $\{v \otimes w : v \in V, w \in W\} \neq V \otimes W$。)",
                             QuestionType::Subjective, 10,
                             R"(纯张量的集合不构成线性空间。
\n【详细解析】
选定基底举出反例。两个非零纯张量之和不一定是纯张量。如果维数都大于 1，很容易构造出 rank 为 2 的张量，它不属于纯张量集合。)",
                             {}, 0, "", false});

        list.append(Question{202306, source,
                             R"(【解答题（20分）】群 $G$ 的中心定义为 $Z(G) = \{z \in G: zg = gz\}$。
(1) 证明 $Z(\text{SL}(n, F)) = \{\lambda I : \lambda^n = 1\}$。
(2) 证明辛群的中心 $Z(\text{Sp}(V)) = \{\pm I\}$。)",
                             QuestionType::Subjective, 20,
                             R"(中心元素必须是纯量矩阵，再结合行列式或保辛内积条件求解。
\n【详细解析】
(1) 与所有初等矩阵可交换的矩阵必须是纯量矩阵 $\lambda I$。代入行列式 $\det(\lambda I) = \lambda^n = 1$。
(2) 同理得到纯量矩阵 $\lambda I$，由于它属于辛群，必须保持辛形式：$B(\lambda x, \lambda y) = B(x, y)$，即 $\lambda^2 B(x, y) = B(x, y)$，故 $\lambda^2 = 1$，解得 $\lambda = \pm 1$。)",
                             {}, 0, "", false});
    }
    // ==========================================
    // 试卷 2: 北京大学 2022-2023 高等代数 (II) 期末考_wfz
    // ==========================================
    else if (examIndex == 2) {
        QString source = "北京大学 2022-2023 高等代数 (II) 期末考_wfz";

        list.append(Question{202351, source,
                             R"(【解答题（10分）】
1) 任意线性变换是否都有 $V = \text{Ker}(\mathscr{A}) \oplus \text{Im}(\mathscr{A})$？对于酉空间上的 Hermite 变换呢？(如果正确，请给出证明；如果错误，请举出反例)
2) $\mathbb{Q}$ 上的 $n$ 维向量空间上是否存在线性变换 $\mathscr{A}$ 满足 $\mathscr{A}^3 + \mathscr{A} - I = 0$？如果存在，请写出这样的变换；如果不存在，请说明理由。)",
                             QuestionType::Subjective, 10,
                             R"(一般变换不成立，Hermite 变换成立；第二个存在性取决于 $n$ 是否是 3 的倍数。
\n【详细解析】
1) 考虑幂零阵 $\begin{pmatrix} 0 & 1 \\ 0 & 0 \end{pmatrix}$ 即可得反例，核与像重合。而 Hermite 矩阵酉等价于对角阵，核与像正交且直和必然成立。
2) $\mathscr{A}^3 + \mathscr{A} - I = 0$ 意味着其最小多项式必须整除 $\lambda^3 + \lambda - 1$。该多项式在 $\mathbb{Q}$ 上不可约。因而特征多项式必须是该不可约多项式的次幂，空间维数 $n$ 必须是 3 的倍数时才存在。)",
                             {}, 0, "", false});

        list.append(Question{202352, source,
                             R"(【解答题（15分）】$M = \begin{pmatrix} a & b \\ c & d \end{pmatrix} \in M_2(F)$。定义 $M_2(F)$ 上的映射 $\mathscr{A}$：$\mathscr{A}(X) = XM, \forall X \in M_2(F)$。
1) 证明 $\mathscr{A}$ 是 $M_2(F)$ 上的线性变换，并证明 $\mathscr{A}$ 的最小多项式和 $M$ 的最小多项式相同；
2) 若 $M = \begin{pmatrix} 1 & -4 \\ -1 & 4 \end{pmatrix}$，求 $\text{Ker}(\mathscr{A})$ 及 $\text{Im}(\mathscr{A})$ 的一组基，并说明 $\text{Ker}(\mathscr{A}) + \text{Im}(\mathscr{A})$ 是否是直和。)",
                             QuestionType::Subjective, 15,
                             R"(代入零化多项式等式 $f(\mathscr{A})(X) = X f(M)$ 证明。
\n【详细解析】
对任意多项式 $f$，有 $f(\mathscr{A})(X) = X f(M)$。若 $f(M)=0$ 则 $f(\mathscr{A})=0$；反之若 $f(\mathscr{A})=0$，取 $X=I$ 即得 $f(M)=0$。因此最小多项式相同。对于具体的 $M$，化简计算列向量和零空间即可。)",
                             {}, 0, "", false});

        // =====================================
        // 这是遗漏并补充的第三题！(Jordan 标准形)
        // =====================================
        list.append(Question{20235, source,
                             R"(【解答题（20分）】设 $\alpha_1, \alpha_2, \alpha_3, \alpha_4$ 是向量空间 $V$ 的一组基，线性映射 $\mathscr{A}$ 满足：
$$\mathscr{A}\alpha_1 = \alpha_1$$
$$\mathscr{A}\alpha_2 = 2\alpha_1 + \alpha_2$$
$$\mathscr{A}\alpha_3 = 3\alpha_1 + 2\alpha_2 + \alpha_3$$
$$\mathscr{A}\alpha_4 = 4\alpha_1 + 3\alpha_2 + 2\alpha_3 + \alpha_4$$
1) 求 $\mathscr{A}$ 的 Jordan 标准形和 $V$ 的一组 Jordan 基；
2) 求 $V$ 的所有 $\mathscr{A}$-不变子空间。(要求说明理由))",
                             QuestionType::Subjective, 20,
                             R"(1) Jordan 标准形为一个 4 阶 Jordan 块 $J_4(1)$，一组 Jordan 基为 $\{8\alpha_1, 12\alpha_1+4\alpha_2, 4\alpha_1+3\alpha_2+2\alpha_3, \alpha_4\}$；
2) 不变子空间共 5 个：$\{0\}$、$\text{Span}(\alpha_1)$、$\text{Span}(\alpha_1, \alpha_2)$、$\text{Span}(\alpha_1, \alpha_2, \alpha_3)$ 以及 $V$ 本身。
\n【详细解析】
1) 设 $\mathscr{A}$ 在基 $\alpha_1, \alpha_2, \alpha_3, \alpha_4$ 下的矩阵为 $A$。根据题意有：
$$A = \begin{pmatrix} 1 & 2 & 3 & 4 \\ 0 & 1 & 2 & 3 \\ 0 & 0 & 1 & 2 \\ 0 & 0 & 0 & 1 \end{pmatrix}$$
令 $N = A - I = \begin{pmatrix} 0 & 2 & 3 & 4 \\ 0 & 0 & 2 & 3 \\ 0 & 0 & 0 & 2 \\ 0 & 0 & 0 & 0 \end{pmatrix}$。
计算可得 $N^2 \neq 0, N^3 \neq 0, N^4 = 0$，且 $\text{Rank}(N) = 3$。
这说明 $A$ 只有一个对应特征值 1 的 4 阶 Jordan 块，即 Jordan 标准形为 $J_4(1)$。
求 Jordan 基：我们需要找一个循环向量 $v$ 使得 $N^3 v \neq 0$。取 $v = (0, 0, 0, 1)^T$，对应向量 $\beta_4 = \alpha_4$。
计算其循环空间链：
$\beta_3 = N\beta_4 = 4\alpha_1 + 3\alpha_2 + 2\alpha_3$
$\beta_2 = N\beta_3 = N(4\alpha_1 + 3\alpha_2 + 2\alpha_3) = 12\alpha_1 + 4\alpha_2$
$\beta_1 = N\beta_2 = N(12\alpha_1 + 4\alpha_2) = 8\alpha_1$
因此 $\{8\alpha_1, 12\alpha_1+4\alpha_2, 4\alpha_1+3\alpha_2+2\alpha_3, \alpha_4\}$ 就是一组 Jordan 基。

2) 因为 $\mathscr{A}$ 只有一个 Jordan 块，说明它的极小多项式等于特征多项式 $(\lambda-1)^4$。
此时 $\mathscr{A}$ 是一个循环变换（Cyclic operator）。对于这种只有一个 Jordan 块的幂零变换（平移后），它的不变子空间是完全确定的链式结构。
对于每一个维数 $k \in \{0, 1, 2, 3, 4\}$，存在且仅存在一个唯一的不变子空间，即 $\text{Ker}((\mathscr{A}-I)^k)$。
分别计算：
$k=0$: $\{0\}$
$k=1$: $\text{Ker}(N) = \text{Span}(\alpha_1)$
$k=2$: $\text{Ker}(N^2) = \text{Span}(\alpha_1, \alpha_2)$
$k=3$: $\text{Ker}(N^3) = \text{Span}(\alpha_1, \alpha_2, \alpha_3)$
$k=4$: $\text{Ker}(N^4) = V$
共 5 个不变子空间。)",
                             {}, 0, "", false});

        list.append(Question{202353, source,
                             R"(【解答题（15分）】欧式空间 $(V, (,))$ 的一组标准正交基为 $\eta_1, \eta_2, \dots, \eta_n$。任取 $k \in \mathbb{R}$ 和非零向量 $\xi \in V$，定义 $V$ 上的映射 $\mathscr{A}_\xi(\alpha) = k(\alpha, \xi)\xi - \alpha, \forall \alpha \in V$。
1) 证明 $\mathscr{A}_\xi$ 是线性变换，并求出它在标准正交基下的矩阵；
2) 确定 $k$ 的值使得 $\mathscr{A}_\xi$ 为 $V$ 上的正交变换。)",
                             QuestionType::Subjective, 15,
                             R"($k = \frac{2}{||\xi||^2}$
\n【详细解析】
正交变换要求矩阵 $P$ 满足 $P P^T = I$。
变换的矩阵形式可写作 $k \xi \xi^T - I$。
平方得 $(k \xi \xi^T - I)^2 = k^2 ||\xi||^2 \xi \xi^T - 2k \xi \xi^T + I$。要使其等于 $I$，要求 $k^2 ||\xi||^2 - 2k = 0$。因为 $\xi \neq 0$ 且变换不是恒等负值变换，解得 $k = \frac{2}{||\xi||^2}$。)",
                             {}, 0, "", false});

        list.append(Question{202354, source,
                             R"(【解答题（15分）】设 $\alpha_1, \alpha_2, \cdots, \alpha_s$ 是向量空间 $V$ 中的 $s$ 个非零向量。证明：存在线性函数 $f$ 使得
$$f(\alpha_i) \neq 0, \quad \forall i = 1, 2, \cdots, s$$)",
                             QuestionType::Subjective, 15,
                             R"(利用对偶空间和数域上的空间不可被有限个真子空间覆盖的性质证明。
\n【详细解析】
条件 $f(\alpha_i) = 0$ 定义了对偶空间 $V^*$ 中的一个余维数为 1 的子空间（超平面）$H_i$。实数域（或复数域）是无限域，有限个真子空间的并集绝对不可能覆盖整个空间。因此必定存在 $f \in V^*$ 且 $f \notin H_i$，即满足题意。)",
                             {}, 0, "", false});

        list.append(Question{202356, source,
                             R"(【解答题（15分）】设 $(V, (, ))$ 为复数域上的 $n(>2)$ 维非退化正交空间。
1) 证明 $V$ 中存在非零迷向向量和双曲平面；
2) 试确定 $V$ 的极大零内积子空间的维数。)",
                             QuestionType::Subjective, 15,
                             R"(1) 利用复数域内平方和可为零构造迷向向量。
2) 极大零内积子空间的维数为 $\lfloor \frac{n}{2} \rfloor$。
\n【详细解析】
1) 证明存在迷向向量：
因为 $V$ 是非退化的，且维数 $n > 2$，可以找到一组正交基 $e_1, e_2$ 使得 $(e_1, e_1) \neq 0, (e_2, e_2) \neq 0$。
由于在复数域 $\mathbb{C}$ 上，可以将它们规范化使得 $(e_1, e_1) = 1, (e_2, e_2) = 1$。
构造向量 $x = e_1 + i e_2$，其中 $i$ 是虚数单位。则 $(x, x) = (e_1, e_1) + i^2 (e_2, e_2) = 1 - 1 = 0$。
因为 $e_1, e_2$ 线性无关，$x \neq 0$。所以 $x$ 是一个非零迷向向量。
有了非零迷向向量，由于空间非退化，必然存在另一个迷向向量 $y$ 使得 $(x, y) \neq 0$，从而 $x$ 和 $y$ 张成一个双曲平面。

2) 极大零内积（全迷向）子空间的维数（即 Witt 指数）：
在复数域上，任何 $n$ 维非退化正交空间的度量矩阵都合同于单位矩阵 $I_n$。
即存在正交基使得任意向量的模长平方形式为 $x_1^2 + x_2^2 + \dots + x_n^2$。
令 $x_{2k-1} = i x_{2k}$，可以两两配对消去平方和。最多能配对出 $\lfloor n/2 \rfloor$ 个相互正交的双曲平面。
因此，极大零内积子空间的维数为 $\lfloor \frac{n}{2} \rfloor$。)",
                             {}, 0, "", false});

        list.append(Question{202357, source,
                             R"(【解答题（10分）】证明 $V$ 上秩为 $r$ 的线性变换的最小多项式的次数不超过 $r+1$。)",
                             QuestionType::Subjective, 10,
                             R"(利用像空间限制变换的降维性质构造零化多项式证明。
\n【详细解析】
设线性变换为 $\mathscr{A}$，其像空间 $\text{Im}(\mathscr{A})$ 的维数为 $r$（因为秩为 $r$）。
显然，像空间 $\text{Im}(\mathscr{A})$ 是 $\mathscr{A}$ 的不变子空间。
考虑 $\mathscr{A}$ 限制在 $\text{Im}(\mathscr{A})$ 上的诱导变换 $\mathscr{A}|_{\text{Im}(\mathscr{A})}$。由于 $\text{Im}(\mathscr{A})$ 的维数为 $r$，该诱导变换的最小多项式 $m_1(\lambda)$ 的次数不超过 $r$。
即对于任意 $\beta \in \text{Im}(\mathscr{A})$，都有 $m_1(\mathscr{A})(\beta) = 0$。
现在考虑整个空间 $V$ 中的任意向量 $\alpha$。显然 $\mathscr{A}(\alpha) \in \text{Im}(\mathscr{A})$。
因此，将 $m_1(\mathscr{A})$ 作用在 $\mathscr{A}(\alpha)$ 上必然得到零向量：
$$m_1(\mathscr{A})(\mathscr{A}(\alpha)) = 0 \implies (\mathscr{A} \cdot m_1(\mathscr{A}))(\alpha) = 0$$
这说明多项式 $f(\lambda) = \lambda m_1(\lambda)$ 是 $\mathscr{A}$ 的一个零化多项式。
由于 $f(\lambda)$ 的次数为 $\deg(m_1) + 1 \le r + 1$，而最小多项式 $m(\lambda)$ 必定整除任何零化多项式，故：
$$\deg(m) \le \deg(f) \le r + 1$$
定理得证。)",
                             {}, 0, "", false});
    }

/*
    // ==========================================
    // 试卷 3: 北京大学 2024-2025 高等代数 (II) 期中考_lww
    // ==========================================
    else if (examIndex == 3) {
        QString source = "北京大学 2024-2025 高等代数 (II) 期中考_lww";

        list.append(Question{202401, source,
                             R"(【解答题（20分）】对以下 Hermite 矩阵 $A$，求酉矩阵 $P$ 和对角阵 $D$ (要求对角元从大到小):
(a) $A = \begin{pmatrix} 3 & 2+2i \\ 2-2i & 1 \end{pmatrix}$
(b) $A = \begin{pmatrix} 3 & -i \\ i & 3 \end{pmatrix}$)",
                             QuestionType::Subjective, 20,
                             R"((a) $D = \text{diag}(5, -1)$；(b) $D = \text{diag}(4, 2)$
\n【详细解析】
求解特征方程：
(a) 特征方程 $\lambda^2 - 4\lambda - 5 = 0$，特征值 5 和 -1。分别求对应特征向量并归一化构成 $P$。
(b) 特征方程 $(\lambda-3)^2 - 1 = 0$，特征值 4 和 2。归一化特征向量构成 $P = \frac{1}{\sqrt{2}} \begin{pmatrix} 1 & 1 \\ i & -i \end{pmatrix}$。)",
                             {}, 0, "", false});

        list.append(Question{202402, source,
                             R"(【解答题（10分）】考虑 Hamilton 定义的四元数环 $\mathbb{H}$。
(a) 确定 $x^2 = 2$ 的所有解。
(b) 确定 $x^2 = -2$ 的所有解。)",
                             QuestionType::Subjective, 10,
                             R"((a) $\pm \sqrt{2}$；(b) $x = ai+bj+ck$，其中 $a,b,c \in \mathbb{R}$ 且 $a^2+b^2+c^2=2$。
\n【详细解析】
(a) 因为实数是 $\mathbb{H}$ 的中心，方程可分解为 $(x-\sqrt{2})(x+\sqrt{2}) = 0$，因为 $\mathbb{H}$ 是除环，所以只有 $\pm \sqrt{2}$。
(b) 令 $y = x/\sqrt{2}$，则 $y^2 = -1$。其解构成一个二维球面 $y = ai+bj+ck$ ($a^2+b^2+c^2=1$)。所以 $x$ 的系数满足 $a^2+b^2+c^2=2$。)",
                             {}, 0, "", false});

        list.append(Question{202403, source,
                             R"(【解答题（10分）】考虑 $L = A+iB$，其中 $A$ 和 $B$ 都是 Hermite 矩阵。设 $A$ 正定。
(a) 证明 $|\det L| \ge \det A$。
(b) 确定等号成立的充要条件。)",
                             QuestionType::Subjective, 10,
                             R"(利用矩阵酉变换证明，等号成立当且仅当 $B = 0$。
\n【详细解析】
利用 $A$ 的正定性，存在可逆矩阵 $P$ 使得 $P^T A P = I$。
此时 $L$ 变为 $I + iB'$。其行列式的绝对值的平方等于 $\det(I + (B')^2)$。因为 $(B')^2$ 是半正定矩阵，所以该行列式 $\ge 1$。当且仅当 $B'=0$ (即 $B=0$) 时等号成立。)",
                             {}, 0, "", false});

        list.append(Question{202404, source,
                             R"(【解答题（20分）】判断下列有理系数多项式的可约性（不必过程）：
(a) $2X^4 - X^3 + 2X - 3$
(b) $-7X^4 + 25X^2 - 15X + 10$
(c) $X^4 + X^2 + 1$
(d) $X^4 + 4X + 1$)",
                             QuestionType::Subjective, 20,
                             R"((a) 可约; (b) 不可约; (c) 可约; (d) 不可约。
\n【详细解析】
(a) 显然有根 $X=1$，可约。
(b) 应用 Eisenstein 判别法，取 $p=5$ 即可证明不可约。
(c) 配方得 $(X^2+X+1)(X^2-X+1)$，可约。
(d) 整体平移 $f(X+1) = X^4 + 4X^3 + 6X^2 + 8X + 6$，应用 Eisenstein 判别法 (取 $p=2$)，不可约。)",
                             {}, 0, "", false});

        list.append(Question{202405, source,
                             R"(【解答题（10分）】证明如果 $A \in M_n(\mathbb{C})$ 的特征多项式 $\text{Char}_A$ 是整系数多项式，则对于所有正整数 $p$，$\text{Char}_{A^p}$ 也都是整系数多项式。)",
                             QuestionType::Subjective, 10,
                             R"(利用对称多项式基本定理证明。
\n【详细解析】
设 $A$ 特征值为 $\lambda_i$。则 $A^p$ 特征值为 $\lambda_i^p$。
$A^p$ 的特征多项式系数实际上是 $\lambda_1^p, \dots, \lambda_n^p$ 的基本对称多项式。根据对称多项式基本定理，它们可以表示为原来的初等对称多项式（即 $A$ 的特征多项式的整系数）的整系数多项式。整系数的整系数代数操作当然还是整数。)",
                             {}, 0, "", false});
    }*/

    return list;
}

} // namespace AlgeMate::Learning