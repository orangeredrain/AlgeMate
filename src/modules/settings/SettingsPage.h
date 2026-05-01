#ifndef ALGEMATE_SETTINGSPAGE_H
#define ALGEMATE_SETTINGSPAGE_H

#include <QWidget>

namespace AlgeMate::Settings {

class SettingsPage : public QWidget {
    Q_OBJECT
public:
    explicit SettingsPage(QWidget* parent = nullptr);
};

}

#endif
