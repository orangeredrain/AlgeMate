# 计算助手模块 (Calculator)

## 概述

矩阵代数计算与可视化模块，包含三个子面板：**交互式计算**、**算法演示**、**可视化**。

## 目录结构

```
calculator/
├── CalculatorPage.h/cpp         #   三标签页外壳
├── README.md
├── CMakeLists.txt
├── interactive/                 #   Part 1 : 交互式 Notebook UI
│   ├── InteractivePage.h/cpp    #   主交互界面（输入/输出单元格）
│   ├── HelpDialog.h/cpp         #   帮助对话框
│   ├── LatexTextBrowser.h/cpp   #   LaTeX 渲染文本浏览器
│   └── expr/                    #   表达式引擎
│       ├── Lexer.h/cpp          #   词法分析
│       ├── Parser.h/cpp         #   语法分析
│       ├── Evaluator.h/cpp      #   求值器（调用算法库）
│       ├── Value.h/cpp          #   值类型
│       └── RenderSettings.h/cpp #   主题/渲染设置
├── demo/                        #   Part 2 : 算法演示
│   ├── DemoPage.h/cpp           #   算法目录页
│   ├── DemoCommon.h             #   共享工具（LaTeX渲染、矩阵格式化等）
│   ├── HomoLinearSystemPage     #   齐次线性方程组
│   ├── NonhomoLinearSystemPage  #   非齐次线性方程组
│   ├── MaxIndepPage             #   极大线性无关组
│   ├── InversePage              #   矩阵求逆
│   ├── GSOPage                  #   Schmidt 正交化
│   ├── EigenPage                #   特征值与特征向量
│   ├── SymDiagPage              #   实对称矩阵对角化
│   ├── QuadFormPage             #   实二次型化标准形
│   ├── PolyGCDPage              #   多项式最大公因式
│   ├── SymReducePage            #   对称多项式化简
│   └── JordanFormPage           #   Jordan 标准形
└── visualize/                   #   Part 3 : 可视化
    ├── VisualizePage.h/cpp      #   二次曲面分类与可视化面板
    └── QuadricWidget.h/cpp      #   OpenGL 3D 曲面渲染器
```

## 交互式 (interactive)

### 核心功能
- 支持 **分数**、**矩阵**、**多项式** 运算  
- **Notebook 风格 UI**：输入单元格 → 输出单元格  
- **LaTeX 数学渲染**（输出公式）

### 计算模式
- **精确计算**（如分数运算，保持有理数精度）  
- **数值计算**（浮点数计算，用户可设置保留小数位数）

### 辅助功能
- **帮助文件**：包含所有函数的使用说明  
- **自动补全**：提升用户输入效率

## 算法演示 (demo)

### 添加新 Demo

1. 新建 `demo/NewAlgoPage.h` 和 `demo/NewAlgoPage.cpp`，参照 `InversePage` 模式：
   - 继承 `QWidget`，声明 `backRequested()` 信号
   - 构造 UI：顶栏（返回按钮 + 标题）→ 参数区 → 结果区 (`QTextBrowser`)
   - 使用 `DemoCommon.h` 中的 `matLtx()`、`fracLtx()`、`polyLtx()`、`formulaHtml()` 等生成 LaTeX 输出
2. 在 `DemoPage.cpp` 中 `#include`、实例化、连接 `backRequested`、`addCard()` 注册
3. 在 `CMakeLists.txt` 的 `qt_add_library` 源列表中添加新文件

### 已注册的 Demo 页

| 页面 | 卡片图标 | 算法简述 |
|------|---------|---------|
| 齐次线性方程组 | `$AX=0$` | 高斯消元 → RREF → 基础解系 |
| 非齐次线性方程组 | `$AX=b$` | 高斯消元 → RREF → 特解 + 基础解系 |
| 极大线性无关组 | `$\operatorname{rank}(A)$` | 向量组 → 行变换 → 秩 + 极大无关组 |
| 矩阵求逆 | `$A^{-1}$` | (A, I) → 初等行变换 → (I, A⁻¹) |
| Schmidt 正交化 | `$\alpha\perp\beta$` | 极大无关组 → 正交化 → 单位化 |
| 特征值与特征向量 | `$A\xi=\lambda\xi$` | 特征多项式 → 特征值 → 特征向量 |
| 实对称矩阵对角化 | `$T^{-1}AT=\Lambda$` | 特征值 → 特征向量 → 正交化 → T |
| 实二次型化标准形 | `$f=X^{T}AX$` | 成对初等行/列变换 → 合同对角化 |
| 多项式最大公因式 | `$\gcd(f,g)$` | 辗转相除法 → 倍式和表示 |
| 对称多项式化简 | `$g(\sigma_1,\cdots,\sigma_n)$` | 字典序降次法 → 初等对称多项式 |
| Jordan 标准形 | `$P^{-1}AP=J$` | λ-矩阵 → 行列式因子 → 不变因子 → 初等因子 → Jordan 标准形 |

## 可视化 (visualize)

### 二次曲面分类与 3D 渲染

- **QuadricWidget**：基于 `QOpenGLWidget` 的 3D 曲面渲染器
  - GLSL 330 着色器，Phong 光照 + Fresnel 边缘光 + 半球环境光
  - 鼠标拖拽旋转（Arcball）、滚轮缩放
  - 支持主曲面 + 第二曲面（双叶双曲面、双曲柱面等两叶/两支类型）
  - 参数曲面采样，数值法线，两遍绘制（背面 + 正面）
  - 坐标轴（带箭头）、网格平面

- **VisualizePage**：
  - 左侧面板：方程输入 Tab（10 个系数） / 预设选择 Tab（9 种实曲面类型 + 参数调节）
  - 右侧：3D 视图
  - 分类器 `classifyQuadric()`：由 10 个系数（a₁₁~a₀）计算特征值、惯性指数，判定曲面类型
  - 预设曲面类型：椭球面、单/双叶双曲面、椭圆/双曲抛物面、实二次锥面、椭圆柱面、双曲柱面、抛物柱面

### 依赖

- **Qt6::OpenGLWidgets**（根 `CMakeLists.txt` 已添加）
- **数学库** (`algemate_math`)：`Fraction`、`Matrix`、`Polynomial`
- **JKQTMathTextLib**：LaTeX 数学公式渲染

## 模块编组

- 向 `MainWindow` 仅导出 `CalculatorPage`（三标签页容器）
- 不依赖其他 `modules/` 目录
- 新增 `.h/.cpp` 文件需同时追加到本目录的 `CMakeLists.txt`
