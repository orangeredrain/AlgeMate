# 学习中心模块 (Learning)

## 职责
Dashboard 风格的学习记录面板：今日学习 / 本周进度 / 错题本 / 推荐练习 / 打卡日历。参考 UI1 / UI2 / UI4 配色与布局。

## 对外接口
```cpp
AlgeMate::Learning::LearningPage : public QWidget
```

## 分工建议
- 数据持久化推荐 `QtSql + SQLite`，在本目录新增 `db/Schema.h`。需在 CMakeLists 追加 `Qt6::Sql`。
- 图表 MVP 阶段可自绘 `QPainter`，后期需要复杂图表再引 `Qt6::Charts`。
- 已内置 4 张统计占位卡片，可直接改数据源。

## 第一步
1. 设计 `study_session(date, minutes, topic)` 表。
2. `LearningPage` 启动时查库填充 4 张卡片。
