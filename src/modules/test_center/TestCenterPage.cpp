#include "TestCenterPage.h"

#include "latex/LatexRenderer.h"
#include "latex/LatexTextBrowser.h"

#include <QVBoxLayout>
#include <QSplitter>
#include <QLabel>
#include <QTextEdit>

namespace AlgeMate::TestCenter {

TestCenterPage::TestCenterPage(QWidget* parent) : QWidget(parent)
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(24, 24, 24, 24);
    root->setSpacing(12);

    auto* title = new QLabel(QStringLiteral("测试中心"));
    title->setStyleSheet("font-size:22px; font-weight:700; color:#E0E0E0;");

    auto* hint = new QLabel(QStringLiteral("上方输入 LaTeX, 下方实时渲染. 支持 $...$ / $$...$$ / \\[...\\] / 矩阵 / 自定义宏."));
    hint->setStyleSheet("font-size:12px; color:#8A8FA3;");

    // --- 拆分窗: 上输入 / 下输出 ---
    m_splitter = new QSplitter(Qt::Vertical);

    m_input = new QTextEdit;
    m_input->setPlaceholderText(QStringLiteral("在此输入 LaTeX 代码..."));
    m_input->setStyleSheet(QStringLiteral(
        "QTextEdit { font-family:'Cascadia Mono','Consolas','Courier New',monospace; "
        "font-size:13pt; border:1px solid #ccc; border-radius:6px; "
        "padding:8px; background:#FFFFFF; color:#1E1E1E; }"));
    m_input->setMinimumHeight(120);

    m_browser = new Latex::LatexTextBrowser;
    m_browser->setOpenLinks(false);
    m_browser->setStyleSheet(QStringLiteral(
        "QTextBrowser { border:1px solid #ccc; border-radius:6px; "
        "padding:16px; background:#FFFFFF; }"));

    m_splitter->addWidget(m_input);
    m_splitter->addWidget(m_browser);
    m_splitter->setStretchFactor(0, 2); // 输入占 2
    m_splitter->setStretchFactor(1, 3); // 输出占 3

    // --- 渲染器配置 ---
    m_renderer = new Latex::LatexRenderer;
    m_renderer->setTextColor(QColor(Qt::black));
    m_renderer->addMathMacro(QStringLiteral("F"),  QStringLiteral("\\mathbb{F}"));
    m_renderer->addMathMacro(QStringLiteral("ch"), QStringLiteral("\\operatorname{char}"));
    m_renderer->addCommand(QStringLiteral("exercise"),
        [](const QString& opt, const QString&) {
            return QStringLiteral(
                "<h2 style='color:#4FC3F7; margin:16px 0 8px;'>习题 %1</h2>"
            ).arg(opt);
        });

    // 默认示例内容
    m_input->setPlainText(QStringLiteral(
        "% 线性代数测试\n"
        "\\exercise[对偶空间与子空间覆盖]\n"
        "设 $V$ 是域 $\\F$ ($\\ch \\F = 0$) 上的线性空间, "
        "$\\alpha_1, \\alpha_2, \\cdots, \\alpha_n, "
        "\\beta_1, \\beta_2, \\cdots, \\beta_n\\in V$. "
        "若对任意 $f\\in V^{*}$, 有\n"
        "\\[\n"
        "(f(\\alpha_1),f(\\alpha_2),\\cdots,f(\\alpha_n)) "
        "\\quad\\text{与}\\quad "
        "(f(\\beta_1),f(\\beta_2),\\cdots,f(\\beta_n))\n"
        "\\]\n"
        "在不计分量顺序意义下相同.\n"
        "\n"
        "--- 矩阵与范数测试 ---\n"
        "设 $A\\in \\mathbb{C}^{n\\times n}$, Frobenius 范数:\n"
        "$$\n"
        "\\| A \\|_F = \\sqrt{\\sum\\limits_{i=1}^{m}"
        "\\sum\\limits_{j=1}^{n}|a_{ij}|^2}\n"
        "$$\n"
        "矩阵 $\\begin{pmatrix} a & b \\\\ c & d \\end{pmatrix}$ "
        "的行列式为 $\\det\\begin{pmatrix} a & b \\\\ c & d \\end{pmatrix}"
        " = ad - bc$.\n"
    ));

    // 实时渲染
    connect(m_input, &QTextEdit::textChanged, this, &TestCenterPage::onInputChanged);

    root->addWidget(title);
    root->addWidget(hint);
    root->addWidget(m_splitter, 1);

    // 初始渲染
    onInputChanged();
}

TestCenterPage::~TestCenterPage()
{
    delete m_renderer;
}

void TestCenterPage::onInputChanged()
{
    m_renderer->clearCache();
    QString html = m_renderer->render(m_input->toPlainText(), m_browser->document());
    m_browser->setHtml(html);
}

} // namespace AlgeMate::TestCenter
