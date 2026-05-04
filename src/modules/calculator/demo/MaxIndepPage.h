#ifndef ALGEMATE_MAXINDEPPAGE_H
#define ALGEMATE_MAXINDEPPAGE_H

#include <QWidget>
#include <vector>

class QSpinBox;
class QLineEdit;
class QTextBrowser;
class QVBoxLayout;
class QPushButton;

namespace AlgeMate::Calculator::Demo {

class MaxIndepPage : public QWidget {
    Q_OBJECT
public:
    explicit MaxIndepPage(QWidget* parent = nullptr);

signals:
    void backRequested();

private slots:
    void onGenerate();
    void onSolve();
    void onDemo();

    bool eventFilter(QObject* obj, QEvent* ev) override;

private:
    QSpinBox*     spinN_          = nullptr;  // 维数
    QSpinBox*     spinM_          = nullptr;  // 向量个数
    QWidget*      gridContainer_  = nullptr;
    QVBoxLayout*  gridContainerLay_ = nullptr;
    QTextBrowser* resultBrowser_  = nullptr;
    QPushButton*  solveBtn_       = nullptr;

    int curN_ = 0;  // 维数
    int curM_ = 0;  // 向量个数
    std::vector<QLineEdit*> cells_;  // column-major: vector j row i → cells_[j * curN_ + i]
};

}

#endif
