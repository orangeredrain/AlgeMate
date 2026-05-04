#include "HelpDialog.h"

#include "expr/RenderSettings.h"
#include "expr/Value.h"
#include "LatexTextBrowser.h"
#include "core/ThemeManager.h"

#include <QPalette>
#include <QPushButton>
#include <QTextBrowser>
#include <QTextDocument>
#include <QVBoxLayout>
#include <QHBoxLayout>

namespace AlgeMate::Calculator::Interactive {

namespace {

struct FuncDoc {
    QString sig;      // 签名
    QString desc;     // 说明
    QString example;  // 示例
};

struct Section {
    QString title;
    QList<FuncDoc> items;
};

QList<Section> allSections() {
    return {
        Section{
            QStringLiteral("语法基础"),
            {
                {QStringLiteral("name = expr"),
                 QStringLiteral("赋值, 变量名以字母开头, 允许 a-z A-Z 0-9 _"),
                 QStringLiteral("a = 1+2")},
                {QStringLiteral("ans"),
                 QStringLiteral("上一次求值的结果 (自动维护)"),
                 QStringLiteral("ans*2")},
                {QStringLiteral("+  -  *  /  ^"),
                 QStringLiteral("加减乘除与幂, 幂次仅限整数"),
                 QStringLiteral("2^10")},
                {QStringLiteral("-expr"),
                 QStringLiteral("一元取负, 矩阵也可"),
                 QStringLiteral("-m")},
                {QStringLiteral("M'"),
                 QStringLiteral("矩阵转置 (单引号后缀)"),
                 QStringLiteral("[1,2; 3,4]'")},
                {QStringLiteral("1/2, 3/7"),
                 QStringLiteral("可进行精确分数运算"),
                 QStringLiteral("1/2 + 1/3")},
            },
        },
        Section{
            QStringLiteral("矩阵字面量"),
            {
                {QStringLiteral("[a, b, c; d, e, f]"),
                 QStringLiteral("逗号分隔列, 分号分隔行"),
                 QStringLiteral("[1, 2, 3; 4, 5, 6]")},
                {QStringLiteral("[a b c; d e f]"),
                 QStringLiteral("空格也可分隔列"),
                 QStringLiteral("[1 2 3; 4 5 6; 7 8 9]")},
            },
        },
        Section{
            QStringLiteral("数值 / 代数数函数"),
            {
                {QStringLiteral("sqrt(x)"),
                 QStringLiteral("平方根"),
                 QStringLiteral("sqrt(2)")},
                {QStringLiteral("root(n, x)"),
                 QStringLiteral("$\\sqrt[n]{x}$, n 为大于 1 的正整数"),
                 QStringLiteral("root(3, 5)")},
                {QStringLiteral("abs(x)"),
                 QStringLiteral("绝对值"),
                 QStringLiteral("abs(-3/7)")},
            },
        },
        Section{
            QStringLiteral("复数"),
            {
                {QStringLiteral("i"),
                 QStringLiteral("虚数单位, $i^{2} = -1$"),
                 QStringLiteral("1 + 2i")},
                {QStringLiteral("a + b i"),
                 QStringLiteral("复数字面量: 实部 / 虚部可为任意整数、分数或小数"),
                 QStringLiteral("1/2 + 3/4 i")},
                {QStringLiteral("re(z)"),
                 QStringLiteral("复数的实部 $\\mathrm{Re}(z)$"),
                 QStringLiteral("re(3 + 4i)")},
                {QStringLiteral("im(z)"),
                 QStringLiteral("复数的虚部 $\\mathrm{Im}(z)$"),
                 QStringLiteral("im(3 + 4i)")},
                {QStringLiteral("conj(z)"),
                 QStringLiteral("复共轭 $\\bar{z}$"),
                 QStringLiteral("conj(3 + 4i)")},
                {QStringLiteral("abs(z)"),
                 QStringLiteral("复数的模 $|z| = \\sqrt{a^{2} + b^{2}}$"),
                 QStringLiteral("abs(3 + 4i)")},
                {QStringLiteral("arg(z)"),
                 QStringLiteral("辐角主值 $\\arg(z) \\in (-\\pi, \\pi]$, 以弧度表示"),
                 QStringLiteral("arg(1 + i)")},
                {QStringLiteral("sqrt(-x)"),
                 QStringLiteral("负数开方自动进入复数域"),
                 QStringLiteral("sqrt(-2)")},
            },
        },
        Section{
            QStringLiteral("矩阵构造"),
            {
                {QStringLiteral("Identity(n) / identity(n)"),
                 QStringLiteral("n × n 单位矩阵"),
                 QStringLiteral("Identity(3)")},
                {QStringLiteral("zeros(m, n)"),
                 QStringLiteral("m × n 全零矩阵"),
                 QStringLiteral("zeros(2, 3)")},
                {QStringLiteral("ones(m, n)"),
                 QStringLiteral("m × n 全一矩阵"),
                 QStringLiteral("ones(2, 2)")},
            },
        },
        Section{
            QStringLiteral("矩阵查询与变换"),
            {
                {QStringLiteral("tr(M)"),
                 QStringLiteral("方阵的迹"),
                 QStringLiteral("tr([1,2;3,4])")},
                {QStringLiteral("transpose(M)"),
                 QStringLiteral("转置, 等价于 M'"),
                 QStringLiteral("transpose(m)")},
                {QStringLiteral("rref(M)"),
                 QStringLiteral("简化行阶梯形矩阵"),
                 QStringLiteral("rref([1,2,3;4,5,6])")},
            },
        },
        Section{
            QStringLiteral("行列式 / 秩 / 逆"),
            {
                {QStringLiteral("det(M)"),
                 QStringLiteral("行列式"),
                 QStringLiteral("det([1,3;1,4])")},
                {QStringLiteral("rank(M)"),
                 QStringLiteral("矩阵的秩"),
                 QStringLiteral("rank([1,2;2,4])")},
                {QStringLiteral("inv(M)"),
                 QStringLiteral("矩阵逆 (需可逆方阵)"),
                 QStringLiteral("inv([1,2;3,5])")},
            },
        },
        Section{
            QStringLiteral("线性方程"),
            {
                {QStringLiteral("solve(A, b)"),
                 QStringLiteral("解 $Ax=b$; 判断唯一解 / 无穷多解 / 无解"),
                 QStringLiteral("solve([1,2;3,4], [5;11])")},
                {QStringLiteral("nullspace(M)"),
                 QStringLiteral("零空间的基. 若为平凡零空间, 则返回\"无\""),
                 QStringLiteral("nullspace([1,2;2,4])")},
            },
        },
        Section{
            QStringLiteral("特征结构"),
            {
                {QStringLiteral("charpoly(M)"),
                 QStringLiteral("特征多项式 $\\det{(\\lambda I - M)}$"),
                 QStringLiteral("charpoly([0,1;1,1])")},
                {QStringLiteral("eigs(M) / eigenvalues(M)"),
                 QStringLiteral("实特征值. 没有则返回\"无\""),
                 QStringLiteral("eigs([0,-1;1,0])")},
                {QStringLiteral("ceigs(M) / complexeigs(M)"),
                 QStringLiteral("复特征值"),
                 QStringLiteral("ceigs([0,-1;1,0])")},
            },
        },
        Section{
            QStringLiteral("二次型 / 对称矩阵"),
            {
                {QStringLiteral("issym(A)"),
                 QStringLiteral("判断 $A$ 是否对称 ($A = A^{T}$)"),
                 QStringLiteral("issym([1,2;2,3])")},
                {QStringLiteral("signature(A)"),
                 QStringLiteral("对称矩阵的惯性指数 $(p^+, p^-, p^0)$ (正/负/零惯性指数)"),
                 QStringLiteral("signature([2,-1;-1,2])")},
                {QStringLiteral("definiteness(A)"),
                 QStringLiteral("定性分类: 正定 / 半正定 / 负定 / 半负定 / 不定 / 零型"),
                 QStringLiteral("definiteness([2,-1;-1,2])")},
                {QStringLiteral("congdiag(A)"),
                 QStringLiteral("合同对角化: 返回 $D, P$ 使 $P^{T} A P = D$"),
                 QStringLiteral("congdiag([2,-1;-1,2])")},
            },
        },
        Section{
            QStringLiteral("标准形"),
            {
                {QStringLiteral("jordan(A)"),
                 QStringLiteral("Jordan 标准形 $J$ 与相似矩阵 $Q$ ($Q^{-1} A Q = J$); 同时给出行列式因子 / 不变因子 / 初等因子"),
                 QStringLiteral("jordan([3,-1;1,1])")},
                {QStringLiteral("rcf(A) / frobenius(A)"),
                 QStringLiteral("有理标准形 $F$ = $\\mathrm{diag}(C(d_1), \\ldots, C(d_r))$; 同时给出特征多项式的 $\\mathbb{Q}$ 不可约分解 + 不变因子"),
                 QStringLiteral("rcf([2,1;-1,0])")},
            },
        },
        Section{
            QStringLiteral("多项式"),
            {
                {QStringLiteral("g(x) = expr"),
                 QStringLiteral("自定义多项式: 把 $expr$ 存为变量 $g$, 形参名 (如 $x$) 由自由变量自动识别"),
                 QStringLiteral("g(x) = x^2 + 2x + 1")},
                {QStringLiteral("g(a)"),
                 QStringLiteral("以数值代入求值"),
                 QStringLiteral("g(3)")},
                {QStringLiteral("g(x+1)"),
                 QStringLiteral("以表达式代入, 得到新多项式 (自动展开, 支持换元)"),
                 QStringLiteral("g(2x+1)")},
                {QStringLiteral("g(h) / g(g)"),
                 QStringLiteral("以另一多项式复合代入 (支持与自身复合)"),
                 QStringLiteral("g(g)")},
                {QStringLiteral("gcd(p, q) / polygcd(p, q)"),
                 QStringLiteral("两个多项式的最大公因式"),
                 QStringLiteral("gcd(charpoly(A), charpoly(B))")},
                {QStringLiteral("factor(p)"),
                 QStringLiteral("$\\mathbb{Q}[x]$ 上的完全分解"),
                 QStringLiteral("factor(charpoly([0,1,0;0,0,1;6,-11,6]))")},
                {QStringLiteral("rfactor(p)"),
                 QStringLiteral("$\\mathbb{R}[x]$ 上的分解 (一次因式 + 二次不可约因式)"),
                 QStringLiteral("rfactor(x^3 - 2x^2 + x - 2)")},
                {QStringLiteral("res(f, g) / resultant(f, g)"),
                 QStringLiteral("结式 $\\mathrm{Res}(f, g)$"),
                 QStringLiteral("resultant(charpoly(A), charpoly(B))")},
                {QStringLiteral("discriminant(f)"),
                 QStringLiteral("判别式 $\\Delta(f)$, $\\Delta = 0$ 即有重根"),
                 QStringLiteral("discriminant(charpoly(A))")},
                {QStringLiteral("rroots(p) / rationalroots(p)"),
                 QStringLiteral("所有有理根"),
                 QStringLiteral("rroots(charpoly([1,0;0,2]))")},
                {QStringLiteral("squarefree(f) / sqfree(f)"),
                 QStringLiteral("无平方部分 $f / \\gcd(f, f')$ (去重根, 首一化)"),
                 QStringLiteral("squarefree((x-1)^2 (x+2))")},
                {QStringLiteral("minpoly(a)"),
                 QStringLiteral("代数数 $a$ 在 $\\mathbb{Q}$ 上的最小多项式"),
                 QStringLiteral("minpoly(sqrt(2)+sqrt(3))")},
                {QStringLiteral("minpoly(M)"),
                 QStringLiteral("方阵 $M$ 的最小多项式 (= 最大不变因子)"),
                 QStringLiteral("minpoly([[0,1],[1,0]])")},
                {QStringLiteral("irred(f) / irreducible(f)"),
                 QStringLiteral("判断 $f$ 在 $\\mathbb{Q}[x]$ 中是否不可约"),
                 QStringLiteral("irred(x^2 + 1)")},
                {QStringLiteral("irred(f, p) / irreducible(f, p)"),
                 QStringLiteral("判断 $f$ 在 $\\mathbb{Z}_{p}[x]$ 中是否不可约 ($p$ 为素数)"),
                 QStringLiteral("irred(x^2 + 1, 2)")},
                {QStringLiteral("roots(p)"),
                 QStringLiteral("求多项式的所有复根"),
                 QStringLiteral("roots(x^3 - 2x^2 + x - 2)")},
                {QStringLiteral("irredcnt(n, q)"),
                 QStringLiteral("有限域 $\\mathbb{F}_q$ 上所有 $n$ 次不可约多项式的个数"),
                 QStringLiteral("irredcnt(3, 2)")},
                {QStringLiteral("powerSumToSym(k, n)"),
                 QStringLiteral("幂和 $s_k = \\sum_{j=1}^{n} x_j^k$ 表示为初等对称多项式 $\\sigma_i$ 的多项式"),
                 QStringLiteral("powerSumToSym(3, 2)")},
                {QStringLiteral("symToPowerSum(k, n)"),
                 QStringLiteral("初等对称多项式 $\\sigma_k$ 表示为幂和 $s_i = \\sum_{j=1}^{n} x_j^i$ 的多项式 ($k \\le n$)"),
                 QStringLiteral("symToPowerSum(3, 3)")},
            },
        },
        Section{
            QStringLiteral("矩阵分解"),
            {
                {QStringLiteral("lu(A)"),
                 QStringLiteral("Doolittle $LU$ 分解: $A = L U$, $L$ 单位下三角, $U$ 上三角 (不做行交换)"),
                 QStringLiteral("lu([4,3;6,3])")},
                {QStringLiteral("qr(A)"),
                 QStringLiteral("$QR$ 分解: $A = Q R$, $Q$ 列正交单位, $R$ 上三角"),
                 QStringLiteral("qr([1,1;1,-1;1,0])")},
                {QStringLiteral("svd(A)"),
                 QStringLiteral("奇异值分解: $A = U \\Sigma V^{T}$ ($U$ 为 $m\\times m$ 正交矩阵; $\\Sigma$ 为 $m\\times n$ 对角矩阵, 奇异值降序; $V$ 为 $n\\times n$ 正交矩阵)"),
                 QStringLiteral("svd([1,1;1,1])")},
                {QStringLiteral("gramschmidt(V) / gso(V)"),
                 QStringLiteral("列向量组的 Gram-Schmidt 正交单位化 (线性相关列自动丢弃)"),
                 QStringLiteral("gramschmidt([1,1;1,0;0,1])")},
            },
        },
        Section{
            QStringLiteral("结果显示说明"),
            {
                {QStringLiteral("精确 / 数值"),
                 QStringLiteral("右下角切换: 精确模式保留分数等符号 (有时会转换为小数); 数值模式是浮点数运算"),
                 QStringLiteral("不同模式输出格式不同")},
                {QStringLiteral("位数输入框"),
                 QStringLiteral("数值模式下可手动输入或滚轮调节小数位数 (0 – 15)"),
                 QStringLiteral("光标移到输入框上滚动滚轮")},
            },
        },
    };
}

QString buildHtml(const RenderTheme& th, QTextDocument* doc) {
    QString html;
    html += QStringLiteral(
        "<div style=\"padding:4px 6px 12px 6px;\">"
        "<div style=\"color:%1; font-size:18px; font-weight:700; margin-bottom:4px;\">"
        "语法与函数速查</div>"
        "<div style=\"color:%2; font-size:12px; margin-bottom:14px;\">"
        "注意函数名对大小写不敏感</div>")
        .arg(th.accent, th.textMuted);

    for (const auto& sec : allSections()) {
        html += QStringLiteral(
            "<div style=\"color:%1; font-size:14px; font-weight:700; "
            "margin:10px 0 6px 0;\">%2</div>")
            .arg(th.accentSoft, sec.title.toHtmlEscaped());

        html += QStringLiteral(
            "<table cellspacing=\"0\" cellpadding=\"0\" "
            "style=\"border-collapse:collapse; width:100%; margin-bottom:6px;\">");
        for (const auto& it : sec.items) {
            // 说明列支持 $...$ 内联 LaTeX; 签名/示例列保持 code 原样.
            const QString descHtml = renderNoteWithLatex(it.desc, th, doc, 13);
            html += QStringLiteral(
                "<tr>"
                "<td style=\"padding:7px 10px; color:%1; "
                "font-family:'Cascadia Mono','Consolas',monospace; "
                "font-size:12px; width:36%; vertical-align:top;\">%2</td>"
                "<td style=\"padding:7px 10px; color:%3; font-size:12px; vertical-align:top;\">%4</td>"
                "<td style=\"padding:7px 10px; color:%5; "
                "font-family:'Cascadia Mono','Consolas',monospace; "
                "font-size:12px; vertical-align:top;\">%6</td>"
                "</tr>")
                .arg(th.accentSoft, it.sig.toHtmlEscaped(),
                     th.text,       descHtml,
                     th.textSoft,   it.example.toHtmlEscaped());
        }
        html += QStringLiteral("</table>");
    }

    html += QStringLiteral("</div>");
    return html;
}

}

HelpDialog::HelpDialog(QWidget* parent) : QDialog(parent) {
    setObjectName(QStringLiteral("CalcHelpDialog"));
    setWindowTitle(QStringLiteral("计算助手 · 帮助"));
    resize(760, 620);
    setModal(false);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(14, 12, 14, 12);
    root->setSpacing(10);

    view_ = new LatexTextBrowser;
    view_->setObjectName(QStringLiteral("CalcHelpView"));
    view_->setOpenExternalLinks(false);
    view_->setFrameShape(QFrame::NoFrame);
    QFont f(QStringLiteral("Microsoft YaHei UI"));
    f.setPointSize(10);
    view_->setFont(f);

    auto* btnRow = new QHBoxLayout;
    btnRow->addStretch(1);
    auto* closeBtn = new QPushButton(QStringLiteral("关闭"));
    closeBtn->setProperty("primary", true);
    closeBtn->setCursor(Qt::PointingHandCursor);
    btnRow->addWidget(closeBtn);

    root->addWidget(view_, 1);
    root->addLayout(btnRow);

    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    connect(&AlgeMate::ThemeManager::instance(),
            &AlgeMate::ThemeManager::themeChanged,
            this, [this](AlgeMate::ThemeManager::Theme){ onThemeChanged(); });

    rebuildContent();
}

void HelpDialog::onThemeChanged() { rebuildContent(); }

void HelpDialog::rebuildContent() {
    const RenderTheme th = RenderTheme::forCurrent();
    QPalette p = view_->palette();
    p.setColor(QPalette::Base, QColor(th.bgHistory));
    p.setColor(QPalette::Text, QColor(th.text));
    view_->setPalette(p);
    // 传入 document 以供 LaTeX 片段注册 QPixmap 资源.
    view_->setHtml(buildHtml(th, view_->document()));
}

}
