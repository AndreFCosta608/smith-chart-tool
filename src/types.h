#ifndef TYPES_H
#define TYPES_H

#include <complex>
#include <numbers>
#include <QString>
#include <vector>

enum class ElementType {
    SeriesL,
    SeriesC,
    ShuntL,
    ShuntC
};

enum class AIMode {
    FastSurrogate,     // Modelo A: Baixa Latência / Aproximação Contínua
    MultiTaskCatalog,  // Modelo C: Seleção Direta de Componentes E24
    DeepRLSynthesis    // Modelo B: Síntese Interativa via PPO
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

// Tabela de apoio para desmapeamento das classes comerciais E24 no C++
inline const std::vector<double>& getE24Inductors() {
    static const std::vector<double> L_E24 = []() {
        std::vector<double> base = {1.0, 1.1, 1.2, 1.3, 1.5, 1.6, 1.8, 2.0, 2.2, 2.4, 2.7, 3.0, 3.3, 3.6, 3.9, 4.3, 4.7, 5.1, 5.6, 6.2, 6.8, 7.5, 8.2, 9.1};
        std::vector<double> vals;
        for (double exp : {1e-9, 10e-9, 100e-9}) {
            for (double b : base) vals.push_back(b * exp);
        }
        return vals;
    }();
    return L_E24;
}

inline const std::vector<double>& getE24Capacitors() {
    static const std::vector<double> C_E24 = []() {
        std::vector<double> base = {1.0, 1.1, 1.2, 1.3, 1.5, 1.6, 1.8, 2.0, 2.2, 2.4, 2.7, 3.0, 3.3, 3.6, 3.9, 4.3, 4.7, 5.1, 5.6, 6.2, 6.8, 7.5, 8.2, 9.1};
        std::vector<double> vals;
        for (double exp : {1e-12, 10e-12, 100e-12}) {
            for (double b : base) vals.push_back(b * exp);
        }
        return vals;
    }();
    return C_E24;
}

#endif // TYPES_H
