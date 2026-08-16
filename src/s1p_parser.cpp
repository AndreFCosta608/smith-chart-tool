#include "s1p_parser.h"
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <cmath>
#include <numbers>

bool S1PParser::loadFile(const QString& filePath, std::vector<S1PPoint>& outPoints, [[maybe_unused]] double z0) {
    // z0 é aceito para compatibilidade de API, mas não é usado: zNorm já é
    // calculado normalizado (Z0=1) a partir de gamma, e todo o resto do app
    // (SmithChartCanvas, MatchingElement::apply) assume essa mesma convenção.
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return false;

    outPoints.clear();
    QTextStream in(&file);

    enum Format { RI, MA, DB };
    Format fmt = MA;
    double freqMult = 1.0;

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith("!")) continue;

        if (line.startsWith("#")) {
            QString upper = line.toUpper();
            if (upper.contains("GHZ")) freqMult = 1e9;
            else if (upper.contains("MHZ")) freqMult = 1e6;
            else if (upper.contains("KHZ")) freqMult = 1e3;

            if (upper.contains(" RI")) fmt = RI;
            else if (upper.contains(" MA")) fmt = MA;
            else if (upper.contains(" DB")) fmt = DB;
            continue;
        }

        QStringList parts = line.split(QRegularExpression("\\s+"));
        if (parts.size() < 3) continue;

        bool ok1, ok2, ok3;
        double f = parts[0].toDouble(&ok1) * freqMult;
        double v1 = parts[1].toDouble(&ok2);
        double v2 = parts[2].toDouble(&ok3);

        if (!ok1 || !ok2 || !ok3) continue;

        std::complex<double> gamma;
        if (fmt == RI) {
            gamma = std::complex<double>(v1, v2);
        } else if (fmt == MA) {
            double rad = v2 * std::numbers::pi / 180.0;
            gamma = std::polar(v1, rad);
        } else if (fmt == DB) {
            double mag = std::pow(10.0, v1 / 20.0);
            double rad = v2 * std::numbers::pi / 180.0;
            gamma = std::polar(mag, rad);
        }

        if (std::abs(gamma - 1.0) < 1e-6) gamma = std::complex<double>(0.9999, 0.0);

        std::complex<double> z = (1.0 + gamma) / (1.0 - gamma);
        outPoints.push_back({f, z});
    }

    return !outPoints.empty();
}
