#pragma once

#include <QDialog>
#include "TomatoManager.h" // 引入番茄钟控制器

class QLabel;
class QPushButton;

namespace AlgeMate {

class TomatoClockDialog : public QDialog {
    Q_OBJECT

public:
    explicit TomatoClockDialog(QWidget* parent = nullptr);
    ~TomatoClockDialog() = default;

private:
    void buildUi();
    void initConnections();
    void updateStyleByStatus(TomatoManager::State state);

private:
    QLabel* labelStatus_ = nullptr;  // 显示当前状态（专注中/休息中）
    QLabel* labelTime_ = nullptr;    // 显示倒计时文本（如 24:59）
    QPushButton* btnPauseResume_ = nullptr; // 暂停/恢复按钮
    QPushButton* btnSkip_ = nullptr;        // 跳过/重置按钮
};

} // namespace AlgeMate