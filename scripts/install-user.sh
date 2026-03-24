#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BIN_DIR="${HOME}/.local/bin"
AUTOSTART_DIR="${HOME}/.config/autostart"

mkdir -p "${BIN_DIR}" "${AUTOSTART_DIR}"

install -Dm755 "${ROOT_DIR}/changewall" "${BIN_DIR}/changewall"
install -Dm755 "${ROOT_DIR}/scripts/changewall-next" "${BIN_DIR}/changewall-next"
install -Dm755 "${ROOT_DIR}/scripts/changewall-random" "${BIN_DIR}/changewall-random"
install -Dm755 "${ROOT_DIR}/scripts/changewall-setup" "${BIN_DIR}/changewall-setup"

sed "s|^Exec=.*$|Exec=${BIN_DIR}/changewall|" \
  "${ROOT_DIR}/packaging/changewall.desktop" \
  > "${AUTOSTART_DIR}/changewall.desktop"

chmod 644 "${AUTOSTART_DIR}/changewall.desktop"

echo "Instalado en ${BIN_DIR}/changewall"
echo "Comandos extra: changewall-next, changewall-random, changewall-setup"
echo "Autostart creado en ${AUTOSTART_DIR}/changewall.desktop"
echo "En el primer arranque, ChangeWall abrira un selector para elegir tu carpeta de fondos."
