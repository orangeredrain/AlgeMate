#ifndef ALGEMATE_CALC_INTERACTIVE_RENDERSETTINGS_H
#define ALGEMATE_CALC_INTERACTIVE_RENDERSETTINGS_H

#include <QString>

namespace AlgeMate::Calculator::Interactive {

// 渲染调色板: HTML 片段里所有颜色都取自这里, 以便跟随 ThemeManager 切换
struct RenderTheme {
    QString bgHistory;   // 历史大方框背景
    QString bgCell;      // 单个 cell 的内部背景 (通常与 bgHistory 相同或微差)
    QString borderCell;  // cell 之间的分隔线
    QString accent;      // 主色 / 括号 / 标签
    QString accentSoft;  // 副主色 / 变量名
    QString text;        // 主文字
    QString textSoft;    // 次文字
    QString textMuted;   // 弱文字
    QString inputText;   // 输入回显
    QString error;       // 错误主色
    QString errorSoft;   // 错误描述

    static RenderTheme dark();
    static RenderTheme light();
    // 按 ThemeManager 当前主题返回; UI 侧通常缓存成员, 在 themeChanged 时重取
    static RenderTheme forCurrent();
};

// 数值格式: 精确代数数字串 / 固定小数位
struct DisplayFormat {
    enum Kind { Exact, Decimal };
    Kind kind     = Exact;
    int  decimals = 4;
};

}

#endif
