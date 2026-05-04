#ifndef ALGEMATE_POLYGCDPAGE_H
#define ALGEMATE_POLYGCDPAGE_H

#include <QWidget>

class QLineEdit;
class QTextBrowser;
class QPushButton;

namespace AlgeMate::Calculator::Demo {

class PolyGCDPage : public QWidget {
    Q_OBJECT
public:
    explicit PolyGCDPage(QWidget* parent = nullptr);

signals:
    void backRequested();

private slots:
    void onSolve();
    void onDemo();

private:
    QLineEdit*    inputF_        = nullptr;
    QLineEdit*    inputG_        = nullptr;
    QTextBrowser* resultBrowser_ = nullptr;
};

}

#endif
