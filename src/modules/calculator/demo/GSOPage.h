#ifndef ALGEMATE_GSOPAGE_H
#define ALGEMATE_GSOPAGE_H

#include <QWidget>
#include <vector>

class QSpinBox;
class QLineEdit;
class QTextBrowser;
class QVBoxLayout;
class QPushButton;

namespace AlgeMate::Calculator::Demo {

class GSOPage : public QWidget {
    Q_OBJECT
public:
    explicit GSOPage(QWidget* parent = nullptr);

signals:
    void backRequested();

private slots:
    void onGenerate();
    void onSolve();
    void onDemo();

    bool eventFilter(QObject* obj, QEvent* ev) override;

private:
    QSpinBox*     spinN_          = nullptr;
    QSpinBox*     spinM_          = nullptr;
    QWidget*      gridContainer_  = nullptr;
    QVBoxLayout*  gridContainerLay_ = nullptr;
    QTextBrowser* resultBrowser_  = nullptr;
    QPushButton*  solveBtn_       = nullptr;

    int curN_ = 0;
    int curM_ = 0;
    std::vector<QLineEdit*> cells_;  
};

}

#endif
