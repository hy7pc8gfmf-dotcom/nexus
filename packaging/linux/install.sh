#!/usr/bin/env bash
# ─────────────────────────────────────────────────────────────────────
# Nexus Linux 安装脚本
# ─────────────────────────────────────────────────────────────────────
# 用法:
#   chmod +x install.sh
#   sudo ./install.sh              # 安装到 /opt/nexus
#   ./install.sh --prefix=~/.local # 安装到用户目录（无需 root）
#   ./install.sh --uninstall       # 卸载
#
# 支持: Ubuntu 20.04+, Debian 11+, Fedora 36+, Arch Linux
# ─────────────────────────────────────────────────────────────────────

set -euo pipefail

# ═══════════════════════════════════════════════════════════════════
# 配置
# ═══════════════════════════════════════════════════════════════════

APP_NAME="nexus"
APP_VERSION="1.0.0"
INSTALL_PREFIX="/opt/${APP_NAME}"
BUILD_DIR=""
SKIP_BUILD=false

# ═══════════════════════════════════════════════════════════════════
# 颜色
# ═══════════════════════════════════════════════════════════════════

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

info()  { echo -e "${BLUE}[INFO]${NC}  $1"; }
ok()    { echo -e "${GREEN}[OK]${NC}    $1"; }
warn()  { echo -e "${YELLOW}[WARN]${NC}  $1"; }
error() { echo -e "${RED}[ERROR]${NC} $1"; }

# ═══════════════════════════════════════════════════════════════════
# 检查系统
# ═══════════════════════════════════════════════════════════════════

check_system() {
  info "检查系统环境..."

  if [[ ! -f /etc/os-release ]]; then
    warn "无法检测操作系统类型 (无 /etc/os-release)"
  else
    source /etc/os-release
    info "操作系统: $PRETTY_NAME"
  fi

  # 检查 GCC
  if command -v g++ &>/dev/null; then
    GPP_VER=$(g++ -dumpversion)
    ok "GCC: $GPP_VER"
    if [[ ! "$(printf '%s\n' "14" "$GPP_VER" | sort -V | head -1)" = "14" ]]; then
      error "需要 GCC >= 14 (当前: $GPP_VER)"
      exit 1
    fi
  else
    error "未找到 g++"
    echo "安装: sudo apt install g++-14  (Debian/Ubuntu)"
    echo "      sudo dnf install gcc-c++ (Fedora)"
    exit 1
  fi

  # 检查 CMake
  if command -v cmake &>/dev/null; then
    CMAKE_VER=$(cmake --version | head -1 | awk '{print $3}')
    ok "CMake: $CMAKE_VER"
  else
    error "未找到 cmake"
    exit 1
  fi

  # 检查 Git
  if command -v git &>/dev/null; then
    ok "Git: $(git --version | awk '{print $3}')"
  else
    error "未找到 git"
    exit 1
  fi

  ok "系统检查通过"
}

# ═══════════════════════════════════════════════════════════════════
# 构建
# ═══════════════════════════════════════════════════════════════════

do_build() {
  info "开始构建 Nexus..."

  local SRC_DIR
  SRC_DIR="$(cd "$(dirname "$0")/.." && pwd)"
  BUILD_DIR="${SRC_DIR}/build-linux"

  if [[ -d "$BUILD_DIR" ]]; then
    warn "构建目录已存在，执行增量构建"
  fi

  cmake -B "$BUILD_DIR" -S "$SRC_DIR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DNEXUS_CUDA_SUPPORT=OFF \
    -DNEXUS_BUILD_TESTS=OFF

  cmake --build "$BUILD_DIR" --parallel "$(nproc)"

  ok "构建完成"
}

# ═══════════════════════════════════════════════════════════════════
# 安装
# ═══════════════════════════════════════════════════════════════════

do_install() {
  info "安装到: $INSTALL_PREFIX"

  local BIN_DIR="${INSTALL_PREFIX}/bin"
  local DOC_DIR="${INSTALL_PREFIX}/share/doc"
  local DATA_DIR="${INSTALL_PREFIX}/data"
  local CONF_DIR="${INSTALL_PREFIX}/conf"

  # 确定二进制来源
  local EXE_SRC
  if [[ -n "$BUILD_DIR" ]]; then
    EXE_SRC="$BUILD_DIR/bin"
  else
    EXE_SRC="$(cd "$(dirname "$0")/../build" && pwd)/bin"
  fi

  local SRC_DIR
  SRC_DIR="$(cd "$(dirname "$0")/.." && pwd)"

  # 创建目录
  mkdir -p "$BIN_DIR" "$DOC_DIR" "$DATA_DIR" "$CONF_DIR"

  # 复制二进制
  local EXES=(
    neural daemon core algo algo_engine env_checker
    psyche bridge coordinator quantum logic holographic
  )

  local COUNT=0
  for exe in "${EXES[@]}"; do
    local SRC="${EXE_SRC}/${exe}"
    if [[ -f "$SRC" ]]; then
      install -m 755 "$SRC" "$BIN_DIR/"
      ((COUNT++))
    else
      warn "未找到: $SRC (此模块在 Linux 上可能不可用)"
    fi
  done

  ok "已安装 $COUNT 个可执行模块"

  # 复制文档
  install -m 644 "$SRC_DIR/README.md"   "$DOC_DIR/"
  install -m 644 "$SRC_DIR/LICENSE"     "$DOC_DIR/"
  install -m 644 "$SRC_DIR/NOTICE"      "$DOC_DIR/"
  install -m 644 "$SRC_DIR/CHANGELOG.md" "$DOC_DIR/"

  # 创建环境变量配置
  local PROFILE_FILE
  if [[ "$INSTALL_PREFIX" == "/opt/"* ]]; then
    PROFILE_FILE="/etc/profile.d/nexus.sh"
  else
    PROFILE_FILE="${INSTALL_PREFIX}/env.sh"
  fi

  cat > "$PROFILE_FILE" << PROFILEEOF
# Nexus 环境变量
export NEXUS_DATA_DIR="${DATA_DIR}"
export PATH="\${PATH}:${BIN_DIR}"
PROFILEEOF

  if [[ "$PROFILE_FILE" == "/etc/profile.d/"* ]]; then
    chmod 644 "$PROFILE_FILE"
  fi

  ok "环境变量配置: $PROFILE_FILE"
  ok "安装完成!"
  echo ""
  echo "  可执行文件: $BIN_DIR"
  echo "  数据目录:   $DATA_DIR"
  echo "  文档:       $DOC_DIR"
  echo ""
  echo "  运行环境检查: env_checker"
  echo "  启动守护:     daemon"
  echo "  查看帮助:     neural --help"
}

# ═══════════════════════════════════════════════════════════════════
# 卸载
# ═══════════════════════════════════════════════════════════════════

do_uninstall() {
  if [[ ! -d "$INSTALL_PREFIX" ]]; then
    error "未找到 Nexus 安装目录: $INSTALL_PREFIX"
    exit 1
  fi

  warn "即将卸载 Nexus (版本 $APP_VERSION) 从: $INSTALL_PREFIX"
  echo -n "确认卸载? [y/N] "
  read -r CONFIRM
  if [[ ! "$CONFIRM" =~ ^[Yy]$ ]]; then
    info "取消卸载"
    exit 0
  fi

  # 停止守护进程 (如果运行中)
  if [[ -f "${INSTALL_PREFIX}/bin/daemon" ]]; then
    "${INSTALL_PREFIX}/bin/daemon" --stop 2>/dev/null || true
  fi

  rm -rf "$INSTALL_PREFIX"

  # 清理 profile.d
  if [[ "$INSTALL_PREFIX" == "/opt/"* ]]; then
    rm -f /etc/profile.d/nexus.sh
  fi

  ok "卸载完成"
}

# ═══════════════════════════════════════════════════════════════════
# 主流程
# ═══════════════════════════════════════════════════════════════════

main() {
  # 解析参数
  while [[ $# -gt 0 ]]; do
    case "$1" in
      --prefix=*)
        INSTALL_PREFIX="${1#*=}"
        shift
        ;;
      --prefix)
        INSTALL_PREFIX="$2"
        shift 2
        ;;
      --uninstall)
        do_uninstall
        exit 0
        ;;
      --skip-build)
        SKIP_BUILD=true
        shift
        ;;
      --help|-h)
        echo "Nexus Linux 安装脚本 v${APP_VERSION}"
        echo ""
        echo "用法: $0 [选项]"
        echo ""
        echo "选项:"
        echo "  --prefix=PATH   安装路径 (默认: /opt/nexus)"
        echo "  --uninstall     卸载"
        echo "  --skip-build    跳过构建 (使用已存在的构建产物)"
        echo "  --help, -h      显示此帮助"
        exit 0
        ;;
      *)
        error "未知选项: $1"
        exit 1
        ;;
    esac
  done

  echo "═══════════════════════════════════════════"
  echo "  Nexus v${APP_VERSION} Linux 安装程序"
  echo "═══════════════════════════════════════════"
  echo ""

  check_system

  if [[ "$SKIP_BUILD" == false ]]; then
    do_build
  fi

  # 检查 root 权限 (安装到系统路径时需要)
  if [[ "$INSTALL_PREFIX" == "/opt/"* ]] && [[ "$EUID" -ne 0 ]]; then
    error "安装到 $INSTALL_PREFIX 需要 root 权限"
    echo "请使用 sudo: sudo $0 $@"
    exit 1
  fi

  do_install
}

main "$@"
