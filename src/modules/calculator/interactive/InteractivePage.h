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

// 输入框: Enter = 提交, Shift+Enter = 换行; 支持函数/变量智能补全
class InputEditor : public QPlainTextEdit {
    Q_OBJECT
public:
    explicit InputEditor(QWidget* parent = nullptr);

    // 设置补全词库 (函数名 / 变量名); 函数名选中时自动追加 '('
    void setCompletionData(const QStringList& functions, const QStringList& variables);

    // 开关补全 (关闭时隐藏 popup 且不再触发)
    void setCompletionEnabled(bool on);
    bool completionEnabled() const { return completionEnabled_; }

    // 为补全 popup 应用主题配色
    void applyCompleterTheme(const RenderTheme& th);

    // 插入带占位符的模板:
    //   prefix + placeholders[0] + ", " + placeholders[1] + ... + suffix
    //   placeholders 为空 → 光标定位到 prefix 与 suffix 之间
    //   否则 → 默认选中第一个占位符, Tab 跳到下一个, 箭头/Esc 退出
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

    // 由外部工具栏 (希腊字母 / 根号等) 调用, 将 text 插入到当前输入光标处
    void insertAtCursor(const QString& text);

    // 根号模板: 插入 sqrt() 光标自动置于括号中间
    void insertSqrtTemplate();
    // n 次根号模板: 插入 root(n, x) 默认选中 n, Tab 跳到 x
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

    // 保存每个 cell 的源数据, 切主题时整体重绘 (矩阵 pixmap 主题随快照)
    std::vector<std::pair<QString, EvalResult>> cells_;
};

}

#endif
