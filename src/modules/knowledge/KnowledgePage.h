#ifndef ALGEMATE_KNOWLEDGEPAGE_H
#define ALGEMATE_KNOWLEDGEPAGE_H

#include <QWidget>

namespace AlgeMate::Knowledge {

class KnowledgePage : public QWidget {
    Q_OBJECT
public:
    explicit KnowledgePage(QWidget* parent = nullptr);
};

}

#endif
