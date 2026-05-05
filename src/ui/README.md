# UI

应用界面层，包含主窗口、标题栏、侧边栏导航、用户资料弹窗。命名空间 `AlgeMate`。

## MainWindow

继承 `QMainWindow`，组合标题栏、侧边栏、页面栈。

```
┌─────────────┬────────────────────────────────┐
│  TitleBar   │  ← light/dark / 字体 / 用户    │
├─────────────┤                                  │
│             │                                  │
│ Navigation  │  QStackedWidget                 │
│    Bar      │  (各模块 Page 切换)               │
│             │                                  │
└─────────────┴────────────────────────────────┘
```

`composeLayout()` 构建水平布局：左侧 `NavigationBar`（220px），右侧 `QStackedWidget`。
`registerModules()` 注册所有功能模块（首页、计算助手、AI 解题、知识点、学习中心、测试中心、设置），每项含图标 emoji、中文标题和 Page 实例。

## TitleBar

顶部操作栏。包含：
- 主题切换按钮（亮色/暗色）
- 字体缩放按钮（`A⁻` / `A⁰` / `A⁺`）
- 用户头像和名称，点击弹出 `UserProfileDialog`

所有按钮通过 `ThemeManager::instance()` 和 `UserProfile::instance()` 单例与全局状态同步。

## NavigationBar

左侧侧边栏。内部使用 `QListWidget`，每个 item 为 emoji 图标 + 中文标题。

```cpp
nav->addNavItem(QStringLiteral("🧮"), QStringLiteral("计算助手"));
nav->setCurrentIndex(2);              // 切换到第 3 个 tab
connect(nav, &NavigationBar::navigated, stack, &QStackedWidget::setCurrentIndex);
```

## UserProfileDialog

继承 `QDialog`，模态弹窗。提供：
- 用户昵称编辑
- 头像选择/移除
- 修改后同步到 `UserProfile` 单例并发射 `profileChanged` 信号
