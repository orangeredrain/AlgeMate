# Learning（学习中心）

线性代数学习系统的核心模块，整合知识点学习、练习、考试、错题管理四大功能，通过内部 `QStackedWidget` 实现子页面切换。

## 架构

```
LearningPage (QStackedWidget 容器)
├── [0] Dashboard          — 主面板：统计卡片 + 四大入口
├── [1] KnowledgePage      — 知识点学习：章节树 + Markdown 讲解
├── [2] PracticePage       — 练习模式入口：三种练习方式
├── [3] CalculationProblemPage  — 计算题练习
├── [4] ChapterPracticePage     — 对应章节练习
├── [5] TopicPracticePage       — 专题模式练习
├── [6] ExamPage           — 考试模式
├── [7] WrongBookPage      — 错题本
└── [8] LearningCenterPage — 学习数据统计
```

所有子页面提供 `backRequested()` 信号，统一返回 Dashboard。

## 页面说明

### Dashboard（主面板）

- **上方**：四个统计卡片（今日学习、本周进度、错题数、推荐练习），点击 → LearningCenterPage
- **右上角**："学习管理中心 →" 链接
- **下方**：2×2 模块入口卡片：

| 卡片 | 跳转 | 说明 |
|------|------|------|
| 📗 知识点学习 | KnowledgePage | 线性代数章节树 · 例题与思考题 |
| 📙 练习模式 | PracticePage | 计算题 · 章节练习 · 专题训练 |
| 📔 考试模式 | ExamPage | 模拟真实考试 · 限时答题 · 自动评分 |
| 📓 错题本 | WrongBookPage | 错题归档 · 分类管理 · 重做复习 |

### KnowledgePage（知识点学习）

- 左侧：章节树（`QTreeWidget`），节点绑定 `resources/knowledge/*.md` 的 qrc 路径
- 右侧：`QTextBrowser` 使用 `setMarkdown()` 渲染内容
- 顶栏："进入章节练习 →" 按钮跳转 ChapterPracticePage
- 内容文件位于 `resources/knowledge/`，通过 `resources.qrc` 打包

### PracticePage（练习模式入口）

三种练习子模式卡片：
- 🔢 **计算题** → CalculationProblemPage（随机生成矩阵运算题，实时输入与验证）
- 📖 **对应章节练习** → ChapterPracticePage（按当前章节针对性训练，与知识点章节绑定）
- 🎯 **专题模式** → TopicPracticePage（跨章节综合训练：特征值专题、线性空间专题等）

### 子模式页面

| 页面 | 功能（待实现） |
|------|---------------|
| CalculationProblemPage | 随机生成计算题，输入矩阵并实时计算，显示标准答案 |
| ChapterPracticePage | 按章节组织题目，自动加载对应章节练习，错题自动入错题本 |
| TopicPracticePage | 跨章节综合训练，按专题组织题目，支持难度分级 |

### ExamPage（考试模式）

模拟真实考试环境：自动组卷、限时答题、自动评分、成绩统计、错题自动归档、历史记录保存。

### WrongBookPage（错题本）

自动收集所有错题，支持按章节/专题分类，管理掌握状态（未掌握/待重做/已掌握），支持错题重做与跳转对应练习。

### LearningCenterPage（学习管理中心）

展示学习数据统计：今日学习时长、本周进度、连续打卡、错题趋势、学习趋势图。

## 导航流程

```
Dashboard
  ├─ 统计卡片 → LearningCenterPage
  ├─ 📗 知识点学习 → KnowledgePage
  │     └─ "进入章节练习" → ChapterPracticePage
  ├─ 📙 练习模式 → PracticePage
  │     ├─ 🔢 计算题 → CalculationProblemPage
  │     ├─ 📖 章节练习 → ChapterPracticePage
  │     └─ 🎯 专题模式 → TopicPracticePage
  ├─ 📔 考试模式 → ExamPage
  └─ 📓 错题本 → WrongBookPage

所有子页面: ← 返回 → Dashboard
```

## 文件结构

```
src/modules/learning/
├── CMakeLists.txt
├── README.md
├── ClickableCard.h              # 可点击卡片组件（QFrame 子类）
├── LearningPage.h/.cpp          # 主容器 + Dashboard
├── KnowledgePage.h/.cpp         # 知识点学习
├── PracticePage.h/.cpp          # 练习入口 + 3 个子模式页面
├── ExamPage.h/.cpp              # 考试模式
├── WrongBookPage.h/.cpp         # 错题本
└── LearningCenterPage.h/.cpp    # 学习管理中心
```

## 主题支持

页面通过 QSS 对象名适配亮/暗主题：
- 卡片使用 `QFrame#Card` / `QWidget#Card` 规则（`light.qss` / `dark.qss`）
- 标题使用 `QLabel#PageTitle`、副标题 `QLabel#PageSubtitle`
- 占位文本使用 `QLabel#PlaceholderLabel`
- 返回按钮使用 `QPushButton#LearnBackBtn`
- 知识点树 `QTreeWidget#KnowledgeTree`、内容 `QTextBrowser#KnowledgeContent`

## 依赖

- Qt6（Core, Gui, Widgets）
- `algemate_latex`（知识点页使用 LatexRenderer 渲染公式，待接入）
- `resources/knowledge/*.md`（知识点 Markdown 内容）

## 扩展指南

1. **添加新章节**：在 `KnowledgePage::buildChapterTree()` 中添加节点和对应 md 文件
2. **新增 markdown 内容**：在 `resources/knowledge/` 下创建 `.md` 文件，并在 `resources.qrc` 中注册
3. **实现练习功能**：修改 `CalculationProblemPage` 等子页面，接入 `algemate_math` 出题/判题
4. **接入数据持久化**：错题本和考试记录需要 SQLite（计划使用 `QtSql`）
