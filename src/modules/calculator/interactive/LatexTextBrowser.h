
#ifndef ALGEMATE_CALC_LATEX_TEXT_BROWSER_H
#define ALGEMATE_CALC_LATEX_TEXT_BROWSER_H

#include <QTextBrowser>

class QMimeData;

namespace AlgeMate::Calculator::Interactive {

class LatexTextBrowser : public QTextBrowser {
    Q_OBJECT
public:
    using QTextBrowser::QTextBrowser;

protected:

    QMimeData* createMimeDataFromSelection() const override;
};

} 

#endif
