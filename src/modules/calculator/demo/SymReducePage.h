#ifndef ALGEMATE_SYMREDUCEPAGE_H
#define ALGEMATE_SYMREDUCEPAGE_H

#include <QWidget>

class QLabel;
class QSpinBox;
class QComboBox;
class QLineEdit;
class QTextBrowser;
class QPushButton;

namespace AlgeMate::Calculator::Demo {

class SymReducePage : public QWidget {
    Q_OBJECT
public:
    explicit SymReducePage(QWidget* parent = nullptr);

signals:
    void backRequested();

private slots:
    void onSolve();
    void onDemo();

private:
    QLabel*       spinNLabel_    = nullptr;
    QSpinBox*     spinN_         = nullptr;
    QComboBox*    modeCombo_     = nullptr;
    QLineEdit*    inputPoly_     = nullptr;
    QTextBrowser* resultBrowser_ = nullptr;
};

}

#endif
