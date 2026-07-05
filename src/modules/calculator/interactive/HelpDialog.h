#ifndef ALGEMATE_CALC_INTERACTIVE_HELPDIALOG_H
#define ALGEMATE_CALC_INTERACTIVE_HELPDIALOG_H

#include <QDialog>

class QTextBrowser;

namespace AlgeMate::Calculator::Interactive {

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
