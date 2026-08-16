#!/bin/bash
#
# install_smith_chart.sh
#
# Instalador do Smith Chart Tool - RF Matching Studio (IA Edition)
#
# Uso:
#   sudo ./install_smith_chart.sh                     # instala usando os
#                                                       # arquivos da mesma
#                                                       # pasta deste script
#   sudo ./install_smith_chart.sh --source DIR         # instala a partir de DIR
#   sudo ./install_smith_chart.sh --ort-version X.Y.Z  # versão específica do
#                                                       # ONNX Runtime (padrão
#                                                       # abaixo em ORT_VERSION)
#   sudo ./install_smith_chart.sh --force-ort          # baixa/reinstala o ONNX
#                                                       # Runtime mesmo se já
#                                                       # houver uma cópia no
#                                                       # sistema
#
# Para desinstalar, use o script separado: uninstall_smith_chart.sh
# (ele lê o manifesto gravado por este instalador em
#  /opt/smith_chart_pro/.install_manifest para saber exatamente o que foi
#  instalado e remover com precisão, inclusive o ONNX Runtime se for o caso).
#
# O QUE ESTE SCRIPT ESPERA ENCONTRAR (na pasta de origem):
#   smith_chart_pro           <- binário compilado (obrigatório)
#   surrogate_model.onnx      <- modelo A (obrigatório)
#   multitask_model.onnx      <- modelo C (obrigatório)
#   ppo_policy_model.onnx     <- modelo B (obrigatório)
#   *.onnx.data                <- pesos externos dos modelos acima, se o
#                                 exportador do PyTorch tiver gerado esse
#                                 formato (opcional -- só existe se o modelo
#                                 for grande o suficiente para o ONNX usar
#                                 "external data"; copiado automaticamente
#                                 se presente, sem ele a IA não roda)
#
# O QUE ESTE SCRIPT FAZ COM O ONNX RUNTIME:
#   libonnxruntime não vem empacotado via apt. Este instalador baixa o
#   pacote oficial pré-compilado direto do GitHub (mesmo procedimento
#   manual: baixar o .tgz, extrair, copiar para /usr/local/lib e
#   /usr/local/include, rodar ldconfig), e ao final APAGA tanto o .tgz
#   baixado quanto a pasta extraída -- fica só o resultado instalado em
#   /usr/local, nada de lixo temporário.
#
set -euo pipefail

# ----------------------------------------------------------------------------
# Configuração
# ----------------------------------------------------------------------------
APP_NAME="smith_chart_pro"
APP_DISPLAY_NAME="Smith Chart Tool - RF Matching Studio (IA Edition)"
INSTALL_DIR="/opt/${APP_NAME}"
BIN_LINK="/usr/local/bin/${APP_NAME}"
DESKTOP_FILE="/usr/share/applications/${APP_NAME}.desktop"
ICON_DEST="${INSTALL_DIR}/${APP_NAME}.png"

ORT_VERSION="1.29.0"
FORCE_ORT=0

REQUIRED_FILES=(
    "smith_chart_pro"
    "surrogate_model.onnx"
    "multitask_model.onnx"
    "ppo_policy_model.onnx"
)

SOURCE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# ----------------------------------------------------------------------------
# Cores para output (desliga sozinho se não for terminal interativo)
# ----------------------------------------------------------------------------
if [ -t 1 ]; then
    C_RESET='\033[0m'; C_GREEN='\033[0;32m'; C_YELLOW='\033[0;33m'; C_RED='\033[0;31m'; C_BLUE='\033[0;34m'
else
    C_RESET=''; C_GREEN=''; C_YELLOW=''; C_RED=''; C_BLUE=''
fi

info()  { echo -e "${C_BLUE}[INFO]${C_RESET} $1"; }
ok()    { echo -e "${C_GREEN}[ OK ]${C_RESET} $1"; }
warn()  { echo -e "${C_YELLOW}[WARN]${C_RESET} $1"; }
error() { echo -e "${C_RED}[ERRO]${C_RESET} $1" >&2; }

# ----------------------------------------------------------------------------
# Parse de argumentos
# ----------------------------------------------------------------------------
while [ $# -gt 0 ]; do
    case "$1" in
        --source)
            SOURCE_DIR="$(cd "$2" && pwd)"
            shift 2
            ;;
        --ort-version)
            ORT_VERSION="$2"
            shift 2
            ;;
        --force-ort)
            FORCE_ORT=1
            shift
            ;;
        -h|--help)
            grep '^#' "$0" | sed 's/^#//'
            exit 0
            ;;
        *)
            error "Argumento desconhecido: $1"
            exit 1
            ;;
    esac
done

# ----------------------------------------------------------------------------
# Precisa de root (grava em /opt, /usr/local, /usr/share/applications)
# ----------------------------------------------------------------------------
if [ "$(id -u)" -ne 0 ]; then
    error "Este instalador precisa ser rodado como root."
    echo "       Use: sudo $0 $*"
    exit 1
fi

#-----------------------------------------------------------------------------
# (desinstalação: veja uninstall_smith_chart.sh)
#-----------------------------------------------------------------------------

# ----------------------------------------------------------------------------
# Validação dos arquivos de origem (obrigatórios)
# ----------------------------------------------------------------------------
info "Procurando arquivos em: $SOURCE_DIR"

missing=0
for f in "${REQUIRED_FILES[@]}"; do
    if [ ! -f "${SOURCE_DIR}/${f}" ]; then
        error "Arquivo obrigatório não encontrado: ${f}"
        missing=1
    fi
done

if [ "$missing" -eq 1 ]; then
    echo
    error "Instalação abortada. Coloque este script na mesma pasta dos"
    error "arquivos acima (ou use --source DIR) e rode novamente."
    exit 1
fi

ok "Todos os arquivos obrigatórios encontrados."

# Pesos externos (*.onnx.data) -- opcionais, mas se existirem SÃO
# necessários para a IA funcionar. Detecta qualquer arquivo desse padrão
# na pasta de origem (não assume nome fixo).
ONNX_DATA_FILES=()
while IFS= read -r -d '' f; do
    ONNX_DATA_FILES+=("$f")
done < <(find "$SOURCE_DIR" -maxdepth 1 -name "*.onnx.data" -print0 2>/dev/null)

if [ "${#ONNX_DATA_FILES[@]}" -gt 0 ]; then
    ok "Encontrado(s) ${#ONNX_DATA_FILES[@]} arquivo(s) de pesos externos (*.onnx.data)."
else
    info "Nenhum *.onnx.data encontrado (ok se os modelos são pequenos o bastante"
    info "para não usar formato de dados externos -- normalmente é o caso aqui)."
fi

# ----------------------------------------------------------------------------
# Checagem de dependências de sistema (Qt6 etc.) -- apenas avisa
# ----------------------------------------------------------------------------
info "Checando dependências de runtime (ldd)..."
if command -v ldd >/dev/null 2>&1; then
    MISSING_DEPS="$(ldd "${SOURCE_DIR}/smith_chart_pro" 2>/dev/null | grep "not found" || true)"
    if [ -n "$MISSING_DEPS" ]; then
        # libonnxruntime ainda não foi instalada nesta etapa -- ignorada aqui,
        # checada de novo no final depois de instalada.
        FILTERED="$(echo "$MISSING_DEPS" | grep -v "libonnxruntime" || true)"
        if [ -n "$FILTERED" ]; then
            warn "Bibliotecas de sistema ausentes nesta máquina:"
            echo "$FILTERED" | sed 's/^/         /'
            warn "Provavelmente falta o runtime do Qt6. Tente:"
            warn "  sudo apt install qt6-base-dev libqt6svg6 libqt6printsupport6"
        fi
    else
        ok "Nenhuma dependência de sistema ausente (fora o ONNX Runtime, tratado a seguir)."
    fi
else
    warn "Comando 'ldd' não disponível, pulando checagem de dependências."
fi

# ----------------------------------------------------------------------------
# ONNX Runtime: baixa, instala em /usr/local, roda ldconfig, limpa tudo
# ----------------------------------------------------------------------------
ORT_ALREADY_INSTALLED=0
if ldconfig -p 2>/dev/null | grep -q "libonnxruntime.so"; then
    ORT_ALREADY_INSTALLED=1
fi

# Preenchido apenas se formos nós a instalar o ONNX Runtime nesta execução
# (usado para escrever o manifesto de desinstalação com precisão).
ORT_INSTALLED_BY_US=0
ORT_INSTALLED_VERSION=""
ORT_LIB_FILES=()
ORT_INCLUDE_FILES=()

if [ "$ORT_ALREADY_INSTALLED" -eq 1 ] && [ "$FORCE_ORT" -eq 0 ]; then
    ok "ONNX Runtime já está instalado no sistema (libonnxruntime.so encontrada)."
    info "Pulando download. Use --force-ort para forçar reinstalação."
else
    info "Instalando ONNX Runtime v${ORT_VERSION}..."

    case "$(uname -m)" in
        x86_64)
            ORT_ARCH="x64"
            ;;
        aarch64|arm64)
            ORT_ARCH="aarch64"
            ;;
        *)
            error "Arquitetura não suportada automaticamente: $(uname -m)"
            error "Baixe manualmente em https://github.com/microsoft/onnxruntime/releases"
            exit 1
            ;;
    esac

    ORT_PKG="onnxruntime-linux-${ORT_ARCH}-${ORT_VERSION}"
    ORT_URL="https://github.com/microsoft/onnxruntime/releases/download/v${ORT_VERSION}/${ORT_PKG}.tgz"

    # Diretório temporário isolado, com limpeza garantida mesmo se o script
    # falhar no meio do caminho (trap).
    ORT_TMP_DIR="$(mktemp -d /tmp/ort_install.XXXXXX)"
    cleanup_ort_tmp() {
        rm -rf "$ORT_TMP_DIR"
    }
    trap cleanup_ort_tmp EXIT

    ORT_TGZ="${ORT_TMP_DIR}/${ORT_PKG}.tgz"

    info "Baixando ${ORT_URL} ..."
    if command -v curl >/dev/null 2>&1; then
        if ! curl -fL --progress-bar -o "$ORT_TGZ" "$ORT_URL"; then
            error "Falha ao baixar o ONNX Runtime. Verifique sua conexão ou a"
            error "versão informada (--ort-version)."
            exit 1
        fi
    elif command -v wget >/dev/null 2>&1; then
        if ! wget -q --show-progress -O "$ORT_TGZ" "$ORT_URL"; then
            error "Falha ao baixar o ONNX Runtime. Verifique sua conexão ou a"
            error "versão informada (--ort-version)."
            exit 1
        fi
    else
        error "Nem 'curl' nem 'wget' disponíveis para baixar o ONNX Runtime."
        error "Instale um dos dois (ex: sudo apt install curl) e rode de novo."
        exit 1
    fi
    ok "Download concluído (${ORT_TGZ})."

    info "Extraindo..."
    tar -xzf "$ORT_TGZ" -C "$ORT_TMP_DIR"
    ORT_EXTRACTED_DIR="${ORT_TMP_DIR}/${ORT_PKG}"

    if [ ! -d "$ORT_EXTRACTED_DIR" ]; then
        error "Estrutura inesperada dentro do pacote baixado."
        exit 1
    fi

    info "Copiando bibliotecas para /usr/local/lib ..."
    cp -a "${ORT_EXTRACTED_DIR}"/lib/*.so* /usr/local/lib/

    info "Copiando headers para /usr/local/include ..."
    mkdir -p /usr/local/include
    cp -a "${ORT_EXTRACTED_DIR}"/include/*.h /usr/local/include/ 2>/dev/null || true

    # Registra exatamente o que foi copiado (nomes de arquivo, não padrões
    # glob), pra desinstalação futura poder remover com precisão cirúrgica.
    ORT_INSTALLED_BY_US=1
    ORT_INSTALLED_VERSION="$ORT_VERSION"
    while IFS= read -r -d '' f; do
        ORT_LIB_FILES+=("/usr/local/lib/$(basename "$f")")
    done < <(find "${ORT_EXTRACTED_DIR}/lib" -maxdepth 1 -name "*.so*" -print0)
    while IFS= read -r -d '' f; do
        ORT_INCLUDE_FILES+=("/usr/local/include/$(basename "$f")")
    done < <(find "${ORT_EXTRACTED_DIR}/include" -maxdepth 1 -name "*.h" -print0 2>/dev/null)

    ldconfig
    ok "ONNX Runtime v${ORT_VERSION} instalado em /usr/local (lib + include)."

    # Limpeza explícita (o trap já cobre isso, mas deixamos claro no log e
    # cobre também o caso de o script ser chamado de novo sem sair --
    # trap dispara no exit natural do script mais abaixo).
    cleanup_ort_tmp
    trap - EXIT
    ok "Arquivos temporários do ONNX Runtime removidos (.tgz e pasta extraída)."
fi

# ----------------------------------------------------------------------------
# Instalação do app
# ----------------------------------------------------------------------------
info "Instalando aplicativo em ${INSTALL_DIR} ..."
mkdir -p "$INSTALL_DIR"

install -m 755 "${SOURCE_DIR}/smith_chart_pro" "${INSTALL_DIR}/smith_chart_pro"
install -m 644 "${SOURCE_DIR}/surrogate_model.onnx"   "${INSTALL_DIR}/surrogate_model.onnx"
install -m 644 "${SOURCE_DIR}/multitask_model.onnx"   "${INSTALL_DIR}/multitask_model.onnx"
install -m 644 "${SOURCE_DIR}/ppo_policy_model.onnx"  "${INSTALL_DIR}/ppo_policy_model.onnx"
ok "Binário e modelos .onnx copiados."

if [ "${#ONNX_DATA_FILES[@]}" -gt 0 ]; then
    for f in "${ONNX_DATA_FILES[@]}"; do
        install -m 644 "$f" "${INSTALL_DIR}/$(basename "$f")"
    done
    ok "${#ONNX_DATA_FILES[@]} arquivo(s) de pesos externos (*.onnx.data) copiados."
fi

# Ícone opcional: se existir smith_chart_pro.png/svg do lado do instalador, empacota
for ext in png svg; do
    if [ -f "${SOURCE_DIR}/${APP_NAME}.${ext}" ]; then
        install -m 644 "${SOURCE_DIR}/${APP_NAME}.${ext}" "${INSTALL_DIR}/${APP_NAME}.${ext}"
        ICON_DEST="${INSTALL_DIR}/${APP_NAME}.${ext}"
        ok "Ícone empacotado (${ext})."
        break
    fi
done

# ----------------------------------------------------------------------------
# Wrapper de execução
#
# AIEngine carrega os .onnx com caminho relativo ("surrogate_model.onnx"),
# então o processo precisa nascer com cwd = INSTALL_DIR, senão os modelos
# não são encontrados dependendo de onde o usuário clicou/chamou o app.
# Como o ONNX Runtime agora vai para /usr/local/lib (resolvido via
# ldconfig), não precisamos mais de LD_LIBRARY_PATH manual aqui.
# ----------------------------------------------------------------------------
info "Criando wrapper executável..."
cat > "${INSTALL_DIR}/run.sh" <<EOF
#!/bin/bash
# Wrapper gerado por install_smith_chart.sh -- não edite manualmente.
cd "${INSTALL_DIR}"
exec "${INSTALL_DIR}/smith_chart_pro" "\$@"
EOF
chmod 755 "${INSTALL_DIR}/run.sh"
ok "Wrapper criado em ${INSTALL_DIR}/run.sh"

# ----------------------------------------------------------------------------
# Link simbólico no PATH
# ----------------------------------------------------------------------------
ln -sf "${INSTALL_DIR}/run.sh" "$BIN_LINK"
ok "Link criado: ${BIN_LINK} -> ${INSTALL_DIR}/run.sh"

# ----------------------------------------------------------------------------
# Entrada no menu de aplicativos (.desktop)
# ----------------------------------------------------------------------------
info "Registrando atalho no menu de aplicativos..."
cat > "$DESKTOP_FILE" <<EOF
[Desktop Entry]
Type=Application
Name=${APP_DISPLAY_NAME}
Comment=Casamento de impedância com síntese assistida por IA
Exec=${INSTALL_DIR}/run.sh
Icon=${ICON_DEST}
Terminal=false
Categories=Engineering;Science;Electronics;
EOF
chmod 644 "$DESKTOP_FILE"

if command -v update-desktop-database >/dev/null 2>&1; then
    update-desktop-database /usr/share/applications >/dev/null 2>&1 || true
fi
ok "Atalho de menu registrado."

# ----------------------------------------------------------------------------
# Checagem final de dependências
# ----------------------------------------------------------------------------
if command -v ldd >/dev/null 2>&1; then
    FINAL_MISSING="$(ldd "${INSTALL_DIR}/smith_chart_pro" 2>/dev/null | grep "not found" || true)"
    if [ -n "$FINAL_MISSING" ]; then
        warn "Ainda há dependências ausentes após a instalação:"
        echo "$FINAL_MISSING" | sed 's/^/         /'
        warn "O app pode não abrir. Revise as dependências acima."
    else
        ok "Todas as dependências de biblioteca resolvidas."
    fi
fi

# ----------------------------------------------------------------------------
# Manifesto de instalação
#
# Grava exatamente o que foi feito nesta instalação -- em especial, se
# fomos NÓS que instalamos o ONNX Runtime (e quais arquivos exatos) ou se
# ele já estava presente no sistema por outro motivo. O uninstall_smith_
# chart.sh lê este arquivo para saber com precisão o que é seguro remover,
# em vez de usar padrões glob "no chute".
# ----------------------------------------------------------------------------
MANIFEST_FILE="${INSTALL_DIR}/.install_manifest"
{
    echo "# Gerado automaticamente por install_smith_chart.sh -- não editar."
    echo "INSTALL_DATE=\"$(date -Iseconds)\""
    echo "INSTALL_DIR=\"${INSTALL_DIR}\""
    echo "BIN_LINK=\"${BIN_LINK}\""
    echo "DESKTOP_FILE=\"${DESKTOP_FILE}\""
    echo "ORT_INSTALLED_BY_US=${ORT_INSTALLED_BY_US}"
    echo "ORT_INSTALLED_VERSION=\"${ORT_INSTALLED_VERSION}\""
    echo -n "ORT_LIB_FILES=("
    for f in "${ORT_LIB_FILES[@]:-}"; do [ -n "$f" ] && echo -n "\"$f\" "; done
    echo ")"
    echo -n "ORT_INCLUDE_FILES=("
    for f in "${ORT_INCLUDE_FILES[@]:-}"; do [ -n "$f" ] && echo -n "\"$f\" "; done
    echo ")"
} > "$MANIFEST_FILE"
chmod 644 "$MANIFEST_FILE"
ok "Manifesto de instalação salvo em ${MANIFEST_FILE}"

# ----------------------------------------------------------------------------
# Resumo
# ----------------------------------------------------------------------------
echo
ok "Instalação concluída!"
echo -e "  ${C_GREEN}->${C_RESET} Rodar pelo terminal : ${APP_NAME}"
echo -e "  ${C_GREEN}->${C_RESET} Rodar pelo menu     : procure por \"${APP_DISPLAY_NAME}\""
echo -e "  ${C_GREEN}->${C_RESET} Arquivos instalados : ${INSTALL_DIR}"
echo -e "  ${C_GREEN}->${C_RESET} ONNX Runtime        : /usr/local/lib (sistema)"
echo -e "  ${C_GREEN}->${C_RESET} Desinstalar         : sudo ./uninstall_smith_chart.sh"
echo
