


# 📗 Knowledge（知识点学习模块）


# 知识点学习模块 (Knowledge)

## 职责
提供线性代数完整知识体系学习：
- 章节目录结构
- Markdown 图文讲解
- 例题 + 思考题
- 支持跳转 Practice 模块进行练习

---

## 对外接口
```cpp
AlgeMate::Knowledge::KnowledgePage : public QWidget
```

## 分工建议
### UI结构：
- QSplitter 左右布局
- 左：章节树（QTreeView / QListWidget）
- 右：内容展示（QTextBrowser）
### 数据管理：
- resources/knowledge/*.md
- chapters.json 统一索引
### 渲染方式：
- MVP：QTextBrowser + Markdown
- 进阶：QtWebEngine + KaTeX
- 与 Practice 模块通过 chapter_id 关联

## 第一步
- 构建章节树静态数据
- 实现 Markdown 内容加载
- 添加“进入练习”按钮（跳转 Practice）