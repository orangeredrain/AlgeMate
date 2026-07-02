#include "RenderSettings.h"

#include "core/ThemeManager.h"

namespace AlgeMate::Calculator::Interactive {

RenderTheme RenderTheme::dark() {
    return RenderTheme{
         "#1F1E31",
         "#1F1E31",
         "#2E2C48",
         "#8B7BFF",
         "#B8ACFF",
         "#FFFFFF",
         "#C8CAE0",
         "#8B8FB0",
         "#FFFFFF",
         "#FF8A8A",
         "#FFB3B3",
    };
}

RenderTheme RenderTheme::light() {
    return RenderTheme{
         "#FFFFFF",
         "#FFFFFF",
         "#ECEAF6",
         "#6C5CE7",
         "#8A7BFF",
         "#1F2033",
         "#5A6074",
         "#8A8FA3",
         "#2A2B3D",
         "#D93025",
         "#A81C0D",
    };
}

RenderTheme RenderTheme::forCurrent() {
    auto t = AlgeMate::ThemeManager::instance().currentTheme();
    return t == AlgeMate::ThemeManager::Theme::Dark ? dark() : light();
}

}
