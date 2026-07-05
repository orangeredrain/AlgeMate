#ifndef ALGEMATE_HOMOLINEARSYSTEMPAGE_H
#define ALGEMATE_HOMOLINEARSYSTEMPAGE_H

#include <QWidget>
#include <vector>

class QSpinBox;
class QLineEdit;
class QTextBrowser;
class QVBoxLayout;
class QPushButton;

namespace AlgeMate::Calculator::Demo {

class HomoLinearSystemPage : public QWidget {
    Q_OBJECT
public:
    explicit HomoLinearSystemPage(QWidget* parent = nullptr);

signals:
    void backRequested();

private slots:
    void onGenerate();
    void onSolve();
    void onDemo();

    bool eventFilter(QObject* obj, QEvent* ev) override;

private:
    QSpinBox*    spinM_          = nullptr;
    QSpinBox*    spinN_          = nullptr;
    QWidget*     gridContainer_  = nullptr;
    QVBoxLayout* gridContainerLay_ = nullptr;
    QTextBrowser* resultBrowser_ = nullptr;
    QPushButton*  solveBtn_      = nullptr;

    int curM_ = 0;
    int curN_ = 0;
    std::vector<QLineEdit*> cells_;   
};

}

#endif
