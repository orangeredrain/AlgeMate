#ifndef ALGEMATE_CALCULATORPAGE_H
#define ALGEMATE_CALCULATORPAGE_H

#include <QWidget>

namespace AlgeMate::Calculator {

class CalculatorPage : public QWidget {
    Q_OBJECT
public:
    explicit CalculatorPage(QWidget* parent = nullptr);
};

}

#endif
