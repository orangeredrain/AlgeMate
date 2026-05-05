# Test Center（测试中心）

**临时调试模块**，用于验证 `src/latex/` 中 LaTeX 渲染器的功能以及各种 Qt 模块的使用，**将在正式发布前删除**。

## 功能

- 上方输入框（等宽字体）：编写 LaTeX 代码
- 下方预览区：实时渲染结果
- 支持 `$...$` / `$$...$$` / `\[...\]` / 矩阵 / 自定义宏 / 自定义命令
- 复制预览区公式可还原为 LaTeX 源码（使用 `LatexTextBrowser`）

## 使用

运行 AlgeMate → 侧边栏 “测试中心”，输入 LaTeX 代码即可实时看到渲染效果。

## 注意事项

- 本模块**不参与正式业务逻辑**，仅供开发阶段验证 LaTeX 渲染
- `LatexRenderer` 的 API 用法以 `src/latex/README.md` 为准
- 正式版删除时：移除 `src/modules/test_center/` 目录，清理 `MainWindow.cpp` 和根 `CMakeLists.txt` 中的引用
