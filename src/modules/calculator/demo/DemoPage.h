#ifndef ALGEMATE_DEMOPAGE_H
#define ALGEMATE_DEMOPAGE_H

#include <QWidget>

class QStackedWidget;

namespace AlgeMate::Calculator::Demo {

class DemoPage : public QWidget {
    Q_OBJECT
public:
    explicit DemoPage(QWidget* parent = nullptr);

private:
    QStackedWidget* stack_ = nullptr;

    void buildCatalog();
    void addCard(QWidget* grid, int row, int col,
                 const QString& icon, const QString& title,
                 const QString& desc, int pageIndex);
};

}

#endif
