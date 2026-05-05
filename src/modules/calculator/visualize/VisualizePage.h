#ifndef ALGEMATE_VISUALIZEPAGE_H
#define ALGEMATE_VISUALIZEPAGE_H

#include <QWidget>
#include <vector>

class QComboBox;
class QLineEdit;
class QTextBrowser;
class QVBoxLayout;
class QPushButton;
class QDoubleSpinBox;
class QTabWidget;

namespace AlgeMate::Calculator::Visualize {

class QuadricWidget;

struct QuadricClass {
    int id;
    const char* name;
    const char* stdEq;
    int p, q, r;
    const char* geo;
    const char* cat;
};

class VisualizePage : public QWidget {
    Q_OBJECT
public:
    explicit VisualizePage(QWidget* parent = nullptr);

private slots:
    void onAnalyze();
    void onPresetSelected(int index);
    void onResetView();

private:
    QWidget* createInputTab();
    QWidget* createPresetTab();
    void setupRenderArea(QWidget* parent);

    void classifyAndRender(const std::vector<double>& coeffs);
    void renderPreset(int id, double a, double b, double c, double p);
    void updateInfo(int cls);

    // Input
    QLineEdit* coeffEdit_[10] = {};
    QPushButton* analyzeBtn_ = nullptr;

    // Preset
    QComboBox*    presetCombo_ = nullptr;
    QDoubleSpinBox* paramA_    = nullptr;
    QDoubleSpinBox* paramB_    = nullptr;
    QDoubleSpinBox* paramC_    = nullptr;
    QDoubleSpinBox* paramP_    = nullptr;
    QPushButton*  presetBtn_  = nullptr;

    // Render
    QuadricWidget* quadric_     = nullptr;
    QPushButton*   resetViewBtn_ = nullptr;
    QTextBrowser*  infoBrowser_  = nullptr;
};

}

#endif
