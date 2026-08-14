#ifndef TYPES_H
#define TYPES_H

#include <complex>
#include <numbers>
#include <QString>

enum class ElementType {
    SeriesL,
    SeriesC,
    ShuntL,
    ShuntC
};

struct MatchingElement {
    ElementType type;
    double value; // Farads (F) ou Henries (H)

    QString name() const {
        switch (type) {
            case ElementType::SeriesL: return "L-Série";
            case ElementType::SeriesC: return "C-Série";
            case ElementType::ShuntL:  return "L-Paralelo";
            case ElementType::ShuntC:  return "C-Paralelo";
        }
        return "";
    }

    std::complex<double> apply(std::complex<double> zIn, double freqHz, double z0 = 50.0) const {
        double omega = 2.0 * std::numbers::pi * freqHz;
        if (omega <= 0.0 || value <= 0.0) return zIn;

        switch (type) {
            case ElementType::SeriesL: {
                double x = (omega * value) / z0;
                return zIn + std::complex<double>(0.0, x);
            }
            case ElementType::SeriesC: {
                double x = -1.0 / (omega * value * z0);
                return zIn + std::complex<double>(0.0, x);
            }
            case ElementType::ShuntC: {
                std::complex<double> yIn = 1.0 / zIn;
                double b = omega * value * z0;
                std::complex<double> yOut = yIn + std::complex<double>(0.0, b);
                return 1.0 / yOut;
            }
            case ElementType::ShuntL: {
                std::complex<double> yIn = 1.0 / zIn;
                double b = -z0 / (omega * value);
                std::complex<double> yOut = yIn + std::complex<double>(0.0, b);
                return 1.0 / yOut;
            }
        }
        return zIn;
    }
};

struct S1PPoint {
    double freqHz;
    std::complex<double> zNorm;
};

#endif // TYPES_H
