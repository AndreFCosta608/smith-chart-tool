#!/bin/bash
#
# uninstall_smith_chart.sh
#
# Desinstalador do Smith Chart Tool - RF Matching Studio (IA Edition)
#
# Uso:
#   sudo ./uninstall_smith_chart.sh                 # remove o app; sobre o
#                                                    # ONNX Runtime, pergunta
#                                                    # antes de remover
#   sudo ./uninstall_smith_chart.sh --dry-run        # mostra o que SERIA
#                                                    # removido, sem apagar
#                                                    # nada de verdade
#   sudo ./uninstall_smith_chart.sh --yes            # não pergunta nada,
#                                                    # assume "sim" em tudo
#   sudo ./uninstall_smith_chart.sh --keep-onnxruntime  # remove só o app,
#                                                    # nunca mexe no ONNX
#                                                    # Runtime do sistema
#   sudo ./uninstall_smith_chart.sh --purge          # remove tudo, incluindo
#                                                    # o ONNX Runtime, sem
#                                                    # perguntar (equivale a
#                                                    # --yes sem
#                                                    # --keep-onnxruntime)
#
# COMO ESTE SCRIPT DECIDE O QUE REMOVER:
#   Ele lê o manifesto gravado pelo install_smith_chart.sh em
#   /opt/smith_chart_pro/.install_manifest, que registra:
#     - se o instalador foi quem baixou/instalou o ONNX Runtime nesta
#       máquina, ou se ele já estava presente antes (nesse caso o
#       instalador não mexeu nele, e este script por padrão também não mexe);
#     - a lista EXATA dos arquivos de biblioteca/header instalados
#       (não um padrão glob "chutado", que poderia apagar algo de outro
#       programa que por acaso tenha nome parecido).
#   Se o manifesto não existir (ex: instalação feita manualmente, ou por
#   uma versão antiga do instalador), o script avisa claramente e cai num
#   modo conservador: só remove o app, e para o ONNX Runtime pede
#   confirmação extra e explícita antes de usar um padrão glob.
#
set -euo pipefail

APP_NAME="smith_chart_pro"
APP_DISPLAY_NAME="Smith Chart Tool - RF Matching Studio (IA Edition)"
INSTALL_DIR="/opt/${APP_NAME}"
BIN_LINK="/usr/local/bin/${APP_NAME}"
DESKTOP_FILE="/usr/share/applications/${APP_NAME}.desktop"
MANIFEST_FILE="${INSTALL_DIR}/.install_manifest"

ASSUME_YES=0
KEEP_ORT=0
PURGE=0
DRY_RUN=0

# ----------------------------------------------------------------------------
# Cores
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
plan()  { echo -e "  ${C_YELLOW}[REMOVERIA]${C_RESET} $1"; }

# ----------------------------------------------------------------------------
# Parse de argumentos
# ----------------------------------------------------------------------------
while [ $# -gt 0 ]; do
    case "$1" in
        --yes|-y)
            ASSUME_YES=1
            shift
            ;;
        --keep-onnxruntime)
            KEEP_ORT=1
            shift
            ;;
        --purge)
            PURGE=1
            ASSUME_YES=1
            shift
            ;;
        --dry-run)
            DRY_RUN=1
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

if [ "$KEEP_ORT" -eq 1 ] && [ "$PURGE" -eq 1 ]; then
    error "--purge e --keep-onnxruntime são mutuamente exclusivos."
    exit 1
fi

# ----------------------------------------------------------------------------
# Precisa de root, exceto em --dry-run (só leitura/simulação)
# ----------------------------------------------------------------------------
if [ "$(id -u)" -ne 0 ] && [ "$DRY_RUN" -eq 0 ]; then
    error "Este desinstalador precisa ser rodado como root."
    echo "       Use: sudo $0 $*"
    exit 1
fi

confirm() {
    # confirm "pergunta" -> retorna 0 (sim) ou 1 (não)
    if [ "$ASSUME_YES" -eq 1 ]; then
        return 0
    fi
    local reply
    read -r -p "$(echo -e "${C_YELLOW}[?]${C_RESET} $1 [s/N] ")" reply
    case "$reply" in
        [sS]|[sS][iI][mM]) return 0 ;;
        *) return 1 ;;
    esac
}

if [ "$DRY_RUN" -eq 1 ]; then
    info "Modo --dry-run: nada será apagado de verdade, só simulado."
fi

echo
info "Desinstalando ${APP_DISPLAY_NAME}..."
echo

# ----------------------------------------------------------------------------
# Lê o manifesto ANTES de remover qualquer coisa (ele mora dentro do
# INSTALL_DIR que estamos prestes a apagar)
# ----------------------------------------------------------------------------
ORT_INSTALLED_BY_US=0
ORT_INSTALLED_VERSION=""
ORT_LIB_FILES=()
ORT_INCLUDE_FILES=()
HAS_MANIFEST=0

if [ -f "$MANIFEST_FILE" ]; then
    HAS_MANIFEST=1
    # shellcheck disable=SC1090
    source "$MANIFEST_FILE"
    ok "Manifesto de instalação encontrado (instalado em: ${INSTALL_DATE:-desconhecido})."
else
    warn "Manifesto não encontrado em ${MANIFEST_FILE}."
    warn "Essa instalação pode ter sido feita manualmente ou por uma versão"
    warn "antiga do instalador. Vou remover o app normalmente, mas para o"
    warn "ONNX Runtime do sistema vou pedir uma confirmação extra."
fi

# ----------------------------------------------------------------------------
# Passo 1: aplicativo (binário, modelos, link, atalho de menu)
# ----------------------------------------------------------------------------
echo
info "Passo 1/2 -- Aplicativo"

APP_TARGETS=("$INSTALL_DIR" "$BIN_LINK" "$DESKTOP_FILE")
ANY_APP_TARGET_EXISTS=0
for t in "${APP_TARGETS[@]}"; do
    if [ -e "$t" ] || [ -L "$t" ]; then
        ANY_APP_TARGET_EXISTS=1
    fi
done

if [ "$ANY_APP_TARGET_EXISTS" -eq 0 ]; then
    warn "Nenhum arquivo do app encontrado (já desinstalado?)."
else
    if [ "$DRY_RUN" -eq 1 ]; then
        [ -d "$INSTALL_DIR" ]  && plan "diretório completo: $INSTALL_DIR"
        [ -L "$BIN_LINK" ]     && plan "link: $BIN_LINK"
        [ -f "$DESKTOP_FILE" ] && plan "atalho de menu: $DESKTOP_FILE"
    else
        rm -f "$BIN_LINK"
        rm -f "$DESKTOP_FILE"
        rm -rf "$INSTALL_DIR"
        command -v update-desktop-database >/dev/null 2>&1 && update-desktop-database /usr/share/applications >/dev/null 2>&1 || true
        ok "Aplicativo removido (binário, modelos .onnx, link e atalho de menu)."
    fi
fi

# ----------------------------------------------------------------------------
# Passo 2: ONNX Runtime do sistema
# ----------------------------------------------------------------------------
echo
info "Passo 2/2 -- ONNX Runtime"

if [ "$KEEP_ORT" -eq 1 ]; then
    info "--keep-onnxruntime informado: ONNX Runtime do sistema não será tocado."

elif [ "$HAS_MANIFEST" -eq 1 ] && [ "${ORT_INSTALLED_BY_US:-0}" -eq 0 ] && [ "$PURGE" -eq 0 ]; then
    info "O manifesto indica que o ONNX Runtime já estava presente no sistema"
    info "antes desta instalação (não fomos nós que o colocamos aqui.)"
    info "Por segurança, ele NÃO será removido. Use --purge se quiser forçar."

elif [ "$HAS_MANIFEST" -eq 1 ] && [ "${ORT_INSTALLED_BY_US:-0}" -eq 1 ]; then
    echo "  Versão instalada por nós: ${ORT_INSTALLED_VERSION:-desconhecida}"
    echo "  Arquivos que seriam removidos:"
    for f in "${ORT_LIB_FILES[@]:-}"; do [ -n "$f" ] && echo "    - $f"; done
    for f in "${ORT_INCLUDE_FILES[@]:-}"; do [ -n "$f" ] && echo "    - $f"; done
    echo

    if [ "$DRY_RUN" -eq 1 ]; then
        for f in "${ORT_LIB_FILES[@]:-}" "${ORT_INCLUDE_FILES[@]:-}"; do
            [ -n "$f" ] && plan "$f"
        done
        plan "ldconfig (atualização do cache de bibliotecas)"
    else
        if confirm "Remover também o ONNX Runtime do sistema? (outros programas podem depender dele)"; then
            removed=0
            for f in "${ORT_LIB_FILES[@]:-}" "${ORT_INCLUDE_FILES[@]:-}"; do
                if [ -n "$f" ] && [ -e "$f" ]; then
                    rm -f "$f"
                    removed=$((removed + 1))
                fi
            done
            ldconfig
            ok "${removed} arquivo(s) do ONNX Runtime removidos e cache atualizado (ldconfig)."
        else
            info "Mantendo o ONNX Runtime do sistema."
        fi
    fi

elif [ "$HAS_MANIFEST" -eq 1 ] && [ "${ORT_INSTALLED_BY_US:-0}" -eq 0 ] && [ "$PURGE" -eq 1 ]; then
    # --purge foi explicitamente pedido, mesmo o manifesto dizendo que o ORT
    # já estava presente antes de nós. Como não temos a lista exata (o
    # manifesto só guarda arquivos de quando SOMOS nós que instalamos),
    # caímos no padrão glob -- mas só porque --purge foi pedido de forma
    # explícita e consciente pelo usuário.
    warn "--purge pedido: o manifesto diz que o ONNX Runtime NÃO foi instalado"
    warn "por nós, mas vou removê-lo mesmo assim via padrão de busca, por"
    warn "você ter pedido --purge explicitamente."
    echo "    - /usr/local/lib/libonnxruntime*"
    echo "    - /usr/local/include/onnxruntime*.h"

    if [ "$DRY_RUN" -eq 1 ]; then
        plan "/usr/local/lib/libonnxruntime* (padrão glob, --purge)"
        plan "/usr/local/include/onnxruntime*.h (padrão glob, --purge)"
        plan "ldconfig"
    else
        rm -f /usr/local/lib/libonnxruntime* /usr/local/include/onnxruntime*.h
        ldconfig
        ok "ONNX Runtime removido via padrão glob (--purge)."
    fi

else
    # Sem manifesto: modo conservador, glob explícito só com confirmação clara
    FOUND_ORT="$(ldconfig -p 2>/dev/null | grep -c "libonnxruntime.so" || true)"
    if [ "$FOUND_ORT" -eq 0 ]; then
        info "Nenhum ONNX Runtime encontrado no sistema. Nada a fazer."
    else
        warn "Sem manifesto, não sei dizer com certeza se este ONNX Runtime foi"
        warn "instalado por nós ou por outro programa/instalação manual."
        echo "  Arquivos que seriam removidos (padrão de busca, não lista exata):"
        echo "    - /usr/local/lib/libonnxruntime*"
        echo "    - /usr/local/include/onnxruntime*.h"
        echo

        if [ "$DRY_RUN" -eq 1 ]; then
            plan "/usr/local/lib/libonnxruntime* (padrão glob)"
            plan "/usr/local/include/onnxruntime*.h (padrão glob)"
            plan "ldconfig"
        else
            if [ "$PURGE" -eq 1 ]; then
                do_remove=0
                if confirm "Sem manifesto -- tem certeza que quer remover o ONNX Runtime via padrão glob?"; then
                    do_remove=1
                fi
            else
                warn "Por segurança, sem manifesto isso só é removido com --purge"
                warn "(e ainda assim vai pedir confirmação, a menos que combine com --yes)."
                do_remove=0
            fi

            if [ "$do_remove" -eq 1 ]; then
                rm -f /usr/local/lib/libonnxruntime* /usr/local/include/onnxruntime*.h
                ldconfig
                ok "ONNX Runtime removido via padrão glob."
            else
                info "Mantendo o ONNX Runtime do sistema."
            fi
        fi
    fi
fi

echo
if [ "$DRY_RUN" -eq 1 ]; then
    ok "Simulação concluída (--dry-run). Nada foi alterado no sistema."
    echo "  Rode sem --dry-run para aplicar de verdade."
else
    ok "Desinstalação concluída."
fi
echo
