# AI 智能解题模块 (AiSolver)

## 职责
接入大模型 API，完成自然语言题目解析、矩阵题目分步作答、思路追问与对话式辅导。

## 对外接口
```cpp
AlgeMate::AiSolver::AiSolverPage : public QWidget
```

## 分工建议
- 新增 `network/ApiClient.{h,cpp}` 封装 `QNetworkAccessManager`，API Key 读取自 QSettings。
- 需要网络模块时在本目录 CMakeLists 追加 `Qt6::Network`。
- 消息流式渲染可用 `QTextBrowser` + Markdown 或自绘消息气泡。

## 第一步
1. 先做同步非流式调用跑通链路。
2. 再做流式（`QNetworkReply::readyRead`）渲染打字机效果。
