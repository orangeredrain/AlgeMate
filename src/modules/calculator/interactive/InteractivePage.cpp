#include "InteractivePage.h"

#include "HelpDialog.h"
#include "LatexTextBrowser.h"
#include "core/ThemeManager.h"
#include "latex/LatexRenderer.h"

#include <QAbstractItemView>
#include <QCheckBox>
#include <QComboBox>
#include <QCompleter>
#include <QFocusEvent>
#include <QFrame>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QPalette>
#include <QPushButton>
#include <QScrollBar>
#include <QSpinBox>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QTextBlock>
#include <QTextBrowser>
#include <QTextCursor>
#include <QTextEdit>
#include <QTextFragment>
#include <QMimeData>
#include <QPointer>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>
#include <QWheelEvent>
#include <algorithm>

namespace AlgeMate::Calculator::Interactive {

class WheelSpinBox : public QSpinBox {
public:
    explicit WheelSpinBox(QWidget* p = nullptr) : QSpinBox(p) {
        setFocusPolicy(Qt::StrongFocus);
    }
protected:
    void wheelEvent(QWheelEvent* e) override {
        if (!hasFocus()) setFocus(Qt::MouseFocusReason);
        QSpinBox::wheelEvent(e);
    }
};

// ======================== InputEditor (保持不动) ========================

InputEditor::InputEditor(QWidget* parent) : QPlainTextEdit(parent) {
    setObjectName(QStringLiteral("CalcInput"));
    setPlaceholderText(QStringLiteral("输入表达式 例如: m = [1, 2, 3; 4, 5, 6]\nEnter 执行, Shift+Enter 换行"));
    setTabChangesFocus(true);
    QFont f(QStringLiteral("Cascadia Mono"));
    f.setStyleHint(QFont::Monospace);
    f.setPointSize(12);
    setFont(f);
    setFixedHeight(84);

    completerModel_ = new QStandardItemModel(this);
    completer_      = new QCompleter(completerModel_, this);
    completer_->setWidget(this);
    completer_->setCompletionMode(QCompleter::PopupCompletion);
    completer_->setCaseSensitivity(Qt::CaseInsensitive);
    completer_->setFilterMode(Qt::MatchStartsWith);
    completer_->setMaxVisibleItems(10);
    completer_->setCompletionRole(Qt::UserRole);
    connect(completer_, QOverload<const QModelIndex&>::of(&QCompleter::activated),
            this, [this](const QModelIndex& idx) {
                if (!idx.isValid()) return;
                insertCompletion_(idx.data(Qt::UserRole).toString());
            });
}

void InputEditor::setCompletionData(const QStringList& functions, const QStringList& variables) {
    static const QMap<QString, QString> kFuncZh = {
                                                   {QStringLiteral("sqrt"),          QStringLiteral("平方根")},
                                                   {QStringLiteral("root"),          QStringLiteral("n 次根")},
                                                   {QStringLiteral("abs"),           QStringLiteral("绝对值")},
                                                   {QStringLiteral("re"),            QStringLiteral("实部")},
                                                   {QStringLiteral("im"),            QStringLiteral("虚部")},
                                                   {QStringLiteral("conj"),          QStringLiteral("复共轭")},
                                                   {QStringLiteral("arg"),           QStringLiteral("辐角")},
                                                   {QStringLiteral("Identity"),      QStringLiteral("单位矩阵")},
                                                   {QStringLiteral("zeros"),         QStringLiteral("零矩阵")},
                                                   {QStringLiteral("ones"),          QStringLiteral("全一矩阵")},
                                                   {QStringLiteral("tr"),            QStringLiteral("迹")},
                                                   {QStringLiteral("transpose"),     QStringLiteral("转置")},
                                                   {QStringLiteral("det"),           QStringLiteral("行列式")},
                                                   {QStringLiteral("rank"),          QStringLiteral("秩")},
                                                   {QStringLiteral("inv"),           QStringLiteral("逆矩阵")},
                                                   {QStringLiteral("rref"),          QStringLiteral("行最简阶梯形")},
                                                   {QStringLiteral("solve"),         QStringLiteral("解线性方程组")},
                                                   {QStringLiteral("nullspace"),     QStringLiteral("零空间")},
                                                   {QStringLiteral("charpoly"),      QStringLiteral("特征多项式")},
                                                   {QStringLiteral("eigs"),          QStringLiteral("实特征值")},
                                                   {QStringLiteral("ceigs"),         QStringLiteral("复特征值")},
                                                   {QStringLiteral("issym"),         QStringLiteral("是否对称")},
                                                   {QStringLiteral("signature"),     QStringLiteral("惯性指数")},
                                                   {QStringLiteral("definiteness"),  QStringLiteral("矩阵定性分类")},
                                                   {QStringLiteral("congdiag"),      QStringLiteral("合同对角化")},
                                                   {QStringLiteral("lu"),            QStringLiteral("LU 分解")},
                                                   {QStringLiteral("qr"),            QStringLiteral("QR 分解")},
                                                   {QStringLiteral("svd"),           QStringLiteral("奇异值分解")},
                                                   {QStringLiteral("gramschmidt"),   QStringLiteral("施密特正交化")},
                                                   {QStringLiteral("jordan"),        QStringLiteral("Jordan 标准形")},
                                                   {QStringLiteral("rcf"),           QStringLiteral("有理标准形")},
                                                   {QStringLiteral("polygcd"),       QStringLiteral("多项式 GCD")},
                                                   {QStringLiteral("factor"),        QStringLiteral("有理数域上完全因式分解")},
                                                   {QStringLiteral("resultant"),     QStringLiteral("结式")},
                                                   {QStringLiteral("discriminant"),  QStringLiteral("判别式")},
                                                   {QStringLiteral("rationalroots"), QStringLiteral("有理根")},
                                                   {QStringLiteral("squarefree"),    QStringLiteral("无平方因式")},
                                                   {QStringLiteral("minpoly"),       QStringLiteral("最小多项式")},
                                                   {QStringLiteral("irreducible"),   QStringLiteral("不可约性判定")},
                                                   {QStringLiteral("roots"),         QStringLiteral("复根")},
                                                   {QStringLiteral("rfactor"),       QStringLiteral("实数域因式分解")},
                                                   {QStringLiteral("irredcnt"),      QStringLiteral("有限域上不可约多项式的个数")},
                                                   {QStringLiteral("powerSumToSym"), QStringLiteral("幂和表示为初等对称多项式")},
                                                   {QStringLiteral("symToPowerSum"), QStringLiteral("初等对称多项式表示为幂和")},
                                                   };

    functionSet_ = QSet<QString>(functions.begin(), functions.end());
    completerModel_->clear();
    int nameWidth = 4;
    for (const QString& f : functions) nameWidth = std::max(nameWidth, static_cast<int>(f.length()));
    for (const QString& v : variables) nameWidth = std::max(nameWidth, static_cast<int>(v.length()));
    nameWidth += 2;

    QSet<QString> seen;
    auto appendRow = [&](const QString& name, const QString& zh) {
        if (name.isEmpty() || seen.contains(name)) return;
        seen.insert(name);
        auto* item = new QStandardItem;
        item->setEditable(false);
        item->setData(name, Qt::UserRole);
        QString display = zh.isEmpty() ? name : QStringLiteral("%1  —  %2").arg(name.leftJustified(nameWidth, QLatin1Char(' ')), zh);
        item->setData(display, Qt::DisplayRole);
        completerModel_->appendRow(item);
    };

    for (const QString& f : functions) appendRow(f, kFuncZh.value(f));
    for (const QString& v : variables) appendRow(v, QStringLiteral("变量"));
}

void InputEditor::setCompletionEnabled(bool on) {
    completionEnabled_ = on;
    if (!on && completer_ && completer_->popup()) completer_->popup()->hide();
}

void InputEditor::applyCompleterTheme(const RenderTheme& th) {
    if (!completer_ || !completer_->popup()) return;
    const QString qss = QString::fromLatin1(
                            "QAbstractItemView { background: %1; color: %2; border: 1px solid %3; border-radius: 8px; padding: 4px; outline: 0; font-family: 'Cascadia Mono','Consolas',monospace; font-size: 12px; selection-background-color: %4; selection-color: #ffffff; }"
                            "QAbstractItemView::item { padding: 5px 10px; border-radius: 5px; min-height: 18px; }"
                            "QAbstractItemView::item:selected { background: %4; color: #ffffff; }"
                            "QAbstractItemView::item:hover { background: %5; color: %2; }"
                            ).arg(th.bgHistory, th.text, th.borderCell, th.accent, th.accentSoft);
    completer_->popup()->setStyleSheet(qss);
}

QString InputEditor::currentWordPrefix_() const {
    const QTextCursor tc = textCursor();
    const int pos = tc.position();
    const QString doc = toPlainText();
    int start = pos;
    while (start > 0) {
        const QChar c = doc.at(start - 1);
        if (c.isLetterOrNumber() || c == QLatin1Char('_')) --start; else break;
    }
    QString p = doc.mid(start, pos - start);
    if (!p.isEmpty() && p.at(0).isDigit()) return QString();
    return p;
}

void InputEditor::maybeShowCompleter_() {
    if (!completionEnabled_ || !completer_) return;
    const QString prefix = currentWordPrefix_();
    if (prefix.length() < 1) { completer_->popup()->hide(); return; }
    if (prefix != completer_->completionPrefix()) {
        completer_->setCompletionPrefix(prefix);
        completer_->popup()->setCurrentIndex(completer_->completionModel()->index(0, 0));
    }
    if (completer_->completionCount() == 0) { completer_->popup()->hide(); return; }
    QRect cr = cursorRect();
    cr.setWidth(completer_->popup()->sizeHintForColumn(0) + completer_->popup()->verticalScrollBar()->sizeHint().width() + 24);
    cr.moveTop(cr.bottom() + 4);
    completer_->complete(cr);
}

void InputEditor::insertCompletion_(const QString& completion) {
    QTextCursor tc = textCursor();
    const int pos = tc.position();
    const QString doc = toPlainText();
    int start = pos;
    while (start > 0) {
        const QChar c = doc.at(start - 1);
        if (c.isLetterOrNumber() || c == QLatin1Char('_')) --start; else break;
    }
    tc.setPosition(start, QTextCursor::MoveAnchor);
    tc.setPosition(pos, QTextCursor::KeepAnchor);

    const bool isFunc = functionSet_.contains(completion);
    QString text = completion;
    if (isFunc) text += QStringLiteral("()");
    tc.insertText(text);
    setTextCursor(tc);

    if (isFunc) {
        QTextCursor cur = textCursor();
        cur.movePosition(QTextCursor::Left);
        setTextCursor(cur);
    }
}

void InputEditor::clearSnippet_() {
    snippetStops_.clear();
    snippetIdx_ = -1;
    snippetCurrentDirty_ = false;
    setExtraSelections({});
}

void InputEditor::refreshSnippetHighlight_() {
    QList<QTextEdit::ExtraSelection> sels;
    if (!snippetStops_.isEmpty()) {
        QTextCharFormat fmt;
        fmt.setBackground(palette().brush(QPalette::Highlight));
        fmt.setForeground(palette().brush(QPalette::HighlightedText));
        for (int i = 0; i < snippetStops_.size(); ++i) {
            QTextEdit::ExtraSelection sel;
            sel.cursor = snippetStops_[i];
            sel.format = fmt;
            sels.append(sel);
        }
    }
    setExtraSelections(sels);
}

void InputEditor::insertSnippet(const QString& prefix, const QStringList& placeholders, const QString& suffix) {
    setFocus();
    QTextCursor tc = textCursor();
    const int startPos = tc.position();

    QString joined = prefix;
    QList<QPair<int, int>> spans;
    for (int i = 0; i < placeholders.size(); ++i) {
        if (i > 0) joined += QStringLiteral(", ");
        spans.append({joined.length(), static_cast<int>(placeholders[i].length())});
        joined += placeholders[i];
    }
    joined += suffix;

    tc.beginEditBlock();
    tc.insertText(joined);
    tc.endEditBlock();

    clearSnippet_();
    if (placeholders.isEmpty()) {
        QTextCursor c = textCursor();
        c.setPosition(startPos + prefix.length());
        setTextCursor(c);
        return;
    }

    for (const auto& s : spans) {
        QTextCursor c(document());
        c.setPosition(startPos + s.first + s.second);
        c.setPosition(startPos + s.first, QTextCursor::KeepAnchor);
        snippetStops_.append(c);
    }
    snippetExit_ = QTextCursor(document());
    snippetExit_.setPosition(startPos + joined.length());
    snippetIdx_ = 0;
    snippetCurrentDirty_ = false;
    {
        const QTextCursor& stop = snippetStops_.first();
        QTextCursor c = textCursor();
        c.setPosition(qMin(stop.anchor(), stop.position()));
        setTextCursor(c);
    }
    refreshSnippetHighlight_();
}

void InputEditor::keyPressEvent(QKeyEvent* e) {
    if (completer_ && completer_->popup()->isVisible()) {
        switch (e->key()) {
        case Qt::Key_Tab: {
            QModelIndex cur = completer_->popup()->currentIndex();
            if (!cur.isValid()) cur = completer_->completionModel()->index(0, 0);
            if (cur.isValid()) {
                const QString name = cur.data(Qt::UserRole).toString();
                completer_->popup()->hide();
                insertCompletion_(name);
            }
            e->accept(); return;
        }
        case Qt::Key_Escape: completer_->popup()->hide(); e->accept(); return;
        case Qt::Key_Up: case Qt::Key_Down: case Qt::Key_PageUp: case Qt::Key_PageDown: e->ignore(); return;
        case Qt::Key_Backtab: e->ignore(); return;
        case Qt::Key_Return: case Qt::Key_Enter: completer_->popup()->hide(); break;
        default: break;
        }
    }

    if (!snippetStops_.isEmpty()) {
        const bool advance = (e->key() == Qt::Key_Tab && !(e->modifiers() & Qt::ShiftModifier))
        || (e->key() == Qt::Key_Right && !(e->modifiers() & (Qt::ShiftModifier | Qt::ControlModifier | Qt::AltModifier)));
        if (advance) {
            snippetIdx_ += 1;
            if (snippetIdx_ < snippetStops_.size()) {
                const QTextCursor& stop = snippetStops_[snippetIdx_];
                QTextCursor c = textCursor();
                c.setPosition(qMin(stop.anchor(), stop.position()));
                setTextCursor(c);
                snippetCurrentDirty_ = false;
                refreshSnippetHighlight_();
            } else {
                QTextCursor c = textCursor();
                c.setPosition(snippetExit_.position());
                setTextCursor(c);
                clearSnippet_();
            }
            e->accept(); return;
        }
        switch (e->key()) {
        case Qt::Key_Left: case Qt::Key_Up: case Qt::Key_Down: case Qt::Key_Home: case Qt::Key_End: case Qt::Key_Escape: clearSnippet_(); break;
        default: break;
        }

        if (!snippetCurrentDirty_ && snippetIdx_ >= 0 && snippetIdx_ < snippetStops_.size() && !e->text().isEmpty() && e->text().at(0).isPrint() && !(e->modifiers() & (Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier))) {
            setTextCursor(snippetStops_[snippetIdx_]);
            snippetCurrentDirty_ = true;
        }
    }

    if (e->key() == Qt::Key_Backspace && !(e->modifiers() & (Qt::ControlModifier | Qt::AltModifier | Qt::ShiftModifier))) {
        QTextCursor tc = textCursor();
        if (!tc.hasSelection()) {
            const int pos = tc.position();
            const QString doc = toPlainText();
            if (pos > 0) {
                const QChar prev = doc.at(pos - 1);
                const QChar next = (pos < doc.length()) ? doc.at(pos) : QChar();

                auto isIdent = [](QChar c) { return c.isLetterOrNumber() || c == QLatin1Char('_'); };
                auto selectIdentBefore = [&](int curPos) {
                    const QString now = toPlainText();
                    int start = curPos;
                    while (start > 0 && isIdent(now.at(start - 1))) --start;
                    if (start < curPos && (now.at(start).isLetter() || now.at(start) == QLatin1Char('_'))) {
                        QTextCursor sel = textCursor();
                        sel.setPosition(start, QTextCursor::MoveAnchor);
                        sel.setPosition(curPos, QTextCursor::KeepAnchor);
                        setTextCursor(sel);
                    }
                };
                auto hidePopup = [&]() { if (completer_ && completer_->popup()->isVisible()) completer_->popup()->hide(); };

                const bool isPair = (prev == QLatin1Char('(')  && next == QLatin1Char(')')) || (prev == QLatin1Char('[')  && next == QLatin1Char(']')) || (prev == QLatin1Char('{')  && next == QLatin1Char('}')) || (prev == QLatin1Char('"') && next == QLatin1Char('"')) || (prev == QLatin1Char('\'') && next == QLatin1Char('\''));
                if (isPair) {
                    tc.beginEditBlock(); tc.deleteChar(); tc.deletePreviousChar(); tc.endEditBlock();
                    hidePopup();
                    if (prev == QLatin1Char('(') || prev == QLatin1Char('[') || prev == QLatin1Char('{')) { selectIdentBefore(textCursor().position()); }
                    e->accept(); return;
                }

                if (prev == QLatin1Char(')') || prev == QLatin1Char(']') || prev == QLatin1Char('}')) {
                    const QChar openCh = (prev == QLatin1Char(')')) ? QLatin1Char('(') : (prev == QLatin1Char(']')) ? QLatin1Char('[') : QLatin1Char('{');
                    int depth = 1; int L = -1;
                    for (int i = pos - 2; i >= 0; --i) {
                        const QChar c = doc.at(i);
                        if (c == prev) { ++depth; } else if (c == openCh) { if (--depth == 0) { L = i; break; } }
                    }
                    if (L >= 0) {
                        tc.deletePreviousChar(); hidePopup();
                        const int innerStart = L + 1; const int innerEnd   = pos - 1;
                        if (innerEnd > innerStart) {
                            QTextCursor sel = textCursor(); sel.setPosition(innerStart, QTextCursor::MoveAnchor); sel.setPosition(innerEnd, QTextCursor::KeepAnchor); setTextCursor(sel);
                        }
                        e->accept(); return;
                    }
                }

                if (prev == QLatin1Char('(') || prev == QLatin1Char('[') || prev == QLatin1Char('{')) {
                    tc.deletePreviousChar(); hidePopup(); selectIdentBefore(textCursor().position()); e->accept(); return;
                }
            }
        }
    }

    const bool isReturn = (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter);
    if (isReturn && !(e->modifiers() & Qt::ShiftModifier)) { emit submitRequested(); e->accept(); return; }

    if (!textCursor().hasSelection() && !(e->modifiers() & (Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier))) {
        if (e->text() == QLatin1String("(")) {
            QTextCursor tc = textCursor(); tc.beginEditBlock(); tc.insertText(QStringLiteral("()")); tc.movePosition(QTextCursor::Left); tc.endEditBlock(); setTextCursor(tc); e->accept(); return;
        }
        if (e->text() == QLatin1String("[")) {
            QTextCursor tc = textCursor(); tc.beginEditBlock(); tc.insertText(QStringLiteral("[]")); tc.movePosition(QTextCursor::Left); tc.endEditBlock(); setTextCursor(tc); e->accept(); return;
        }
    }
    if (!textCursor().hasSelection() && !(e->modifiers() & (Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier))) {
        const QString doc = toPlainText(); const int pos = textCursor().position();
        if (pos < doc.length()) {
            const QChar nextChar = doc.at(pos);
            if (e->text() == QLatin1String(")") && nextChar == QLatin1Char(')')) { QTextCursor tc = textCursor(); tc.movePosition(QTextCursor::Right); setTextCursor(tc); e->accept(); return; }
            if (e->text() == QLatin1String("]") && nextChar == QLatin1Char(']')) { QTextCursor tc = textCursor(); tc.movePosition(QTextCursor::Right); setTextCursor(tc); e->accept(); return; }
        }
    }

    QPlainTextEdit::keyPressEvent(e);

    if (!completionEnabled_) return;
    const QString txt = e->text();
    const bool isWordChar = !txt.isEmpty() && (txt.at(0).isLetterOrNumber() || txt.at(0) == QLatin1Char('_'));
    const bool isBackspace = (e->key() == Qt::Key_Backspace);
    if (isWordChar || isBackspace) { maybeShowCompleter_(); } else { completer_->popup()->hide(); }
}

void InputEditor::focusInEvent(QFocusEvent* e) {
    if (completer_) completer_->setWidget(this);
    QPlainTextEdit::focusInEvent(e);
}

bool InputEditor::focusNextPrevChild(bool next) {
    if (completer_ && completer_->popup() && completer_->popup()->isVisible()) return false;
    if (!snippetStops_.isEmpty()) return false;
    return QPlainTextEdit::focusNextPrevChild(next);
}

// ======================== InteractivePage ========================
InteractivePage::InteractivePage(QWidget* parent) : QWidget(parent) {
    theme_ = RenderTheme::forCurrent();

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(12);

    history_ = new LatexTextBrowser;
    history_->setObjectName(QStringLiteral("CalcHistory"));
    history_->setOpenLinks(false);
    history_->setOpenExternalLinks(false);
    history_->setFrameShape(QFrame::NoFrame);
    AlgeMate::Latex::attachLatexAutoPostProcess(history_);
    QFont historyFont(QStringLiteral("Cascadia Mono"));
    historyFont.setStyleHint(QFont::Monospace);
    historyFont.setPointSize(12);
    history_->setFont(historyFont);

    auto* hisWrapper = new QVBoxLayout;
    hisWrapper->setContentsMargins(16, 16, 16, 0);
    hisWrapper->addWidget(history_);
    root->addLayout(hisWrapper, 1);

    auto* inputCard = new QFrame;
    inputCard->setObjectName(QStringLiteral("CalcInputCard"));
    auto* cardLay = new QVBoxLayout(inputCard);
    cardLay->setContentsMargins(20, 16, 20, 16);
    cardLay->setSpacing(12);

    input_ = new InputEditor;
    input_->setStyleSheet("QPlainTextEdit { border: none; background: transparent; outline: none; }");

    auto* toolRow = new QHBoxLayout;
    toolRow->setSpacing(12);

    varsLabel_ = new QLabel(QStringLiteral("变量: (空)"));
    varsLabel_->setObjectName(QStringLiteral("CalcVarsLabel"));

    fmtCombo_ = new QComboBox;
    fmtCombo_->addItem(QStringLiteral("精确"), int(DisplayFormat::Exact));
    fmtCombo_->addItem(QStringLiteral("数值"), int(DisplayFormat::Decimal));
    fmtCombo_->setCurrentIndex(0);
    fmtCombo_->setCursor(Qt::PointingHandCursor);

    decLabel_ = new QLabel(QStringLiteral("位数"));
    decSpin_ = new WheelSpinBox;
    decSpin_->setRange(0, 15);
    decSpin_->setValue(format_.decimals);
    decSpin_->setButtonSymbols(QAbstractSpinBox::NoButtons);
    decSpin_->setAlignment(Qt::AlignCenter);
    decSpin_->setFixedWidth(46);
    decSpin_->setEnabled(false);

    auto* helpBtn = new QPushButton(QStringLiteral("帮助"));
    helpBtn->setCursor(Qt::PointingHandCursor);

    completeCheck_ = new QCheckBox(QStringLiteral("智能补全"));
    completeCheck_->setChecked(true);
    completeCheck_->setCursor(Qt::PointingHandCursor);

    auto* clearVarBtn = new QPushButton(QStringLiteral("清空变量"));
    clearVarBtn->setCursor(Qt::PointingHandCursor);

    auto* clearHisBtn = new QPushButton(QStringLiteral("清空历史"));
    clearHisBtn->setCursor(Qt::PointingHandCursor);

    runBtn_ = new QPushButton(QStringLiteral(" 执行  ⏎ "));
    runBtn_->setObjectName("RunBtn");
    runBtn_->setCursor(Qt::PointingHandCursor);

    toolRow->addWidget(varsLabel_, 1);
    toolRow->addWidget(completeCheck_);
    toolRow->addWidget(fmtCombo_);
    toolRow->addWidget(decLabel_);
    toolRow->addWidget(decSpin_);
    toolRow->addWidget(helpBtn);
    toolRow->addWidget(clearVarBtn);
    toolRow->addWidget(clearHisBtn);
    toolRow->addWidget(runBtn_);

    cardLay->addWidget(input_);

    auto* line = new QFrame;
    line->setFixedHeight(1);
    line->setObjectName("InputDivider");
    cardLay->addWidget(line);

    cardLay->addLayout(toolRow);
    root->addWidget(inputCard);

    // ---- 信号 ----
    connect(runBtn_,      &QPushButton::clicked,             this, &InteractivePage::onRun);
    connect(input_,       &InputEditor::submitRequested,     this, &InteractivePage::onRun);
    connect(helpBtn,      &QPushButton::clicked,             this, &InteractivePage::onShowHelp);
    connect(clearVarBtn,  &QPushButton::clicked,             this, &InteractivePage::onClearVariables);
    connect(clearHisBtn,  &QPushButton::clicked,             this, &InteractivePage::onClearHistory);
    connect(fmtCombo_,    QOverload<int>::of(&QComboBox::currentIndexChanged), this, &InteractivePage::onFormatKindChanged);
    connect(decSpin_,     QOverload<int>::of(&QSpinBox::valueChanged), this, &InteractivePage::onDecimalsChanged);
    connect(completeCheck_, &QCheckBox::toggled, this, [this](bool on) { if (input_) input_->setCompletionEnabled(on); });

    // ====== UI 动态主题重绘 ======
    auto applyTheme = [this, inputCard, line, helpBtn, clearVarBtn, clearHisBtn]() {
        bool isDark = AlgeMate::ThemeManager::instance().currentTheme() == AlgeMate::ThemeManager::Theme::Dark;

        inputCard->setStyleSheet(isDark ? "QFrame#CalcInputCard { background: #1C1B2E; border-top: 1px solid #3B395A; }" : "QFrame#CalcInputCard { background: #FFFFFF; border-top: 1px solid #E2E8F0; }");
        line->setStyleSheet(isDark ? "background: #28263F;" : "background: #F1F5F9;");

        QString btnStyle = isDark ?
                               "QPushButton { background: transparent; color: #8A8FA3; border: 1px solid #3B395A; border-radius: 6px; padding: 6px 12px; font-size: 13px; } QPushButton:hover { background: #28263F; color: #E6E7F0; }"
                                  :
                               "QPushButton { background: #F8FAFC; color: #475569; border: 1px solid #E2E8F0; border-radius: 6px; padding: 6px 12px; font-size: 13px; } QPushButton:hover { background: #F1F5F9; color: #0F172A; }";
        helpBtn->setStyleSheet(btnStyle);
        clearVarBtn->setStyleSheet(btnStyle);
        clearHisBtn->setStyleSheet(btnStyle);

        QString inputStyle = isDark ?
                                 "QComboBox, QSpinBox { background: transparent; color: #E6E7F0; border: 1px solid #3B395A; border-radius: 6px; padding: 4px 8px; } QComboBox::drop-down { border: none; width: 20px; }"
                                    :
                                 "QComboBox, QSpinBox { background: transparent; color: #1E293B; border: 1px solid #CBD5E1; border-radius: 6px; padding: 4px 8px; } QComboBox::drop-down { border: none; width: 20px; }";
        fmtCombo_->setStyleSheet(inputStyle);
        decSpin_->setStyleSheet(inputStyle);

        runBtn_->setStyleSheet(isDark ?
                                   "QPushButton#RunBtn { background: #5046E5; color: white; border: none; border-radius: 6px; padding: 6px 16px; font-weight: bold; font-size: 13px; } QPushButton#RunBtn:hover { background: #6366F1; }"
                                      :
                                   "QPushButton#RunBtn { background: #4F46E5; color: white; border: none; border-radius: 6px; padding: 6px 16px; font-weight: bold; font-size: 13px; } QPushButton#RunBtn:hover { background: #6366F1; }");
    };

    applyTheme();

    connect(&AlgeMate::ThemeManager::instance(), &AlgeMate::ThemeManager::themeChanged, this, [this, applyTheme](AlgeMate::ThemeManager::Theme){
        applyTheme();
        onThemeChanged(); // 保留原有的渲染刷新
    });

    applyHistoryPalette();
    input_->applyCompleterTheme(theme_);
    refreshCompletionWords();
    showWelcome();
    refreshVars();
    input_->setFocus();
}

// ======================== 主题 / 格式 ========================

void InteractivePage::applyHistoryPalette() {
    QPalette p = history_->palette();
    p.setColor(QPalette::Base,       QColor(theme_.bgHistory));
    p.setColor(QPalette::Text,       QColor(theme_.text));
    p.setColor(QPalette::WindowText, QColor(theme_.text));
    history_->setPalette(p);
    history_->document()->setDefaultStyleSheet(QStringLiteral("body { color: %1; }").arg(theme_.text));
}

void InteractivePage::onThemeChanged() {
    theme_ = RenderTheme::forCurrent();
    applyHistoryPalette();
    if (input_) input_->applyCompleterTheme(theme_);
    redrawHistory();
}

void InteractivePage::redrawHistory() {
    history_->clear();
    clearLatexImageCache();
    const int savedCounter = counter_;
    counter_ = 0;
    showWelcome();
    auto snap = cells_;
    cells_.clear();
    for (const auto& pr : snap) {
        ++counter_;
        appendCell(pr.first, pr.second);
        cells_.push_back(pr);
    }
    counter_ = savedCounter;
}

void InteractivePage::onFormatKindChanged(int idx) {
    format_.kind = (idx == 1) ? DisplayFormat::Decimal : DisplayFormat::Exact;
    decSpin_->setEnabled(format_.kind == DisplayFormat::Decimal);
}

void InteractivePage::onDecimalsChanged(int v) { format_.decimals = v; }
void InteractivePage::insertAtCursor(const QString& text) { if (input_) { input_->insertPlainText(text); input_->setFocus(); } }
void InteractivePage::insertSqrtTemplate() { if (input_) input_->insertSnippet(QStringLiteral("sqrt("), {}, QStringLiteral(")")); }
void InteractivePage::insertRootTemplate() { if (input_) input_->insertSnippet(QStringLiteral("root("), {QStringLiteral("n"), QStringLiteral("x")}, QStringLiteral(")")); }

void InteractivePage::showWelcome() {
    const QString html = QStringLiteral("<div style=\"margin:6px 2px 12px 2px; padding:14px 16px; border-left:3px solid %1;\"><div style=\"color:%2; font-size:15px; font-weight:700; margin-bottom:6px;\">欢迎使用 AlgeMate · 交互式计算</div><div style=\"color:%3; font-size:13px; line-height:1.6;\">支持 <b>整数、分数精确</b> 运算, 输入表达式后按 Enter 执行. 右下工具条可在 <b>精确</b> 与 <b>数值</b> 之间切换, 并设置小数位数.<br/></div></div>").arg(theme_.accent, theme_.accentSoft, theme_.textSoft);
    appendHtml(html);
}

void InteractivePage::onRun() {
    const QString src = input_->toPlainText().trimmed();
    if (src.isEmpty()) return;
    EvalResult r = eval_.evaluate(src);
    ++counter_;
    appendCell(src, r);
    cells_.emplace_back(src, r);
    input_->clear();
    input_->setFocus();
    refreshVars();
    refreshCompletionWords();
}

void InteractivePage::onClearHistory() {
    history_->clear();
    clearLatexImageCache();
    counter_ = 0;
    cells_.clear();
    showWelcome();
}

void InteractivePage::onClearVariables() {
    eval_.clear();
    refreshVars();
    refreshCompletionWords();
    appendHtml(QStringLiteral("<div style=\"margin:6px 0; color:%1; font-size:12px;\">— 已清空所有变量 —</div>").arg(theme_.textMuted));
}

void InteractivePage::onShowHelp() {
    auto* dlg = new HelpDialog(this);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->show();
}

void InteractivePage::refreshVars() {
    const auto& env = eval_.env();
    if (env.empty()) { varsLabel_->setText(QStringLiteral("变量: (空)")); return; }
    QStringList names;
    for (const auto& kv : env) names << QString::fromStdString(kv.first);
    std::sort(names.begin(), names.end());
    varsLabel_->setText(QStringLiteral("变量: %1").arg(names.join(QStringLiteral(", "))));
}

void InteractivePage::refreshCompletionWords() {
    if (!input_) return;
    QSet<QString> fnSet;
    for (const QString& sig : Evaluator::supportedFunctions()) {
        const int p = sig.indexOf(QLatin1Char('('));
        const QString name = (p < 0 ? sig : sig.left(p)).trimmed();
        if (!name.isEmpty()) fnSet.insert(name);
    }
    QStringList functions(fnSet.begin(), fnSet.end());
    std::sort(functions.begin(), functions.end());
    QStringList variables;
    for (const auto& kv : eval_.env()) variables << QString::fromStdString(kv.first);
    std::sort(variables.begin(), variables.end());
    input_->setCompletionData(functions, variables);
}

void InteractivePage::appendHtml(const QString& html) {
    history_->append(html);
    auto* sb = history_->verticalScrollBar();
    sb->setValue(sb->maximum());
    QPointer<QTextBrowser> guard(history_);
    QTimer::singleShot(0, history_, [guard]() { if (!guard) return; auto* s = guard->verticalScrollBar(); s->setValue(s->maximum()); });
    QTimer::singleShot(16, history_, [guard]() { if (!guard) return; auto* s = guard->verticalScrollBar(); s->setValue(s->maximum()); });
}

void InteractivePage::appendCell(const QString& source, const EvalResult& r) {
    const QString inLabel  = QStringLiteral("In[%1]").arg(counter_);
    const QString outLabel = QStringLiteral("Out[%1]").arg(counter_);
    const RenderTheme th = theme_;
    QString cell;
    cell += QStringLiteral("<div style=\"margin:4px 0 10px 0; padding:10px 6px 14px 6px; border-bottom:1px solid %1;\">").arg(th.borderCell);
    cell += QStringLiteral("<div style=\"font-family:'Cascadia Mono','Consolas',monospace; font-size:12px; margin-bottom:8px;\"><span style=\"color:%1; font-weight:700;\">%2</span><span style=\"color:%3;\"> := </span><span style=\"color:%4;\">%5</span></div>").arg(th.accent, inLabel, th.textMuted, th.inputText, source.toHtmlEscaped());

    if (!r.ok) {
        cell += QStringLiteral("<div style=\"font-family:'Cascadia Mono','Consolas',monospace; font-size:12px;\"><span style=\"color:%1; font-weight:700;\">Error</span><span style=\"color:%2;\"> → </span><span style=\"color:%3;\">%4</span></div>").arg(th.error, th.textMuted, th.errorSoft, r.error.toHtmlEscaped());
    } else {
        QString head;
        if (!r.assignedName.isEmpty()) head = QStringLiteral("<span style=\"color:%1; font-weight:700;\">%2</span><span style=\"color:%3;\"> &larr; </span><span style=\"color:%4;\">%5 &nbsp;(%6)</span>").arg(th.accent, outLabel, th.textMuted, th.textSoft, r.assignedName, r.typeDesc);
        else head = QStringLiteral("<span style=\"color:%1; font-weight:700;\">%2</span><span style=\"color:%3;\"> = </span><span style=\"color:%4;\">%5</span>").arg(th.accent, outLabel, th.textMuted, th.textSoft, r.typeDesc);
        cell += QStringLiteral("<div style=\"font-family:'Cascadia Mono','Consolas',monospace; font-size:12px; margin-bottom:4px;\">%1</div>").arg(head);
        cell += QStringLiteral("<div style=\"margin:4px 0 0 10px;\">%1</div>").arg(r.value.toHtml(th, format_, history_->document()));
        if (!r.extraNote.isEmpty()) cell += QStringLiteral("<div style=\"color:%1; font-size:11px; margin-top:6px; font-family:'Cascadia Mono','Consolas',monospace;\">%2</div>").arg(th.textMuted, renderNoteWithLatex(r.extraNote, th, history_->document()));
    }
    cell += QStringLiteral("</div>");
    appendHtml(cell);
}

} // namespace AlgeMate::Calculator::Interactive