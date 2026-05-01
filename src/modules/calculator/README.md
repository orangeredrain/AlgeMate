# 计算助手模块 (Calculator)

## 职责
矩阵计算器 + 方程组求解 + 行列式 / 秩 / 逆 / 伴随 / 特征值 等高级运算。参考 UI3 的"矩阵计算器"面板。

## 对外接口
向 `MainWindow` 仅导出：
```cpp
AlgeMate::Calculator::CalculatorPage : public QWidget
```

## 分工建议
- 在本目录内自由增删 `.h / .cpp / .ui` 文件，新增文件需追加到本目录 `CMakeLists.txt` 的 `qt_add_library` 源列表。
- 数值算法推荐新建 `core/` 子目录，先在 VS2022 里写纯 C++ 实现，再在 `CalculatorPage` 里调用。
- 不要 `#include` 其他 `modules/` 目录的头文件 —— 模块间解耦。

## 第一步
1. 新增 `core/Matrix.h`，定义支持 `Fraction` 与 `double` 双模式的矩阵类。
2. 拆子面板：输入区、运算按钮区、结果展示区。
