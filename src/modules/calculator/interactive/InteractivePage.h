#ifndef ALGEMATE_CALC_INTERACTIVE_PAGE_H
#define ALGEMATE_CALC_INTERACTIVE_PAGE_H

#include "expr/Evaluator.h"
#include "expr/RenderSettings.h"

#include <QPlainTextEdit>
#include <QList>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QTextCursor>
#include <QWidget>
#include <vector>
#include <utility>

class QTextBrowser;
class QLabel;
class QPushButton;
class QComboBox;
class QSpinBox;
class QCheckBox;
class QCompleter;
class QStandardItemModel;

namespace AlgeMate::Calculator::Interactive {

// 输入框: Enter = 提交, Shift+Enter = 换行
class InputEditor : public QPlainTextEdit {
    Q_OBJECT
public:
    explicit InputEditor(QWidget* parent = nullptr);

    void setCompletionData(const QStringList& functions, const QStringList& variables);

    void setCompletionEnabled(bool on);
    bool completionEnabled() const { return completionEnabled_; }

    void applyCompleterTheme(const RenderTheme& th);

    void insertSnippet(const QString& prefix,
                       const QStringList& placeholders,
                       const QString& suffix);

signals:
    void submitRequested();
protected:
    void keyPressEvent(QKeyEvent* e) override;
    void focusInEvent(QFocusEvent* e) override;
    bool focusNextPrevChild(bool next) override;

private:
    QCompleter*        completer_        = nullptr;
    QStandardItemModel* completerModel_  = nullptr;
    QSet<QString>      functionSet_;
    bool               completionEnabled_ = true;

    QList<QTextCursor> snippetStops_;
    int                snippetIdx_ = -1;
    bool               snippetCurrentDirty_ = false;
    QTextCursor        snippetExit_;

    QString currentWordPrefix_() const;
    void    maybeShowCompleter_();
    void    insertCompletion_(const QString& completion);
    void    clearSnippet_();
    void    refreshSnippetHighlight_();
};

class InteractivePage : public QWidget {
    Q_OBJECT
public:
    explicit InteractivePage(QWidget* parent = nullptr);

    void insertAtCursor(const QString& text);

    // 插入 sqrt() 光标自动置于括号中间
    void insertSqrtTemplate();
    // 插入 root(n, x) 默认选中 n, Tab 跳到 x
    void insertRootTemplate();

private slots:
    void onRun();
    void onClearHistory();
    void onClearVariables();
    void onShowHelp();
    void onFormatKindChanged(int idx);
    void onDecimalsChanged(int v);
    void onThemeChanged();

private:
    Evaluator      eval_;
    int            counter_ = 0;

    // 渲染设置
    RenderTheme    theme_;
    DisplayFormat  format_;

    QTextBrowser*  history_     = nullptr;
    InputEditor*   input_       = nullptr;
    QLabel*        varsLabel_   = nullptr;
    QPushButton*   runBtn_      = nullptr;
    QComboBox*     fmtCombo_    = nullptr;
    QSpinBox*      decSpin_     = nullptr;
    QLabel*        decLabel_    = nullptr;
    QCheckBox*     completeCheck_ = nullptr;

    void appendHtml(const QString& html);
    void appendCell(const QString& source, const EvalResult& r);
    void refreshVars();
    void refreshCompletionWords();
    void showWelcome();
    void applyHistoryPalette();
    void redrawHistory();

    // 保存每个 cell 的源数据, 切主题时整体重绘
    std::vector<std::pair<QString, EvalResult>> cells_;
};

}

#endif
