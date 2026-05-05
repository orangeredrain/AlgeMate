# Core

应用基础设施层，提供主题管理、路径配置、用户资料。命名空间 `AlgeMate`。

## AppPaths

应用元信息和版本号查询。编译期嵌入版本号。

```cpp
QString name = AppPaths::appName();          // "AlgeMate"
QString disp = AppPaths::appDisplayName();   // 显示名
QString ver  = AppPaths::appVersion();       // 版本号
// QSS 资源路径: AppPaths::kLightQss, AppPaths::kDarkQss
```

## ThemeManager

全局单例，管理 Light/Dark 主题切换和字体缩放。

```cpp
ThemeManager& tm = ThemeManager::instance();

tm.applyTheme(ThemeManager::Theme::Light);   // 亮色主题
tm.applyTheme(ThemeManager::Theme::Dark);    // 暗色主题
bool isDark = tm.currentTheme() == ThemeManager::Theme::Dark;

tm.setFontScale(1.2);                        // 字体 120%
tm.increaseFont();                           // +0.1
tm.decreaseFont();                           // -0.1
tm.resetFont();                              // 重置
double s = tm.fontScale();

// 信号
connect(&tm, &ThemeManager::themeChanged, ...);
connect(&tm, &ThemeManager::fontScaleChanged, ...);
```

## UserProfile

全局单例，管理用户名、头像、个性化问候。

```cpp
UserProfile& up = UserProfile::instance();

QString name = up.userName();
QPixmap avatar = up.avatarPixmap(48);

up.setUserName("张三");
up.setAvatarPath(":/avatar.png");

QString greet = UserProfile::greetingByTime();  // "早上好" / "下午好" / "晚上好"

// 信号
connect(&up, &UserProfile::profileChanged, ...);
```
