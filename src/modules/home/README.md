# 首页模块 (Home)

## 职责
展示按时段的个性化问候 + 快捷入口（跳转到其他四个业务模块）。

## 对外接口
```cpp
AlgeMate::Home::HomePage : public QWidget
```

## 与其他模块的约定
- 读取 `AlgeMate::UserProfile` 显示用户名与头像。
- 未来跳转其他模块时，通过 `core/EventBus`（待创建）发信号，不要 `#include` 其他 modules。
