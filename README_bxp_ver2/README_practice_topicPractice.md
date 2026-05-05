
---

# 📒 Topic Practice（专题练习模块）


# 专题练习模块 (Topic Practice)

## 职责
### 综合知识点强化训练，如：
- rank不等式专题
- 线性空间专题

---

## 对外接口
```cpp
AlgeMate::Practice::TopicPracticePage : public QWidget
```

## 分工建议
### 专题 = 多章节组合
### 数据结构：
- topic.json
### 每个专题包含：
- 题库集合
### 支持 AI 推荐薄弱专题

## 第一步
- 定义 topic.json
- 实现专题列表 UI
- 完成专题 → 题目加载 →可点击查看题目解析