# 学习管理中心模块 (Learning Center)

## 职责
Learning Center 用于集中管理与分析用户学习数据，是整个系统的学习统计与反馈中心。

主要功能包括：
- 今日学习统计
- 本周学习进度分析
- 学习时长趋势
- 打卡记录（日历）
- 错题趋势分析
- 学习状态反馈

该模块用于承接：
- Learning Dashboard 中“今日学习”
- Learning Dashboard 中“本周进度”

用户点击对应统计卡片后跳转进入。

---

## 对外接口
```cpp
AlgeMate::Learning::LearningCenterPage : public QWidget
```


## 分工建议
### 数据来源：
- study_session 表
- wrong_question 表
- exam_record 表
### 页面结构建议：
- 顶部：统计卡片
- 今日学习时长
- 本周完成题数
- 错题数量
- 学习连续天数
- 中间：学习趋势图
- 底部：打卡日历 / 学习记录列表
### 可视化方案：
- MVP：QPainter 自绘
- 进阶：QtCharts
### 与 Learning 模块联动：
- Learning Dashboard 点击统计卡片跳转
- 与 WrongBook 联动：
- 展示错题变化趋势
### 后期可扩展：
- AI 学习分析
- 学习建议推荐
- 薄弱章节预测


## 第一步
- 设计 study_session(date, minutes, topic) 表
- 设计 exam_record(score, time, date) 表
- 实现 LearningCenterPage 基础 UI：统计卡片
 趋势图占位
 打卡日历占位
- Learning 页面实现跳转逻辑
