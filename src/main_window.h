#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QMainWindow>
#include <QLineEdit>
#include <QPushButton>
#include <QListWidget>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QLabel>
#include "smith_chart_canvas.h"
#include "types.h"
#include "ai_engine.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void updateLoadImpedance();
    void updateFrequency();
    void addElement(ElementType type);
    void removeSelectedElement();
    void synthesizeLNetwork();
    void synthesizeWithAI(); // NOVO SLOT
    void loadS1PFile();
    void exportSVG();
    void exportPDF();
    void updateToggles();

private:
    SmithChartCanvas* m_canvas;
    AIEngine m_aiEngine; // Instância do motor de inferência

    QLineEdit* m_rInput;
    QLineEdit* m_xInput;
    QDoubleSpinBox* m_freqSpin;

    QListWidget* m_elementsList;
    std::vector<MatchingElement> m_elements;

    QComboBox* m_aiModeCombo; // NOVO
    QPushButton* m_btnRunAI;   // NOVO

    QCheckBox* m_chkZYGrid;
    QCheckBox* m_chkVSWR;
    QDoubleSpinBox* m_vswrSpin;
    QCheckBox* m_chkQ;
    QDoubleSpinBox* m_qSpin;

    QLabel* m_lblFinalResult;

    void refreshElementsList();
};

#endif // MAIN_WINDOW_H
