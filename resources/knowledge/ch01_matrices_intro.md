1.2 矩阵与方程组简介

矩阵是按矩形排列的数表；线性方程组可用矩阵形式 $A\mathbf{x}=\mathbf{b}$ 紧凑书写。

讲解提要

系数矩阵 $A$、未知数向量 $\mathbf{x}$、常数项向量 $\mathbf{b}$。高斯消元对应矩阵的行变换思想（后续章节展开）。

例题

写出方程组 $x+2y=3,\;4x+5y=6$ 的矩阵形式 $A\mathbf{x}=\mathbf{b}$，并写出 $A,\mathbf{x},\mathbf{b}$。

解：将系数、未知数、常数项分别写成矩阵：
$$
A = \left(\begin{array}{cc}1&2\\4&5\end{array}\right),\quad
\mathbf{x} = \left(\begin{array}{cr}x&\\y&\end{array}\right),\quad
\mathbf{b} = \left(\begin{array}{cr}3&\\6&\end{array}\right)
$$
则方程组等价于：
$$
\left(\begin{array}{cc}1&2\\4&5\end{array}\right)
\left(\begin{array}{cr}x&\\y&\end{array}\right)
= \left(\begin{array}{cr}3&\\6&\end{array}\right)
$$

思考题

什么样的方程组"无解""唯一解""无穷多解"？与矩阵的秩有何关系？
