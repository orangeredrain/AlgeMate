2.1 矩阵乘法

讲解提要

若 $A$ 为 $m\times n$ 矩阵，$B$ 为 $n\times p$ 矩阵，则乘积 $AB$ 为 $m\times p$ 矩阵，第 $(i,j)$ 元为 $A$ 的第 $i$ 行与 $B$ 的第 $j$ 列对应分量乘积之和。

设 $A=(a_{ij})_{m\times n},\;B=(b_{ij})_{n\times p}$，则 $C=AB=(c_{ij})_{m\times p}$，其中：
$$
c_{ij}=a_{i1}b_{1j}+a_{i2}b_{2j}+\cdots+a_{in}b_{nj}=\sum_{k=1}^{n}a_{ik}b_{kj}
$$

例题

计算矩阵乘积：
$$
\left(\begin{array}{cc}1&2\\3&4\end{array}\right)
\left(\begin{array}{cc}5&6\\7&8\end{array}\right)
$$

解：按定义逐元计算：
$$
\left(\begin{array}{cc}1&2\\3&4\end{array}\right)
\left(\begin{array}{cc}5&6\\7&8\end{array}\right)
= \left(\begin{array}{cc}
1\cdot5+2\cdot7 & 1\cdot6+2\cdot8\\
3\cdot5+4\cdot7 & 3\cdot6+4\cdot8
\end{array}\right)
= \left(\begin{array}{cc}19&22\\43&50\end{array}\right)
$$

思考题

矩阵乘法为什么一般不满足交换律？即为什么通常 $AB\neq BA$？
