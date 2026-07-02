#include "SymReducePage.h"
#include "DemoCommon.h"

#include "math/mpoly/SymmetricReduction.h"
#include "modules/calculator/interactive/expr/RenderSettings.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QSpinBox>
#include <QTextBrowser>
#include <QVBoxLayout>

#include <sstream>
#include <string>

using AlgeMate::Calculator::Interactive::RenderTheme;

namespace AlgeMate::Calculator::Demo {

SymReducePage::SymReducePage(QWidget* parent) : QWidget(parent) {
    auto* root = new QVBoxLayout(this); root->setContentsMargins(0,0,0,0); root->setSpacing(0);
    auto* topBar = new QWidget; topBar->setFixedHeight(48);
    auto* topLay = new QHBoxLayout(topBar); topLay->setContentsMargins(12,0,12,0);
    auto* backBtn = new QPushButton(QStringLiteral("← 返回"));
    backBtn->setFlat(true); backBtn->setCursor(Qt::PointingHandCursor);
    backBtn->setStyleSheet(QStringLiteral("QPushButton{font-size:14px;color:#8A8FA3;}QPushButton:hover{color:#C0C4D6;}"));
    connect(backBtn, &QPushButton::clicked, this, &SymReducePage::backRequested);
    topLay->addWidget(backBtn);
    auto* tl = new QLabel(QStringLiteral("对称多项式用初等对称多项式表出")); tl->setStyleSheet(QStringLiteral("font-size:18px;font-weight:700;"));
    topLay->addWidget(tl); topLay->addStretch(1); root->addWidget(topBar);

    auto* scroll = new QScrollArea; scroll->setWidgetResizable(true); scroll->setFrameShape(QFrame::NoFrame);
    auto* content = new QWidget;
    auto* cLay = new QVBoxLayout(content); cLay->setContentsMargins(24,16,24,24); cLay->setSpacing(16);

    auto* inW = new QWidget;
    auto* iLay = new QHBoxLayout(inW); iLay->setSpacing(10);
    spinNLabel_ = new QLabel(QStringLiteral("变量个数 n ="));
    iLay->addWidget(spinNLabel_);
    spinN_ = new QSpinBox; spinN_->setRange(2,8); spinN_->setValue(3); iLay->addWidget(spinN_);
    iLay->addSpacing(8);
    modeCombo_ = new QComboBox;
    modeCombo_->addItem(QStringLiteral("具体多项式"));
    modeCombo_->addItem(QStringLiteral("一般形式 (∑)"));
    connect(modeCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int idx){
        bool general = (idx == 1);
        spinNLabel_->setVisible(!general);
        spinN_->setVisible(!general);
    });
    iLay->addWidget(modeCombo_);
    iLay->addSpacing(8);
    iLay->addWidget(new QLabel(QStringLiteral("f =")));
    inputPoly_ = new QLineEdit; inputPoly_->setMinimumWidth(300);
    inputPoly_->setPlaceholderText(QStringLiteral("例如: x1^2*x2^2+x1^2*x3^2+x2^2*x3^2 (具体) 或 x1^2x2^2 (一般)"));
    iLay->addWidget(inputPoly_, 1);
    connect(inputPoly_, &QLineEdit::returnPressed, this, &SymReducePage::onSolve);
    cLay->addWidget(inW);

    auto* btnRow = new QHBoxLayout;
    auto* solveBtn = new QPushButton(QStringLiteral("开始求解"));
    solveBtn->setCursor(Qt::PointingHandCursor); solveBtn->setFixedWidth(160);
    solveBtn->setStyleSheet(QStringLiteral("QPushButton{background:#4A90D9;color:white;border-radius:6px;padding:8px 16px;font-size:14px;font-weight:600;}QPushButton:hover{background:#5BA0E9;}"));
    connect(solveBtn, &QPushButton::clicked, this, &SymReducePage::onSolve); btnRow->addWidget(solveBtn);
    auto* demoBtn = new QPushButton(QStringLiteral("演示例题"));
    demoBtn->setCursor(Qt::PointingHandCursor);
    demoBtn->setStyleSheet(QStringLiteral("QPushButton{background:#2196F3;color:white;border-radius:6px;padding:8px 16px;font-size:14px;font-weight:600;}QPushButton:hover{background:#1976D2;}"));
    connect(demoBtn, &QPushButton::clicked, this, &SymReducePage::onDemo); btnRow->addWidget(demoBtn);
    btnRow->addStretch(1); cLay->addLayout(btnRow);

    resultBrowser_ = new QTextBrowser; resultBrowser_->setOpenLinks(false); resultBrowser_->setMinimumHeight(400);
        attachLatexAutoPostProcess(resultBrowser_);
    resultBrowser_->setStyleSheet(QStringLiteral("QTextBrowser{border:1px solid #3A3D4A;border-radius:8px;padding:12px;}"));
    cLay->addWidget(resultBrowser_,1);
    scroll->setWidget(content); root->addWidget(scroll,1);
}

void SymReducePage::onSolve() {
    int n = spinN_->value();
    std::string input = inputPoly_->text().trimmed().toStdString();
    if (input.empty()) return;
    bool generalMode = (modeCombo_->currentIndex() == 1);

    auto th = RenderTheme::forCurrent();
    auto* doc = resultBrowser_->document(); doc->clear();
    QStringList parts;

    if (generalMode) {

        int effN = std::max(n, 4); 
        algemate::math::mpoly::GeneralResult result;
        try {
            result = algemate::math::mpoly::generalSymmetricReduction(input, effN);
        } catch (...) {
            resultBrowser_->setHtml(QStringLiteral("<p style=\"color:#E74C3C;\">计算出错，请检查输入格式。</p>"));
            return;
        }
        if (result.patterns.empty()) {
            resultBrowser_->setHtml(QStringLiteral("<p style=\"color:#E74C3C;\">无法处理，请检查输入。</p>"));
            return;
        }

        parts << titleHtml(QStringLiteral("用初等对称多项式表出对称多项式："), th);
        parts << formulaHtml(QStringLiteral("f(x_1,x_2,\\cdots,x_n) = \\sum %1.")
            .arg(QString::fromStdString(algemate::math::mpoly::patternToLatex(input))), th, doc);
        parts << sectionHtml(QStringLiteral("解"), th);

        int deg = 0;
        for (int v : result.patterns[0]) deg += v;
        parts << paraHtml(QStringLiteral("$f$ 的首项为 $%1$，是 $%2$ 次齐次对称多项式。")
            .arg(QString::fromStdString(algemate::math::mpoly::patternToLatex(input))).arg(deg), th, doc);
        parts << paraHtml(QStringLiteral("可能的指数组满足："), th, doc);

        QString patternsStr;
        for (std::size_t i = 0; i < result.patterns.size(); ++i) {
            if (i > 0) patternsStr += QStringLiteral(",\\ ");
            patternsStr += QStringLiteral("(");
            for (int j = 0; j < effN; ++j) {
                if (j) patternsStr += QStringLiteral(",");
                patternsStr += QString::number(result.patterns[i][j]);
            }
            patternsStr += QStringLiteral(",\\cdots)");
        }
        parts << formulaHtml(patternsStr + QStringLiteral("."), th, doc, 14);

        parts << paraHtml(QStringLiteral("设："), th, doc);
        for (std::size_t i = 0; i < result.sigmaExprs.size(); ++i) {
            QString coefName = QString::fromStdString(std::string(1, 'a' + static_cast<char>(i)));
            parts << formulaHtml(QStringLiteral("\\Phi_{%1} = %2\\,%3")
                .arg(i+1).arg(coefName).arg(QString::fromStdString(result.sigmaExprs[i])), th, doc, 14);
        }

        parts << paraHtml(QStringLiteral("解得："), th, doc);
        for (std::size_t i = 0; i < result.coefficients.size(); ++i) {
            QString coefName = QString::fromStdString(std::string(1, 'a' + static_cast<char>(i)));
            parts << formulaHtml(QStringLiteral("%1 = %2")
                .arg(coefName).arg(QString::fromStdString(result.coefficients[i].toLatex())), th, doc, 14);
        }

        parts << paraHtml(QStringLiteral("因此"), th, doc);
        parts << formulaHtml(QStringLiteral("f(x_1,x_2,\\cdots,x_n) = %1.")
            .arg(QString::fromStdString(result.finalExpr)), th, doc);

    } else {

        algemate::math::mpoly::MPolynomial poly;
        try {
            poly = algemate::math::mpoly::parseSymmetricPoly(input, n);
        } catch (...) {
            resultBrowser_->setHtml(QStringLiteral("<p style=\"color:#E74C3C;\">解析失败，请检查输入格式。</p>"));
            return;
        }
        if (poly.isZero() && !input.empty() && input != "0") {
            resultBrowser_->setHtml(QStringLiteral("<p style=\"color:#E74C3C;\">解析失败，请检查输入格式。</p>"));
            return;
        }

        std::string note;
        int symStatus = algemate::math::mpoly::ensureSymmetric(poly, n, note);
        if (symStatus == 0) {
            resultBrowser_->setHtml(QStringLiteral("<p style=\"color:#E74C3C;\">%1</p>").arg(QString::fromStdString(note)));
            return;
        }

        std::vector<std::string> xNames;
        for (int i = 1; i <= n; ++i) xNames.push_back("x_{" + std::to_string(i) + "}");

        parts << titleHtml(QStringLiteral("用初等对称多项式表示出对称多项式"), th);
        auto varList = [&]{ QString v; for(int i=1;i<=n;++i){if(i>1)v+=",";v+="x_"+QString::number(i);} return v; };
        parts << formulaHtml(QStringLiteral("f(%1) = %2.").arg(varList()).arg(QString::fromStdString(poly.toString(xNames))), th, doc);

        if (symStatus == 2) parts << paraHtml(QString::fromStdString(note), th, doc);

        parts << sectionHtml(QStringLiteral("解"), th);
        auto steps = algemate::math::mpoly::reduceSymmetric(poly, n);

        for (std::size_t si = 0; si < steps.size(); ++si) {
            auto& s = steps[si];
            std::string leadStr;
            for (int i = 0; i < n; ++i) { if (i) leadStr += ","; leadStr += std::to_string(s.leadExp[i]); }
            parts << paraHtml(QStringLiteral("$f_{%1}$ 的首项指数组为 $(%2)$，作多项式")
                .arg(si).arg(QString::fromStdString(leadStr)), th, doc);
            parts << formulaHtml(QStringLiteral("\\omega_{%1} = %2").arg(si+1).arg(QString::fromStdString(s.phiStr)), th, doc);
            if (si + 1 < steps.size()) {
                QString fNext = QString::fromStdString(steps[si+1].f.toString(xNames));
                if (fNext == "0") fNext = QStringLiteral("0");
                parts << paraHtml(QStringLiteral("令 $f_{%1} = f_{%2} - \\omega_{%3}$，得 $f_{%1} = %4$.")
                    .arg(si+1).arg(si).arg(si+1).arg(fNext), th, doc);
            }
        }

        if (!steps.empty()) {
            QString resultStr;
            for (std::size_t i = 0; i < steps.size(); ++i) {
                QString term = QString::fromStdString(steps[i].phiStr);
                if (i == 0) resultStr = term;
                else if (term.startsWith('-')) resultStr += QStringLiteral(" ") + term;
                else resultStr += QStringLiteral(" + ") + term;
            }
            parts << paraHtml(QStringLiteral("于是"), th, doc);
            parts << formulaHtml(QStringLiteral("f(%1) = %2.").arg(varList()).arg(resultStr), th, doc);
        }
    }

    resultBrowser_->setHtml(QStringLiteral("<div style=\"padding:4px;line-height:2.2;\">%1</div>").arg(parts.join(QString())));
}

void SymReducePage::onDemo() {
    if (modeCombo_->currentIndex() == 1) {

        spinN_->setValue(4);
        inputPoly_->setText(QStringLiteral("x1^2x2^2"));
    } else {

        spinN_->setValue(3);
        inputPoly_->setText(QStringLiteral("x1^2*x2^2+x1^2*x3^2+x2^2*x3^2"));
    }
    onSolve();
}

} 
