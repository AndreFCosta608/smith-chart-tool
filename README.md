# Smith Chart Tool

![Smith Chart Tool Logo](SmithChartLogo.png)

### RF Matching Studio (Linux / Cross-Platform)

[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](https://opensource.org/licenses/Apache-2.0)
![C++ / Qt6](https://img.shields.io/badge/Language-C%2B%2B20%20%2F%20Qt6-00599C?logo=cplusplus)
![Platform](https://img.shields.io/badge/Platform-Linux%20%7C%20Windows-brightgreen)
![NanoVNA Support](https://img.shields.io/badge/RF-NanoVNA%20.s1p-orange)
![AI Powered](https://img.shields.io/badge/AI-ONNX%20Runtime-a6e3a1?logo=onnx)

---

Uma ferramenta leve, rápida e nativa em **C++ e Qt6** para visualização na Carta de Smith, análise de impedância e síntese de redes de casamento de RF.

Feita sob medida para engenheiros de radiofrequência, radioamadores e entusiastas de hardware que buscam uma solução direta, enxuta e sem dependências pesadas de ambientes em Python ou simuladores proprietários caros.

---

## 📸 Interface do Usuário

![Smith Chart Tool Interface Principal](SmithChartMain.png)

---

## ✨ Principais Recursos

* **Carta de Smith Vetorial Avançada:**
  * Visualização gráfica de alta precisão com suporte a grade combinada **Z-Y** (Impedância e Admitância).
  * Renderização configurável de **Círculos de VSWR** e **Curvas de Q Constante**.

* **Síntese e Casamento de Impedância:**
  * **Síntese Automática de Rede L:** Calcula e aplica instantaneamente os componentes ideais para casar $Z_L$ com $50\ \Omega$.
  * Inserção manual de componentes em série ou paralelo (`+ L-Série`, `+ C-Série`, `+ L-Paralelo`, `+ C-Paralelo`).

* **🧠 Síntese Inteligente com IA (Aproximação para Componentes Comerciais):**
  * A síntese clássica devolve valores ideais de L e C que raramente existem numa prateleira. Para resolver isso, o app embarca **três modelos de IA** (treinados em PyTorch, exportados para **ONNX** e executados localmente via **ONNX Runtime C++** — sem chamadas de rede, sem nuvem, tudo roda no seu hardware) que já entregam a rede de casamento pensando no **catálogo real de componentes comerciais série E24**:
    * **Modelo A — Surrogate (Ultrarrápido):** uma MLP compacta que aproxima a topologia e os valores contínuos de L/C quase instantaneamente, útil como estimativa inicial de baixíssima latência.
    * **Modelo C — Catálogo E24 (Multi-Task):** uma rede com três cabeças de classificação (topologia, índice do indutor, índice do capacitor) que escolhe diretamente o par de componentes comerciais mais próximo do casamento ideal, já considerando os valores realmente fabricados (série E24).
    * **Modelo B — Deep RL (Agente PPO):** um agente treinado por reforço (Proximal Policy Optimization) que sintetiza a rede de casamento de forma interativa, aprendendo a política de escolha de componentes a partir de recompensa por proximidade a $50\ \Omega$.
  * Basta escolher o modelo desejado no combo **"Síntese Inteligente (IA)"** e clicar em **"⚡ Sintetizar com IA"** — a rede sugerida já aparece plotada na Carta de Smith e na lista de componentes, pronta para simulação ou ajuste manual.

* **Integração com Instrumentação:**
  * Importação nativa de arquivos Touchstone **NanoVNA (`.s1p`)** para análise de medições reais de bancada.

* **Exportação de Projetos:**
  * Gerador de relatórios e exportação de gráficos em **SVG** e **PDF** vetoriais.

---

## 🛠️ Tecnologias Utilizadas

* **Linguagem:** C++ (C++17 / C++20)
* **Framework GUI:** Qt 6 (Widgets / QPainter)
* **IA / Inferência:** ONNX Runtime (C++), modelos treinados em PyTorch e exportados para ONNX
* **Compilação:** CMake / qmake



```
smith_chart/
├── CMakeLists.txt
├── install_smith_chart.sh      # instalador para máquinas de usuário final
├── uninstall_smith_chart.sh    # desinstalador (app + ONNX Runtime, se aplicável)
├── src/
│   ├── main.cpp
│   ├── types.h
│   ├── s1p_parser.h
│   ├── s1p_parser.cpp
│   ├── smith_chart_canvas.h
│   ├── smith_chart_canvas.cpp
│   ├── main_window.h
│   ├── main_window.cpp
│   ├── ai_engine.h              # motor de inferência ONNX Runtime
│   └── ai_engine.cpp
└── ai_training/                 # scripts Python usados para gerar os modelos .onnx
    ├── synthetic_data_generator.py
    └── train_models.py
```



---

## 🚀 Como Compilar e Rodar

### Pré-requisitos (Linux/Debian/Ubuntu)

```bash
sudo apt update
sudo apt install build-essential cmake qt6-base-dev
```

**ONNX Runtime** (necessário para a Síntese Inteligente com IA) não é distribuído via `apt` — baixe o pacote oficial pré-compilado:

```bash
# Ajuste a versão/arquitetura conforme necessário (x64 ou aarch64)
wget https://github.com/microsoft/onnxruntime/releases/download/v1.29.0/onnxruntime-linux-x64-1.29.0.tgz
tar -xzf onnxruntime-linux-x64-1.29.0.tgz
cd onnxruntime-linux-x64-1.29.0

sudo cp -r include/* /usr/local/include/
sudo cp -r lib/* /usr/local/lib/
sudo ldconfig
```

### Compilando via CMake

```bash
# Clone o repositório
git clone https://github.com/AndreFCosta608/smith-chart-tool.git
cd smith-chart-tool-pro

# Crie a pasta de build e compile
mkdir build && cd build
cmake ..
make -j$(nproc)

# Execute o programa
./smith_chart_pro
```

Os três modelos `.onnx` (`surrogate_model.onnx`, `multitask_model.onnx`, `ppo_policy_model.onnx`) precisam estar no mesmo diretório do executável — gere-os rodando `ai_training/synthetic_data_generator.py` seguido de `ai_training/train_models.py`.

### Instalação em máquinas de usuário final

Para distribuir o app já compilado (sem exigir que o usuário final recompile C++ ou retreine os modelos), use o instalador incluso: ele copia o binário, os modelos `.onnx`, baixa e configura o ONNX Runtime automaticamente, e registra um atalho no menu de aplicativos.

```bash
sudo ./install_smith_chart.sh
```

Para desinstalar (removendo também o ONNX Runtime, se foi este instalador que o colocou no sistema):

```bash
sudo ./uninstall_smith_chart.sh
```

## 🤖 Agradecimentos Especiais
"Nenhum transistor foi danificado e nenhum capacitor explodiu durante a criação deste código. Um agradecimento especial ao Gemini, que atuou como copiloto de C++, revisor de matemática de RF e suporte emocional quando a álgebra de números complexos tentou fritar nossos neurônios!" 🚀✨

## 📄 Licença
Este projeto é distribuído sob a licença Apache 2.0. Veja o arquivo LICENSE para mais detalhes.

Sinta-se à vontade para fazer um fork, abrir issues e enviar pull requests. Toda contribuição é bem-vinda!
