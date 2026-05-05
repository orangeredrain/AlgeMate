# 章节练习模块 (Chapter Practice)

## 职责
章节练习模块用于将 Knowledge 中的“章节学习内容”转化为**针对性训练题目**，实现“学 → 练”的闭环。

核心特点：
- 与 Knowledge 章节严格绑定
- 按章节自动生成或加载题目
- 强化“刚学就练”的即时反馈机制
- 是 Practice 模块中最基础也是最重要的一环

---

## 对外接口
```cpp
AlgeMate::Practice::ChapterPracticePage : public QWidget
```

## 分工建议
### 数据来源：
- Knowledge 章节体系（chapter_id）
- 章节题库（SQLite / JSON）
### 数据结构建议：
- chapter_id
- chapter_name
- question_list
- difficulty_level
### UI设计：
- 左侧：章节选择列表（与 Knowledge 同步）
- 右侧：当前章节练习界面
### 功能逻辑：
- 支持从 Knowledge 页面直接跳转
- 支持“当前章节快速练习”
- 错题自动进入 WrongBook
### 与 TopicPractice 的区别：
- ChapterPractice：单章节训练（基础）
- TopicPractice：跨章节综合训练（进阶）

## 第一步
### 实现 chapter_id 与 Knowledge 章节树的映射关系
### 构建章节 → 题目加载逻辑（mock 数据即可）
### 完成章节练习 UI 框架：
- 章节选择
- 题目展示
- 答题提交
### 实现“从 Knowledge 一键进入当前章节练习”