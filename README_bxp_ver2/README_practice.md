
---

# 📙 Practice（练习模式总模块）


# 练习模式模块 (Practice)

## 职责
统一练习入口，包含三大模式(板块)：
- 计算题模块（Calculation）
- 章节练习（Chapter Practice）
- 专题练习（Topic Practice）

用于承接 Knowledge 学习内容并强化训练。

---

## 对外接口
```cpp
AlgeMate::Practice::PracticePage : public QWidget
```

## 分工建议
### 子模块拆分：
- CalculationPage
- ChapterPracticePage
- TopicPracticePage
### 数据来源：
- Knowledge（章节映射）
- SQLite / JSON 题库
### UI结构：
- 左：练习类型选择
- 右：题目展示区
- 错题自动写入 WrongBook
- 根据统计数据推荐联系类型 或者 根据ai推荐练习（可选）

## 第一步
- 搭建 PracticePage 框架
- 实现三种模式切换
- 接入 chapter_id → 题目映射