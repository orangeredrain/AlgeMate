#include "TomatoClockDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

namespace AlgeMate {

TomatoClockDialog::TomatoClockDialog(QWidget* parent) : QDialog(parent) {
    // 设置窗口属性
    setWindowTitle(QStringLiteral("番茄钟"));
    setFixedSize(300, 220);
    // 移除自带的帮助问号，保持窗口简洁
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    buildUi();
    initConnections();

    // 刚打开时，根据控制器的当前状态初始化界面样式
    updateStyleByStatus(TomatoManager::instance().currentState());
    labelTime_->setText(TomatoManager::instance().remainingTimeStr());
}

void TomatoClockDialog::buildUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 24, 24, 24);
    mainLayout->setSpacing(16);

    // 1. 状态标签
    labelStatus_ = new QLabel(QStringLiteral("专注中..."), this);
    labelStatus_->setAlignment(Qt::AlignCenter);
    labelStatus_->setStyleSheet("font-size: 16px; font-weight: bold; color: #333333;");

    // 2. 时间倒计时标签
    labelTime_ = new QLabel(QStringLiteral("25:00"), this);
    labelTime_->setAlignment(Qt::AlignCenter);
    labelTime_->setStyleSheet("font-size: 48px; font-weight: 700; font-family: 'Courier New', monospace; color: #E05A47;");

    // 3. 控制按钮布局
    auto* btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(12);

    btnPauseResume_ = new QPushButton(QStringLiteral("暂停"), this);
    btnPauseResume_->setCursor(Qt::PointingHandCursor);
    btnPauseResume_->setStyleSheet(
        "QPushButton { background-color: #F0F0F0; border: none; padding: 8px 20px; border-radius: 6px; font-size: 13px; }"
        "QPushButton:hover { background-color: #E5E5E5; }");

    btnSkip_ = new QPushButton(QStringLiteral("放弃"), this);
    btnSkip_->setCursor(Qt::PointingHandCursor);
    btnSkip_->setStyleSheet(
        "QPushButton { background-color: #FFEBE9; border: none; padding: 8px 20px; border-radius: 6px; font-size: 13px; color: #E05A47; }"
        "QPushButton:hover { background-color: #FFDCD9; }");

    btnLayout->addWidget(btnPauseResume_);
    btnLayout->addWidget(btnSkip_);

    mainLayout->addWidget(labelStatus_);
    mainLayout->addWidget(labelTime_);
    mainLayout->addStretch();
    mainLayout->addLayout(btnLayout);
}

void TomatoClockDialog::initConnections() {
    auto& manager = TomatoManager::instance();

    // 核心：监听控制器的 tick 信号，每秒更新时间文本
    connect(&manager, &TomatoManager::tick, this, [this](int seconds, const QString& timeStr) {
        labelTime_->setText(timeStr);
    });

    // 监听状态改变信号，动态调整界面文字和颜色
    connect(&manager, &TomatoManager::statusChanged, this, [this](TomatoManager::State state) {
        updateStyleByStatus(state);
    });

    // 暂停 / 恢复 按钮点击事件
    connect(btnPauseResume_, &QPushButton::clicked, this, [this, &manager]() {
        if (btnPauseResume_->text() == QStringLiteral("暂停")) {
            manager.pause();
            btnPauseResume_->setText(QStringLiteral("继续"));
        } else {
            manager.resume();
            btnPauseResume_->setText(QStringLiteral("暂停"));
        }
    });

    // 放弃 / 重置 按钮点击事件
    connect(btnSkip_, &QPushButton::clicked, this, [this, &manager]() {
        manager.reset();
        accept(); // 关闭弹窗
    });

    // 如果番茄钟自然结束（倒计时到0），自动关闭或提示
    connect(&manager, &TomatoManager::finished, this, [this]() {
        // 这里可以加一个播放提示音或者弹窗通知逻辑
    });
}

void TomatoClockDialog::updateStyleByStatus(TomatoManager::State state) {
    switch (state) {
    case TomatoManager::State::Focus:
        labelStatus_->setText(QStringLiteral("🎯 专注中，保持高效"));
        labelTime_->setStyleSheet("font-size: 48px; font-weight: 700; color: #E05A47;"); // 专注用红色
        btnPauseResume_->setText(QStringLiteral("暂停"));
        break;
    case TomatoManager::State::ShortBreak:
    case TomatoManager::State::LongBreak:
        labelStatus_->setText(QStringLiteral("☕ 休息中，放松一下"));
        labelTime_->setStyleSheet("font-size: 48px; font-weight: 700; color: #2BA245;"); // 休息用绿色
        btnPauseResume_->setText(QStringLiteral("暂停"));
        break;
    case TomatoManager::State::Idle:
        labelStatus_->setText(QStringLiteral("番茄钟未开始"));
        labelTime_->setText(QStringLiteral("00:00"));
        break;
    }
}

} // namespace AlgeMate