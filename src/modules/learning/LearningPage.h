#ifndef ALGEMATE_LEARNINGPAGE_H
#define ALGEMATE_LEARNINGPAGE_H

#include <QWidget>

namespace AlgeMate::Learning {

class LearningPage : public QWidget {
    Q_OBJECT
public:
    explicit LearningPage(QWidget* parent = nullptr);
};

}

#endif
