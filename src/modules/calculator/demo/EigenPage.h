#ifndef ALGEMATE_EIGENPAGE_H
#define ALGEMATE_EIGENPAGE_H

#include <QWidget>
#include <vector>

class QSpinBox;
class QComboBox;
class QLineEdit;
class QTextBrowser;
class QVBoxLayout;
class QPushButton;

namespace AlgeMate::Calculator::Demo {

class EigenPage : public QWidget {
    Q_OBJECT
public:
    explicit EigenPage(QWidget* parent = nullptr);

signals:
    void backRequested();

private slots:
    void onGenerate();
    void onSolve();
    void onDemo();

    bool eventFilter(QObject* obj, QEvent* ev) override;

private:
    QSpinBox*     spinN_          = nullptr;
    QComboBox*    domainCombo_    = nullptr;
    QWidget*      gridContainer_  = nullptr;
    QVBoxLayout*  gridContainerLay_ = nullptr;
    QTextBrowser* resultBrowser_  = nullptr;
    QPushButton*  solveBtn_       = nullptr;

    int curN_ = 0;
    std::vector<QLineEdit*> cells_;  // row-major: cells_[i * curN_ + j]
};

}

#endif
