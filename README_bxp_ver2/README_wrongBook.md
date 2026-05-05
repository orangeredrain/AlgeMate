
---

# 📓 WrongBook（错题本）


# 错题本模块 (WrongBook)

## 职责
管理所有错题：
- 自动收集错题
- 分类（章节 / 专题）
- 重做训练
- 掌握状态管理

---

## 状态定义
未掌握 → 新错题  
待重做 → 错过一次  
已掌握 → 正确掌握  

---

## 对外接口
```cpp
AlgeMate::Learning::WrongBookPage : public QWidget
```

## 分工建议
### SQLite 存储错题数据
### 数据来源：
- Practice
- Exam
### UI设计：
- 左：分类列表
- 右：题目展示
- 支持一键进入练习

## 第一步
- 设计 wrong_question 表
- 实现错题写入机制
- 完成错题列表展示