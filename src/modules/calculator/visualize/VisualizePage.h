#ifndef ALGEMATE_VISUALIZEPAGE_H
#define ALGEMATE_VISUALIZEPAGE_H

#include <QWidget>

namespace AlgeMate::Calculator::Visualize {

class VisualizePage : public QWidget {
    Q_OBJECT
public:
    explicit VisualizePage(QWidget* parent = nullptr);
};

}

#endif
