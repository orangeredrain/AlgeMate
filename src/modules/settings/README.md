# 设置中心模块 (Settings)

## 职责
应用级偏好：主题 / 字号 / 账号 / AI API Key / 快捷键 / 数据导入导出。

## 对外接口
```cpp
AlgeMate::Settings::SettingsPage : public QWidget
```

## 分工建议
- 统一使用 `QSettings`（已通过 `AppPaths` 中的 `appName/orgName` 自动定位存储位置）。
- 主题切换直接调 `ThemeManager::instance().applyTheme(...)`（已示范）。
- 敏感信息（API Key）请用 `QSettings` 系统作用域或后续加轻量加密。

## 第一步
1. 封装 `SettingsKeys` 常量命名空间（避免字符串散落）。
2. 读写主题/字号初值，启动时恢复。
