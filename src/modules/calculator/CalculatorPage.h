#ifndef ALGEMATE_CALCULATORPAGE_H
#define ALGEMATE_CALCULATORPAGE_H

#include <QWidget>

namespace AlgeMate::Calculator {

// 计算助手三栏容器: 交互式 / 可视化 / 算法演示
class CalculatorPage : public QWidget {
    Q_OBJECT
public:
    explicit CalculatorPage(QWidget* parent = nullptr);
};

}

#endif
