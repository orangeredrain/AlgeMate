#ifndef ALGEMATE_CALC_INTERACTIVE_RENDERSETTINGS_H
#define ALGEMATE_CALC_INTERACTIVE_RENDERSETTINGS_H

#include <QString>

namespace AlgeMate::Calculator::Interactive {

struct RenderTheme {
    QString bgHistory;   
    QString bgCell;      
    QString borderCell;  
    QString accent;      
    QString accentSoft;  
    QString text;        
    QString textSoft;    
    QString textMuted;   
    QString inputText;   
    QString error;       
    QString errorSoft;   

    static RenderTheme dark();
    static RenderTheme light();

    static RenderTheme forCurrent();
};

struct DisplayFormat {
    enum Kind { Exact, Decimal };
    Kind kind     = Exact;
    int  decimals = 4;
};

}

#endif
