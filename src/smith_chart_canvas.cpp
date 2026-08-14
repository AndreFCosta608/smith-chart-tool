#include "smith_chart_canvas.h"
#include <QPainter>
#include <QPainterPath>
#include <cmath>
#include <algorithm>

SmithChartCanvas::SmithChartCanvas(QWidget* parent) : QWidget(parent) {
    setMinimumSize(500, 500);
}

void SmithChartCanvas::setLoadImpedance(double r, double x) {
    m_rLoad = r;
    m_xLoad = x;
    update();
}

void SmithChartCanvas::setFrequency(double freqHz) {
    m_freqHz = freqHz;
    update();
}

void SmithChartCanvas::setElements(const std::vector<MatchingElement>& elements) {
    m_elements = elements;
    update();
}

void SmithChartCanvas::setS1PData(const std::vector<S1PPoint>& points) {
    m_s1pPoints = points;
    update();
}

std::complex<double> SmithChartCanvas::finalImpedance() const {
    std::complex<double> zCurr(m_rLoad, m_xLoad);
    for (const auto& elem : m_elements) {
        zCurr = elem.apply(zCurr, m_freqHz);
    }
    return zCurr;
}

QPointF SmithChartCanvas::gammaToPixel(std::complex<double> gamma, double radiusPx, QPointF center) const {
    double px = center.x() + gamma.real() * radiusPx;
    double py = center.y() - gamma.imag() * radiusPx;
    return QPointF(px, py);
}

void SmithChartCanvas::paintEvent(QPaintEvent* /*event*/) {
    QPainter painter(this);
    double margin = 25.0;
    double side = std::min(width(), height()) - 2.0 * margin;
    double radiusPx = side / 2.0;
    QPointF center(width() / 2.0, height() / 2.0);

    renderChart(painter, center, radiusPx);
}

void SmithChartCanvas::renderChart(QPainter& painter, QPointF center, double radiusPx) {
    painter.fillRect(rect(), QColor("#11111b"));
    painter.setRenderHint(QPainter::Antialiasing, true);

    // Círculo Externo
    painter.setPen(QPen(QColor("#89b4fa"), 2.0));
    painter.drawEllipse(center, radiusPx, radiusPx);

    // Eixo Real
    painter.drawLine(QPointF(center.x() - radiusPx, center.y()), QPointF(center.x() + radiusPx, center.y()));

    // 1. Grade de Impedância Z
    painter.setPen(QPen(QColor("#45475a"), 1.0, Qt::DashLine));
    std::vector<double> r_values = {0.2, 0.5, 1.0, 2.0, 5.0};
    for (double r : r_values) {
        double center_x_norm = r / (r + 1.0);
        double rad_norm = 1.0 / (r + 1.0);
        QPointF cPx = gammaToPixel({center_x_norm, 0.0}, radiusPx, center);
        painter.drawEllipse(cPx, rad_norm * radiusPx, rad_norm * radiusPx);
    }

    std::vector<double> x_values = {0.2, 0.5, 1.0, 2.0, 5.0, -0.2, -0.5, -1.0, -2.0, -5.0};
    QPainterPath clipPath;
    clipPath.addEllipse(center, radiusPx, radiusPx);
    painter.save();
    painter.setClipPath(clipPath);

    for (double x_val : x_values) {
        if (x_val == 0.0) continue;
        double center_y_norm = 1.0 / x_val;
        double rad_norm = std::abs(1.0 / x_val);
        QPointF cPx = gammaToPixel({1.0, center_y_norm}, radiusPx, center);
        painter.drawEllipse(cPx, rad_norm * radiusPx, rad_norm * radiusPx);
    }
    painter.restore();

    // 2. Grade de Admitância Y
    if (m_showZYGrid) {
        painter.save();
        painter.setClipPath(clipPath);
        painter.setPen(QPen(QColor("#f38ba8"), 0.8, Qt::DotLine));

        for (double g : r_values) {
            double center_x_norm = -g / (g + 1.0);
            double rad_norm = 1.0 / (g + 1.0);
            QPointF cPx = gammaToPixel({center_x_norm, 0.0}, radiusPx, center);
            painter.drawEllipse(cPx, rad_norm * radiusPx, rad_norm * radiusPx);
        }
        for (double b_val : x_values) {
            if (b_val == 0.0) continue;
            double center_y_norm = 1.0 / b_val;
            double rad_norm = std::abs(1.0 / b_val);
            QPointF cPx = gammaToPixel({-1.0, center_y_norm}, radiusPx, center);
            painter.drawEllipse(cPx, rad_norm * radiusPx, rad_norm * radiusPx);
        }
        painter.restore();
    }

    // 3. VSWR Constante
    if (m_showVSWR && m_vswrVal > 1.0) {
        double gamma_vswr = (m_vswrVal - 1.0) / (m_vswrVal + 1.0);
        painter.setPen(QPen(QColor("#f9e2af"), 1.5, Qt::DashDotLine));
        painter.drawEllipse(center, gamma_vswr * radiusPx, gamma_vswr * radiusPx);
    }

    // 4. Curvas de Q Constante
    if (m_showQ && m_qVal > 0.0) {
        painter.save();
        painter.setClipPath(clipPath);
        painter.setPen(QPen(QColor("#cba6f7"), 1.2, Qt::DashDotDotLine));

        QPainterPath qUpper, qLower;
        bool firstU = true, firstL = true;
        for (double r = 0.01; r <= 10.0; r += 0.02) {
            double xUpper = r / m_qVal;
            std::complex<double> zU(r, xUpper);
            std::complex<double> gU = (zU - 1.0) / (zU + 1.0);
            QPointF pU = gammaToPixel(gU, radiusPx, center);

            if (firstU) { qUpper.moveTo(pU); firstU = false; }
            else { qUpper.lineTo(pU); }

            double xLower = -r / m_qVal;
            std::complex<double> zL(r, xLower);
            std::complex<double> gL = (zL - 1.0) / (zL + 1.0);
            QPointF pL = gammaToPixel(gL, radiusPx, center);

            if (firstL) { qLower.moveTo(pL); firstL = false; }
            else { qLower.lineTo(pL); }
        }
        painter.drawPath(qUpper);
        painter.drawPath(qLower);
        painter.restore();
    }

    // 5. Curva NanoVNA (.s1p)
    if (!m_s1pPoints.empty()) {
        QPainterPath s1pPath;
        bool first = true;
        for (const auto& pt : m_s1pPoints) {
            std::complex<double> gamma = (pt.zNorm - 1.0) / (pt.zNorm + 1.0);
            QPointF px = gammaToPixel(gamma, radiusPx, center);
            if (first) { s1pPath.moveTo(px); first = false; }
            else { s1pPath.lineTo(px); }
        }
        painter.setPen(QPen(QColor("#fab387"), 2.0));
        painter.drawPath(s1pPath);
    }

    // 6. Trajetória de Casamento
    std::complex<double> zCurr(m_rLoad, m_xLoad);
    std::complex<double> gCurr = (zCurr - 1.0) / (zCurr + 1.0);
    QPointF ptPrev = gammaToPixel(gCurr, radiusPx, center);

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor("#f9e2af"));
    painter.drawEllipse(ptPrev, 5.0, 5.0);

    for (const auto& elem : m_elements) {
        std::complex<double> zNext = elem.apply(zCurr, m_freqHz);
        std::complex<double> gNext = (zNext - 1.0) / (zNext + 1.0);
        QPointF ptNext = gammaToPixel(gNext, radiusPx, center);

        painter.setPen(QPen(QColor("#a6e3a1"), 2.5));
        painter.drawLine(ptPrev, ptNext);

        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor("#89b4fa"));
        painter.drawEllipse(ptNext, 4.0, 4.0);

        zCurr = zNext;
        ptPrev = ptNext;
    }

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor("#a6e3a1"));
    painter.drawEllipse(ptPrev, 7.0, 7.0);

    // Ponto Central (50 Ohms)
    painter.setPen(QPen(QColor("#f38ba8"), 2.0));
    int cs = 6;
    painter.drawLine(center.x() - cs, center.y() - cs, center.x() + cs, center.y() + cs);
    painter.drawLine(center.x() - cs, center.y() + cs, center.x() + cs, center.y() - cs);
}
