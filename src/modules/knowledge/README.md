# 知识点学习模块 (Knowledge)

## 职责
线性代数章节目录 + 图文讲解（Markdown/HTML）+ 例题 + 思考题。

## 对外接口
```cpp
AlgeMate::Knowledge::KnowledgePage : public QWidget
```

## 分工建议
- 用 `QSplitter` 分左右：左侧章节树（`QTreeView` / `QListWidget`），右侧内容（`QTextBrowser`）。
- 讲解内容推荐 Markdown 文件放 `resources/knowledge/*.md`，运行时动态加载。
- 公式渲染：简单方案用 LaTeX 图片，复杂方案后期可接 KaTeX via QWebEngineView（需新增 `Qt6::WebEngineWidgets`）。

## 第一步
1. 搭左侧章节树静态数据。
2. 右侧 QTextBrowser 显示选中章节的 Markdown。
