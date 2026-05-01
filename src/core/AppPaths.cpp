#include "AppPaths.h"

namespace AlgeMate::AppPaths {

QString appName()        { return QStringLiteral("AlgeMate"); }
QString appDisplayName() { return QStringLiteral("AlgeMate"); }
QString appSubtitle()    { return QStringLiteral("你的线性代数学习伙伴"); }

QString appVersion() {
#ifdef ALGEMATE_VERSION_STRING
    return QStringLiteral("v") + QStringLiteral(ALGEMATE_VERSION_STRING);
#else
    return QStringLiteral("v1.0.0");
#endif
}

}
