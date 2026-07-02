#ifndef ALGEMATE_LATEX_TEXT_BROWSER_H
#define ALGEMATE_LATEX_TEXT_BROWSER_H

#include <QString>
#include <QTextBrowser>

namespace AlgeMate::Latex {

class LatexTextBrowser : public QTextBrowser {
    Q_OBJECT
public:
    explicit LatexTextBrowser(QWidget* parent = nullptr);

    void setSourceMarkdown(const QString& markdown);

protected:
    QMimeData* createMimeDataFromSelection() const override;

private:
    QString m_sourceMarkdown;
};

} 

#endif 
