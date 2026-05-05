# 第三方库

## JKQtPlotter（JKQTMathText 子集）

**版本**: 5.0.0  
**许可**: Apache-2.0  
**上游**: https://github.com/jkriege2/JKQtPlotter

本项目的图表和数学渲染库。编译时仅启用 `JKQTMathTextLib` 子库（`JKQtPlotter_BUILD_ONLY_MATHTEXT ON`），禁用其他所有组件（examples、tools、shared libs）。

### JKQTMathText

纯 Qt/C++ 的 LaTeX 数学公式渲染库，通过 `QPainter` 原生绘制，不依赖 Web 引擎。使用 XITS 数学字体。

- 支持 `\frac`、`\sqrt`、`\sum`、`\int`、矩阵环境、希腊字母、`\mathbb`、`\mathcal` 等
- 3× 超采样 + 屏幕 DPR 自适应
- 输出为 `QPainter` 绘制或 `QPixmap`

项目中用于 `src/latex/` 模块的数学公式渲染。

## QCustomPlot

**许可**: GPL v3  
**上游**: https://www.qcustomplot.com/

轻量级 Qt 图表库，用于绘制折线图、散点图、柱状图等。项目中用于可视化模块的 2D 图表绘制（`VisualizePage` 等）。
