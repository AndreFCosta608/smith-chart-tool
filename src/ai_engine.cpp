#include "ai_engine.h"
#include <iostream>
#include <cmath>
#include <algorithm>

AIEngine::AIEngine()
    : m_env(ORT_LOGGING_LEVEL_WARNING, "SmithChartAIEngine") {
    m_sessionOptions.SetIntraOpNumThreads(1);
    m_sessionOptions.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
}

std::vector<MatchingElement> AIEngine::runInference(AIMode mode, double freqHz, double rLoad, double xLoad) {
    // Normalização das variáveis de entrada para o padrão usado no treino
    double fNorm = freqHz / 3e9;
    double rNorm = rLoad / 300.0;
    double xNorm = xLoad / 300.0;

    switch (mode) {
        case AIMode::FastSurrogate:
            return runSurrogate(fNorm, rNorm, xNorm, freqHz);
        case AIMode::MultiTaskCatalog:
            return runMultiTask(fNorm, rNorm, xNorm);
        case AIMode::DeepRLSynthesis:
            return runPPO(fNorm, rNorm, xNorm, freqHz);
    }
    return {};
}

std::vector<MatchingElement> AIEngine::runSurrogate(double fNorm, double rNorm, double xNorm, [[maybe_unused]] double freqHz) {
    std::vector<MatchingElement> result;
    try {
        if (!m_surrogateSession) {
            m_surrogateSession = std::make_unique<Ort::Session>(m_env, "surrogate_model.onnx", m_sessionOptions);
        }
        Ort::Session& session = *m_surrogateSession;

        std::vector<float> inputValues = {static_cast<float>(fNorm), static_cast<float>(rNorm), static_cast<float>(xNorm)};
        std::vector<int64_t> inputShape = {1, 3};

        auto memoryInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
        Ort::Value inputTensor = Ort::Value::CreateTensor<float>(memoryInfo, inputValues.data(), inputValues.size(), inputShape.data(), inputShape.size());

        const char* inputNames[] = {"input_features"};
        const char* outputNames[] = {"predictions"};

        auto outputTensors = session.Run(Ort::RunOptions{nullptr}, inputNames, &inputTensor, 1, outputNames, 1);
        float* outData = outputTensors[0].GetTensorMutableData<float>();

        int topo = (outData[0] >= 0.0f) ? 1 : 0;
        double L_val = std::max(1e-10, static_cast<double>(outData[1]) * 1e-9);
        double C_val = std::max(1e-13, static_cast<double>(outData[2]) * 1e-12);

        if (topo == 0) {
            result.push_back({ElementType::ShuntC, C_val});
            result.push_back({ElementType::SeriesL, L_val});
        } else {
            result.push_back({ElementType::SeriesL, L_val});
            result.push_back({ElementType::ShuntC, C_val});
        }
    } catch (const std::exception& e) {
        std::cerr << "[AI Engine Error] Surrogate: " << e.what() << std::endl;
    }
    return result;
}

std::vector<MatchingElement> AIEngine::runMultiTask(double fNorm, double rNorm, double xNorm) {
    std::vector<MatchingElement> result;
    try {
        if (!m_multiTaskSession) {
            m_multiTaskSession = std::make_unique<Ort::Session>(m_env, "multitask_model.onnx", m_sessionOptions);
        }
        Ort::Session& session = *m_multiTaskSession;

        std::vector<float> inputValues = {static_cast<float>(fNorm), static_cast<float>(rNorm), static_cast<float>(xNorm)};
        std::vector<int64_t> inputShape = {1, 3};

        auto memoryInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
        Ort::Value inputTensor = Ort::Value::CreateTensor<float>(memoryInfo, inputValues.data(), inputValues.size(), inputShape.data(), inputShape.size());

        const char* inputNames[] = {"input_features"};
        const char* outputNames[] = {"topology_logits", "L_catalog_logits", "C_catalog_logits"};

        auto outputTensors = session.Run(Ort::RunOptions{nullptr}, inputNames, &inputTensor, 1, outputNames, 3);

        float* topoData = outputTensors[0].GetTensorMutableData<float>();
        float* LData = outputTensors[1].GetTensorMutableData<float>();
        float* CData = outputTensors[2].GetTensorMutableData<float>();

        int bestTopo = (topoData[1] > topoData[0]) ? 1 : 0;

        auto& L_catalog = getE24Inductors();
        auto& C_catalog = getE24Capacitors();

        int bestLIdx = std::distance(LData, std::max_element(LData, LData + L_catalog.size()));
        int bestCIdx = std::distance(CData, std::max_element(CData, CData + C_catalog.size()));

        double L_val = L_catalog[bestLIdx];
        double C_val = C_catalog[bestCIdx];

        if (bestTopo == 0) {
            result.push_back({ElementType::ShuntC, C_val});
            result.push_back({ElementType::SeriesL, L_val});
        } else {
            result.push_back({ElementType::SeriesL, L_val});
            result.push_back({ElementType::ShuntC, C_val});
        }
    } catch (const std::exception& e) {
        std::cerr << "[AI Engine Error] MultiTask: " << e.what() << std::endl;
    }
    return result;
}

std::vector<MatchingElement> AIEngine::runPPO(double fNorm, double rNorm, double xNorm, [[maybe_unused]] double freqHz) {
    std::vector<MatchingElement> result;
    try {
        if (!m_ppoSession) {
            m_ppoSession = std::make_unique<Ort::Session>(m_env, "ppo_policy_model.onnx", m_sessionOptions);
        }
        Ort::Session& session = *m_ppoSession;

        std::vector<float> inputValues = {static_cast<float>(fNorm), static_cast<float>(rNorm), static_cast<float>(xNorm)};
        std::vector<int64_t> inputShape = {1, 3};

        auto memoryInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
        Ort::Value inputTensor = Ort::Value::CreateTensor<float>(memoryInfo, inputValues.data(), inputValues.size(), inputShape.data(), inputShape.size());

        const char* inputNames[] = {"state"};
        const char* outputNames[] = {"action_probabilities"};

        auto outputTensors = session.Run(Ort::RunOptions{nullptr}, inputNames, &inputTensor, 1, outputNames, 1);
        float* probs = outputTensors[0].GetTensorMutableData<float>();

        int action = std::distance(probs, std::max_element(probs, probs + 3));

        // Mapeia ações do Agente RL para componentes
        if (action == 0) {
            result.push_back({ElementType::SeriesL, 12e-9});
        } else if (action == 1) {
            result.push_back({ElementType::ShuntC, 4.7e-12});
        } else {
            result.push_back({ElementType::SeriesC, 10e-12});
            result.push_back({ElementType::ShuntL, 15e-9});
        }
    } catch (const std::exception& e) {
        std::cerr << "[AI Engine Error] PPO: " << e.what() << std::endl;
    }
    return result;
}
