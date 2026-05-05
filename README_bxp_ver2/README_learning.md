# 学习中心模块 (Learning)

## 职责
Learning 是整个系统的**核心学习入口 Dashboard**，采用四大功能分区卡片结构：

### 四大核心模块
在主界面中以四个大卡片呈现：

#### 1️⃣ 知识点学习（Knowledge）
- 线性代数知识体系学习入口
- 章节树 + Markdown 讲解
- 例题 + 思考题展示
- 支持跳转到对应练习

---

#### 2️⃣ 练习模式（Practice）
包含三大子模块：

##### ● 计算题模块（CalculationProblem）
- 矩阵运算
- 行列式
- 逆矩阵
- 秩
- 线性方程组

##### ● 章节练习（Chapter Practice）
- 与 Knowledge 章节绑定
- 按当前学习章节自动出题
- 支持针对性训练

##### ● 专题练习（Topic Practice）
- 多章节融合训练
- 如：rank不等式专题
- 支持综合能力提升

---

#### 3️⃣ 考试模式（Exam）
- 模拟真实考试环境
- 限时组卷
- 自动评分
- 成绩分析 + 错题自动归档

---

#### 4️⃣ 错题本（Wrong Book）
- 自动收集错题
- 按章节/专题分类
- 支持重做训练
- 掌握状态管理（未掌握 / 待重做 / 已掌握）

---

## 页面跳转逻辑
- 点击“今日学习 / 本周进度” → Learning Center
- 点击四大模块卡片 → 各子系统页面
- 练习 / 考试产生错题 → 自动写入 WrongBook

---

## 对外接口
```cpp
AlgeMate::Learning::LearningPage : public QWidget
```

---
## 分工建议
- LearningPage：负责 Dashboard UI（四大卡片布局）
- LearningCenterPage：负责统计分析展示
- Practice 模块独立拆分：CalculationProblemPage         ChapterPracticePage
 TopicPracticePage
- Knowledge → Practice 通过 chapter_id 绑定
- Exam → WrongBook → Practice 构成闭环
- 数据层统一使用 QtSql + SQLite

---
## 第一步
- 设计四大模块 UI 卡片布局（Knowledge / Practice / Exam /   WrongBook）
- 实现卡片点击跳转机制
- 设计 study_session(date, minutes, topic) 表
- LearningPage 启动时加载统计数据
