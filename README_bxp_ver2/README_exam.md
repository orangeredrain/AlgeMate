
---

# 📔 Exam（考试模式）

# 考试模式模块 (Exam)

## 职责
模拟真实考试系统：
- 限时考试
- 自动组卷
- 成绩分析
- 错题自动归档

---

## 对外接口
```cpp
AlgeMate::Exam::ExamPage : public QWidget
```

## 分工建议
### 组卷策略：
- 按知识点权重随机抽题
### UI结构：
- 倒计时组件
- 分页答题系统
### 数据流：
- Exam → WrongBook → Practice

## 第一步
- 实现 mock exam（固定题库）
- 加入倒计时
- 实现提交评分