#ifndef AI_ENGINE_H
#define AI_ENGINE_H

#include <onnxruntime_cxx_api.h>
#include <vector>
#include <string>
#include <memory>
#include "types.h"

class AIEngine {
public:
    AIEngine();
    ~AIEngine() = default;

    // ATENÇÃO: rLoad e xLoad devem ser passados em OHMS REAIS (não normalizados
    // por Z0). A normalização interna (rNorm = rLoad / 300.0 etc.) replica
    // exatamente a normalização usada no treino (synthetic_data_generator.py,
    // onde R_load ∈ [5, 300] Ω e X_load ∈ [-300, 300] Ω). O chamador é
    // responsável por converter valores normalizados de Smith Chart (r, x
    // adimensionais, Z0=50) para ohms antes de chamar esta função.
    std::vector<MatchingElement> runInference(
        AIMode mode,
        double freqHz,
        double rLoad,
        double xLoad
    );

private:
    Ort::Env m_env;
    Ort::SessionOptions m_sessionOptions;

    // Sessões cacheadas: carregar um modelo .onnx do disco é caro, então cada
    // sessão é criada preguiçosamente (lazy) na primeira inferência daquele
    // modo e reaproveitada nas chamadas seguintes.
    std::unique_ptr<Ort::Session> m_surrogateSession;
    std::unique_ptr<Ort::Session> m_multiTaskSession;
    std::unique_ptr<Ort::Session> m_ppoSession;

    std::vector<MatchingElement> runSurrogate(double fNorm, double rNorm, double xNorm, double freqHz);
    std::vector<MatchingElement> runMultiTask(double fNorm, double rNorm, double xNorm);
    std::vector<MatchingElement> runPPO(double fNorm, double rNorm, double xNorm, double freqHz);
};

#endif // AI_ENGINE_H
