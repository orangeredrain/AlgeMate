#ifndef ALGEMATE_QUADFORMPAGE_H
#define ALGEMATE_QUADFORMPAGE_H

#include <QWidget>
#include <vector>

class QSpinBox;
class QLineEdit;
class QTextBrowser;
class QVBoxLayout;
class QPushButton;

namespace AlgeMate::Calculator::Demo {

class QuadFormPage : public QWidget {
    Q_OBJECT
public:
    explicit QuadFormPage(QWidget* parent = nullptr);

signals:
    void backRequested();

private slots:
    void onGenerate();
    void onSolve();
    void onDemo();

    bool eventFilter(QObject* obj, QEvent* ev) override;

private:
    QSpinBox*     spinN_          = nullptr;
    QWidget*      gridContainer_  = nullptr;
    QVBoxLayout*  gridContainerLay_ = nullptr;
    QTextBrowser* resultBrowser_  = nullptr;
    QPushButton*  solveBtn_       = nullptr;

    int curN_ = 0;
    std::vector<QLineEdit*> cells_;
};

}

#endif
