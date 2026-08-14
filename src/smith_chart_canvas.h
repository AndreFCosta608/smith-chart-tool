#ifndef SMITH_CHART_CANVAS_H
#define SMITH_CHART_CANVAS_H

#include <QWidget>
#include <complex>
#include <vector>
#include "types.h"

class SmithChartCanvas : public QWidget {
    Q_OBJECT

public:
    explicit SmithChartCanvas(QWidget* parent = nullptr);

    void setLoadImpedance(double r, double x);
    void setFrequency(double freqHz);
    void setElements(const std::vector<MatchingElement>& elements);
    void setS1PData(const std::vector<S1PPoint>& points);

    void setShowZYGrid(bool show) { m_showZYGrid = show; update(); }
    void setShowVSWR(bool show, double val = 1.5) { m_showVSWR = show; m_vswrVal = val; update(); }
    void setShowQ(bool show, double qVal = 1.0) { m_showQ = show; m_qVal = qVal; update(); }

    std::complex<double> finalImpedance() const;
    void renderChart(QPainter& painter, QPointF center, double radiusPx);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    double m_rLoad{0.5};
    double m_xLoad{1.0};
    double m_freqHz{915e6};
    double m_vswrVal{1.5};
    double m_qVal{1.0};

    bool m_showZYGrid{true};
    bool m_showVSWR{true};
    bool m_showQ{true};

    std::vector<MatchingElement> m_elements;
    std::vector<S1PPoint> m_s1pPoints;

    QPointF gammaToPixel(std::complex<double> gamma, double radiusPx, QPointF center) const;
};

#endif // SMITH_CHART_CANVAS_H
