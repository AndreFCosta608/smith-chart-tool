#include "main_window.h"
#include "s1p_parser.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QSvgGenerator>
#include <QPrinter>
#include <QPainter>
#include <cmath>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("Smith Chart Tool - RF Matching Studio (Linux)");
    resize(1150, 750);

    setStyleSheet("QMainWindow { background-color: #11111b; }"
                  "QGroupBox { color: #89b4fa; font-weight: bold; border: 1px solid #45475a; border-radius: 6px; margin-top: 10px; padding-top: 10px; }"
                  "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 5px; }"
                  "QLabel { color: #cdd6f4; font-size: 12px; }"
                  "QLineEdit, QDoubleSpinBox { background-color: #313244; color: #cdd6f4; padding: 5px; border-radius: 4px; border: 1px solid #45475a; }"
                  "QPushButton { background-color: #313244; color: #cdd6f4; font-weight: bold; padding: 6px; border-radius: 4px; border: 1px solid #45475a; }"
                  "QPushButton:hover { background-color: #45475a; color: #89b4fa; }"
                  "QListWidget { background-color: #1e1e2e; color: #a6e3a1; border: 1px solid #45475a; border-radius: 4px; }"
                  "QCheckBox { color: #cdd6f4; }");

    QWidget* central = new QWidget(this);
    QHBoxLayout* mainLayout = new QHBoxLayout(central);

    m_canvas = new SmithChartCanvas(this);
    mainLayout->addWidget(m_canvas, 3);

    QWidget* sidebar = new QWidget(this);
    QVBoxLayout* sideLayout = new QVBoxLayout(sidebar);

    // 1. Carga
    QGroupBox* grpLoad = new QGroupBox("1. Carga Z_L e Frequência", this);
    QFormLayout* fLoad = new QFormLayout(grpLoad);

    m_rInput = new QLineEdit("0.5", this);
    m_xInput = new QLineEdit("1.0", this);
    m_freqSpin = new QDoubleSpinBox(this);
    m_freqSpin->setRange(0.1, 10000.0);
    m_freqSpin->setValue(915.0);
    m_freqSpin->setSuffix(" MHz");

    fLoad->addRow("Resistência (r):", m_rInput);
    fLoad->addRow("Reatância (x):", m_xInput);
    fLoad->addRow("Frequência:", m_freqSpin);
    sideLayout->addWidget(grpLoad);

    connect(m_rInput, &QLineEdit::textChanged, this, &MainWindow::updateLoadImpedance);
    connect(m_xInput, &QLineEdit::textChanged, this, &MainWindow::updateLoadImpedance);
    connect(m_freqSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MainWindow::updateFrequency);

    // 2. Componentes
    QGroupBox* grpElements = new QGroupBox("2. Rede de Casamento", this);
    QVBoxLayout* vElements = new QVBoxLayout(grpElements);

    QHBoxLayout* btnGrid = new QHBoxLayout();
    QPushButton* btnAddLS = new QPushButton("+ L-Série", this);
    QPushButton* btnAddCS = new QPushButton("+ C-Série", this);
    QPushButton* btnAddLP = new QPushButton("+ L-Paralelo", this);
    QPushButton* btnAddCP = new QPushButton("+ C-Paralelo", this);

    btnGrid->addWidget(btnAddLS); btnGrid->addWidget(btnAddCS);
    vElements->addLayout(btnGrid);
    QHBoxLayout* btnGrid2 = new QHBoxLayout();
    btnGrid2->addWidget(btnAddLP); btnGrid2->addWidget(btnAddCP);
    vElements->addLayout(btnGrid2);

    m_elementsList = new QListWidget(this);
    vElements->addWidget(m_elementsList);

    QHBoxLayout* elemActions = new QHBoxLayout();
    QPushButton* btnRemove = new QPushButton("Remover", this);
    QPushButton* btnAutoL = new QPushButton("Síntese Rede L Auto", this);
    btnAutoL->setStyleSheet("background-color: #89b4fa; color: #11111b; font-weight: bold;");

    elemActions->addWidget(btnRemove);
    elemActions->addWidget(btnAutoL);
    vElements->addLayout(elemActions);

    sideLayout->addWidget(grpElements);

    connect(btnAddLS, &QPushButton::clicked, [this](){ addElement(ElementType::SeriesL); });
    connect(btnAddCS, &QPushButton::clicked, [this](){ addElement(ElementType::SeriesC); });
    connect(btnAddLP, &QPushButton::clicked, [this](){ addElement(ElementType::ShuntL); });
    connect(btnAddCP, &QPushButton::clicked, [this](){ addElement(ElementType::ShuntC); });
    connect(btnRemove, &QPushButton::clicked, this, &MainWindow::removeSelectedElement);
    connect(btnAutoL, &QPushButton::clicked, this, &MainWindow::synthesizeLNetwork);

    // 3. Visualização
    QGroupBox* grpOverlay = new QGroupBox("3. Visualização / Camadas", this);
    QVBoxLayout* vOverlay = new QVBoxLayout(grpOverlay);

    m_chkZYGrid = new QCheckBox("Mostrar Grade Z-Y Combinada", this);
    m_chkZYGrid->setChecked(true);

    QHBoxLayout* hVSWR = new QHBoxLayout();
    m_chkVSWR = new QCheckBox("Círculo VSWR:", this);
    m_chkVSWR->setChecked(true);
    m_vswrSpin = new QDoubleSpinBox(this);
    m_vswrSpin->setRange(1.01, 10.0);
    m_vswrSpin->setValue(1.5);
    hVSWR->addWidget(m_chkVSWR); hVSWR->addWidget(m_vswrSpin);

    QHBoxLayout* hQ = new QHBoxLayout();
    m_chkQ = new QCheckBox("Curvas Q Constante:", this);
    m_chkQ->setChecked(true);
    m_qSpin = new QDoubleSpinBox(this);
    m_qSpin->setRange(0.1, 50.0);
    m_qSpin->setValue(1.0);
    hQ->addWidget(m_chkQ); hQ->addWidget(m_qSpin);

    vOverlay->addWidget(m_chkZYGrid);
    vOverlay->addLayout(hVSWR);
    vOverlay->addLayout(hQ);
    sideLayout->addWidget(grpOverlay);

    connect(m_chkZYGrid, &QCheckBox::toggled, this, &MainWindow::updateToggles);
    connect(m_chkVSWR, &QCheckBox::toggled, this, &MainWindow::updateToggles);
    connect(m_vswrSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MainWindow::updateToggles);
    connect(m_chkQ, &QCheckBox::toggled, this, &MainWindow::updateToggles);
    connect(m_qSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MainWindow::updateToggles);

    // 4. Arquivos
    QHBoxLayout* hFiles = new QHBoxLayout();
    QPushButton* btnS1P = new QPushButton("Abrir NanoVNA (.s1p)", this);
    QPushButton* btnSVG = new QPushButton("SVG", this);
    QPushButton* btnPDF = new QPushButton("PDF", this);

    hFiles->addWidget(btnS1P);
    hFiles->addWidget(btnSVG);
    hFiles->addWidget(btnPDF);
    sideLayout->addLayout(hFiles);

    connect(btnS1P, &QPushButton::clicked, this, &MainWindow::loadS1PFile);
    connect(btnSVG, &QPushButton::clicked, this, &MainWindow::exportSVG);
    connect(btnPDF, &QPushButton::clicked, this, &MainWindow::exportPDF);

    m_lblFinalResult = new QLabel("Z Final: -", this);
    m_lblFinalResult->setStyleSheet("color: #a6e3a1; font-weight: bold; font-size: 13px;");
    sideLayout->addWidget(m_lblFinalResult);

    mainLayout->addWidget(sidebar, 1);
    setCentralWidget(central);

    updateLoadImpedance();
    updateFrequency();
}

void MainWindow::updateLoadImpedance() {
    bool okR, okX;
    double r = m_rInput->text().toDouble(&okR);
    double x = m_xInput->text().toDouble(&okX);

    if (okR && okX) {
        m_canvas->setLoadImpedance(r, x);
        refreshElementsList();
    }
}

void MainWindow::updateFrequency() {
    double freqHz = m_freqSpin->value() * 1e6;
    m_canvas->setFrequency(freqHz);
    refreshElementsList();
}

void MainWindow::addElement(ElementType type) {
    double defValue = (type == ElementType::SeriesL || type == ElementType::ShuntL) ? 10e-9 : 10e-12;
    m_elements.push_back({type, defValue});
    refreshElementsList();
}

void MainWindow::removeSelectedElement() {
    int row = m_elementsList->currentRow();
    if (row >= 0 && row < static_cast<int>(m_elements.size())) {
        m_elements.erase(m_elements.begin() + row);
        refreshElementsList();
    }
}

void MainWindow::refreshElementsList() {
    m_elementsList->clear();

    for (size_t i = 0; i < m_elements.size(); ++i) {
        const auto& elem = m_elements[i];
        QString valStr;
        if (elem.type == ElementType::SeriesL || elem.type == ElementType::ShuntL) {
            valStr = QString("%1 nH").arg(elem.value * 1e9, 0, 'f', 2);
        } else {
            valStr = QString("%1 pF").arg(elem.value * 1e12, 0, 'f', 2);
        }
        m_elementsList->addItem(QString("%1. %2 (%3)").arg(i + 1).arg(elem.name()).arg(valStr));
    }

    m_canvas->setElements(m_elements);

    std::complex<double> zFin = m_canvas->finalImpedance();
    m_lblFinalResult->setText(QString("Z Final = %1 %2 j(%3)")
                                .arg(zFin.real(), 0, 'f', 3)
                                .arg(zFin.imag() >= 0 ? "+" : "-")
                                .arg(std::abs(zFin.imag()), 0, 'f', 3));
}

void MainWindow::synthesizeLNetwork() {
    bool okR, okX;
    double r = m_rInput->text().toDouble(&okR);
    double x = m_xInput->text().toDouble(&okX);
    double freqHz = m_freqSpin->value() * 1e6;
    double omega = 2.0 * std::numbers::pi * freqHz;
    double z0 = 50.0;

    if (!okR || !okX || r <= 0) {
        QMessageBox::warning(this, "Erro", "Impedância de carga Z_L inválida.");
        return;
    }

    m_elements.clear();

    if (r < 1.0) {
        double b = std::sqrt((1.0 - r) / r);
        double x_comp = b * r - x;

        if (x_comp >= 0) {
            m_elements.push_back({ElementType::SeriesL, (x_comp * z0) / omega});
        } else {
            m_elements.push_back({ElementType::SeriesC, 1.0 / (std::abs(x_comp) * z0 * omega)});
        }
        m_elements.push_back({ElementType::ShuntC, b / (z0 * omega)});
    } else {
        double b = (x + std::sqrt(r * (x*x + r*r - r))) / (x*x + r*r);
        double x_comp = 1.0 / b - x;

        m_elements.push_back({ElementType::ShuntC, b / (z0 * omega)});
        if (x_comp >= 0) {
            m_elements.push_back({ElementType::SeriesL, (x_comp * z0) / omega});
        } else {
            m_elements.push_back({ElementType::SeriesC, 1.0 / (std::abs(x_comp) * z0 * omega)});
        }
    }

    refreshElementsList();
    QMessageBox::information(this, "Síntese", "Rede de Casamento em L calculada e aplicada!");
}

void MainWindow::loadS1PFile() {
    QString fileName = QFileDialog::getOpenFileName(this, "Abrir NanoVNA Touchstone", "", "Touchstone (*.s1p)");
    if (fileName.isEmpty()) return;

    std::vector<S1PPoint> points;
    if (S1PParser::loadFile(fileName, points)) {
        m_canvas->setS1PData(points);
        QMessageBox::information(this, "Sucesso", QString("%1 pontos do NanoVNA carregados!").arg(points.size()));
    } else {
        QMessageBox::critical(this, "Erro", "Falha ao ler arquivo .s1p");
    }
}

void MainWindow::exportSVG() {
    QString fileName = QFileDialog::getSaveFileName(this, "Exportar SVG", "smith_chart.svg", "SVG (*.svg)");
    if (fileName.isEmpty()) return;

    QSvgGenerator generator;
    generator.setFileName(fileName);
    generator.setSize(QSize(1000, 1000));
    generator.setViewBox(QRect(0, 0, 1000, 1000));

    QPainter painter(&generator);
    m_canvas->renderChart(painter, QPointF(500, 500), 450.0);
    QMessageBox::information(this, "OK", "Exportado em SVG!");
}

void MainWindow::exportPDF() {
    QString fileName = QFileDialog::getSaveFileName(this, "Exportar PDF", "smith_chart.pdf", "PDF (*.pdf)");
    if (fileName.isEmpty()) return;

    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(fileName);

    QPainter painter(&printer);
    QRect pageRect = printer.pageRect(QPrinter::DevicePixel).toRect();
    double side = std::min(pageRect.width(), pageRect.height()) - 100;
    m_canvas->renderChart(painter, pageRect.center(), side / 2.0);
    QMessageBox::information(this, "OK", "Exportado em PDF!");
}

void MainWindow::updateToggles() {
    m_canvas->setShowZYGrid(m_chkZYGrid->isChecked());
    m_canvas->setShowVSWR(m_chkVSWR->isChecked(), m_vswrSpin->value());
    m_canvas->setShowQ(m_chkQ->isChecked(), m_qSpin->value());
}
