
# 📕 CalculationProblem（计算题生成模块）

# 计算题生成模块（Calculation Practice）

## 职责
## 通过随机数生成各种不同类型的计算题，例如：
- 矩阵加减乘
- 行列式
- 逆矩阵
- 秩
- 线性方程组

---

## 对外接口
```cpp
AlgeMate::Practice::CalculationProblemPage : public QWidget
```

## 分工建议
### 使用 Core 数学计算模块（C++）
### UI设计：
- QTableWidget 输入答案
- 结果输出区（正确与否）
### 功能扩展：
- 随机出题
- 判断正误
- 步骤解析（可选）

## 第一步
- 通过实现随机题生成，来生成计算题