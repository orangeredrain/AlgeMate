#ifndef ALGEMATE_APPPATHS_H
#define ALGEMATE_APPPATHS_H

#include <QString>

namespace AlgeMate::AppPaths {

inline constexpr const char* kLightQss = ":/styles/light.qss";
inline constexpr const char* kDarkQss  = ":/styles/dark.qss";
inline constexpr const char* kAppIcon  = ":/images/icon.png";

QString appName();
QString appDisplayName();
QString appSubtitle();
QString appVersion();

}

#endif
