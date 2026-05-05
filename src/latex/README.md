# LaTeX 渲染模块

基于 JKQTMathText 的通用 LaTeX 渲染器，输入混合中文 + 数学公式的 LaTeX 源码，输出可直接 `setHtml()` 到 `QTextBrowser` 的 HTML。

## 快速开始

```cpp
#include "latex/LatexRenderer.h"
#include "latex/LatexTextBrowser.h"

using namespace AlgeMate::Latex;

LatexRenderer renderer;
LatexTextBrowser* browser = new LatexTextBrowser;
browser->setLatexRenderer(&renderer);

QString html = renderer.render(latexSource, browser->document());
browser->setHtml(html);
```

## 支持的公式分隔符

| 写法 | 类型 | 效果 |
|------|------|------|
| `$...$` | 行内公式 | 与文字同行 |
| `$$...$$` | 块级公式 | 居中 displaystyle |
| `\[...\]` | 块级公式 | 居中 displaystyle |

## 支持的 LaTeX 命令

数学内容直接交给 JKQTMathText 渲染，涵盖线性代数常用：

- **矩阵**: `\begin{pmatrix}` `{bmatrix}` `{vmatrix}` `{Vmatrix}` `{cases}` `{array}`
- **希腊字母**: `\alpha \beta \gamma \lambda \mu \sigma \theta \phi \omega \eta \xi \pi \rho` 等
- **黑板粗体**: `\mathbb{C R Q Z N F P}`
- **花体/哥特**: `\mathcal` `\mathfrak` `\mathscr`
- **分式/根式**: `\frac \dfrac \sqrt \sqrt[n]`
- **求和/求积**: `\sum \prod \bigcap \bigcup \bigoplus \int \iint`
- **极限**: `\lim \sup \inf \max \min \det \ker \dim \operatorname`
- **箭头**: `\rightarrow \Rightarrow \to \mapsto \implies \iff \leftarrow`
- **大括号**: `\left( \right)` `\left[ \right]` `\left\{ \right\}` `\left\| \right\|`
- **标注**: `\hat \bar \tilde \dot \vec \overline \underline \widehat`
- **点号**: `\cdots \vdots \ddots \ldots`
- **空格**: `\quad \qquad \, \; \!`
- **文本**: `\text{中文}` (需系统有对应字体)
- **上下标**: `_{} ^{} \limits \nolimits`
- **上下堆叠**: `\overset \underset \stackrel \overbrace \underbrace`
- **其他**: `\infty \partial \nabla \forall \exists \emptyset \circ \oplus \otimes \wedge \vee \| \|`

## 自定义数学宏

数学模式内自动展开简写：

```cpp
renderer.addMathMacro("F",  "\\mathbb{F}");   // \F  → \mathbb{F}
renderer.addMathMacro("ch", "\\operatorname{char}");  // \ch → \operatorname{char}
renderer.addMathMacro("rank", "\\operatorname{rank}");
renderer.addMathMacro("span", "\\operatorname{span}");
renderer.addMathMacro("tr", "\\operatorname{tr}");
```

## 自定义文档命令

`\cmd[可选参数]{必选参数}` 形式的命令，注册 handler 返回 HTML：

```cpp
renderer.addCommand("exercise", [](const QString& opt, const QString& arg) {
    return QStringLiteral(
        "<h2 style='color:#4FC3F7; margin:16px 0 8px;'>习题 %1</h2>"
    ).arg(opt);
});

renderer.addCommand("proof", [](const QString&, const QString& arg) {
    return QStringLiteral(
        "<div style='margin:8px 0;'><b>证明</b> %1</div>"
    ).arg(arg);
});
```

## 注释

`%` 开头到行尾的内容被忽略，不输出：

```latex
% 这是一行注释
设 $A$ 为 $n$ 阶方阵.  % 行内注释
```

## 可选配置

```cpp
renderer.setFontSize(18);                  // 数学公式字号, 默认 18pt
renderer.setTextColor(QColor("#333333"));   // 数学公式颜色, 默认 black
```

## LatexTextBrowser

继承 `QTextBrowser`，复制选区时自动将渲染的公式图片还原为 `$...$` LaTeX 源码。用法：用 `LatexTextBrowser` 替代 `QTextBrowser`，并调用 `setLatexRenderer()` 绑定渲染器。

## API 参考

### LatexRenderer

| 方法 | 说明 |
|------|------|
| `render(source, doc)` | 渲染 LaTeX 源码 → HTML |
| `setFontSize(pt)` | 数学公式字号 |
| `setTextColor(c)` | 数学公式颜色 |
| `addMathMacro(cmd, expansion)` | 注册数学宏 |
| `addCommand(name, handler)` | 注册文档命令 |
| `latexForUrl(url)` | 反查图片 URL → LaTeX 源码 |
| `clearCache()` | 清空图片缓存 |

### LatexTextBrowser

| 方法 | 说明 |
|------|------|
| `setLatexRenderer(r)` | 绑定渲染器，启用复制还原 |

## 依赖

- Qt6 (Core, Gui, Widgets)
- JKQTMathText（JKQtPlotter 的数学渲染子库）
