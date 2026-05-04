#include "RenderSettings.h"

#include "core/ThemeManager.h"

namespace AlgeMate::Calculator::Interactive {

RenderTheme RenderTheme::dark() {
    return RenderTheme{
        /*bgHistory */ "#1F1E31",
        /*bgCell    */ "#1F1E31",
        /*borderCell*/ "#2E2C48",
        /*accent    */ "#8B7BFF",
        /*accentSoft*/ "#B8ACFF",
        /*text      */ "#FFFFFF",
        /*textSoft  */ "#C8CAE0",
        /*textMuted */ "#8B8FB0",
        /*inputText */ "#FFFFFF",
        /*error     */ "#FF8A8A",
        /*errorSoft */ "#FFB3B3",
    };
}

RenderTheme RenderTheme::light() {
    return RenderTheme{
        /*bgHistory */ "#FFFFFF",
        /*bgCell    */ "#FFFFFF",
        /*borderCell*/ "#ECEAF6",
        /*accent    */ "#6C5CE7",
        /*accentSoft*/ "#8A7BFF",
        /*text      */ "#1F2033",
        /*textSoft  */ "#5A6074",
        /*textMuted */ "#8A8FA3",
        /*inputText */ "#2A2B3D",
        /*error     */ "#D93025",
        /*errorSoft */ "#A81C0D",
    };
}

RenderTheme RenderTheme::forCurrent() {
    auto t = AlgeMate::ThemeManager::instance().currentTheme();
    return t == AlgeMate::ThemeManager::Theme::Dark ? dark() : light();
}

}
