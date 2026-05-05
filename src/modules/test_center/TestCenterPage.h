#ifndef ALGEMATE_TEST_CENTER_PAGE_H
#define ALGEMATE_TEST_CENTER_PAGE_H

#include <QWidget>

class QTextEdit;
class QSplitter;

namespace AlgeMate::Latex {
class LatexRenderer;
class LatexTextBrowser;
}

namespace AlgeMate::TestCenter {

class TestCenterPage : public QWidget {
    Q_OBJECT
public:
    explicit TestCenterPage(QWidget* parent = nullptr);
    ~TestCenterPage() override;

private slots:
    void onInputChanged();

private:
    Latex::LatexRenderer*    m_renderer = nullptr;
    Latex::LatexTextBrowser* m_browser  = nullptr;
    QTextEdit*               m_input    = nullptr;
    QSplitter*               m_splitter = nullptr;
};

} // namespace AlgeMate::TestCenter

#endif // ALGEMATE_TEST_CENTER_PAGE_H
