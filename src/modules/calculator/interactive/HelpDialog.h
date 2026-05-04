#ifndef ALGEMATE_CALC_INTERACTIVE_HELPDIALOG_H
#define ALGEMATE_CALC_INTERACTIVE_HELPDIALOG_H

#include <QDialog>

class QTextBrowser;

namespace AlgeMate::Calculator::Interactive {

// 交互式计算助手 · 语法与函数帮助
// 单独的 QDialog 窗口, 列出所有支持的语法 / 内置函数 / 示例
class HelpDialog : public QDialog {
    Q_OBJECT
public:
    explicit HelpDialog(QWidget* parent = nullptr);

private slots:
    void onThemeChanged();

private:
    QTextBrowser* view_ = nullptr;

    void rebuildContent();
};

}

#endif
