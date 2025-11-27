#!/usr/bin/env bash
# uninstall.sh - unload and remove mod_logfile_domain
# Usage: sudo ./uninstall.sh

set -euo pipefail

FREESWITCH_PREFIX=${FREESWITCH_PREFIX:-/usr}
INSTALL_PREFIX=${INSTALL_PREFIX:-${FREESWITCH_PREFIX}}
MODULE_NAME=mod_logfile_domain
LIB_DIR="${INSTALL_PREFIX}/lib/freeswitch/mod"
CONF_DIR="${INSTALL_PREFIX}/etc/freeswitch/autoload_configs"
MODULE_SO="${LIB_DIR}/${MODULE_NAME}.so"
MODULE_LA="${LIB_DIR}/${MODULE_NAME}.la"
MODULE_CONF="${CONF_DIR}/logfile_domain.conf.xml"
MODULES_CONF="${CONF_DIR}/modules.conf.xml"

echo "=== uninstall.sh: mod_logfile_domain ==="

if [[ $EUID -ne 0 ]]; then
  echo "This script must be run as root (or with sudo)." >&2
  exit 1
fi

# Try to unload module via fs_cli
if command -v fs_cli >/dev/null 2>&1; then
  echo "Unloading module in FreeSWITCH (if loaded)"
  fs_cli -x "unload ${MODULE_NAME}" || echo "Module unload command returned non-zero or module not loaded"
else
  echo "fs_cli not found; proceed to remove files manually." >&2
fi

# Remove module files
if [[ -f "${MODULE_SO}" ]]; then
  echo "Removing ${MODULE_SO}"
  rm -f "${MODULE_SO}"
else
  echo "Module binary ${MODULE_SO} not present"
fi

if [[ -f "${MODULE_LA}" ]]; then
  echo "Removing ${MODULE_LA}"
  rm -f "${MODULE_LA}"
fi

# Remove config file
if [[ -f "${MODULE_CONF}" ]]; then
  echo "Backing up and removing configuration ${MODULE_CONF}"
  cp -f "${MODULE_CONF}" "${MODULE_CONF}.bak.$(date +%s)"
  rm -f "${MODULE_CONF}"
fi

# Remove load entry from modules.conf.xml
if [[ -f "${MODULES_CONF}" ]]; then
  if grep -q "load module=\"${MODULE_NAME}\"" "${MODULES_CONF}"; then
    echo "Removing load entry from ${MODULES_CONF}"
    cp -f "${MODULES_CONF}" "${MODULES_CONF}.bak.$(date +%s)"
    # Remove any line containing load module="mod_logfile_domain"
    sed -i '/load module="'"${MODULE_NAME}"'"/d' "${MODULES_CONF}"
  else
    echo "No load entry found in ${MODULES_CONF} for ${MODULE_NAME}"
  fi
else
  echo "modules.conf.xml not found at ${MODULES_CONF}"
fi

# Reload FreeSWITCH XML
if command -v fs_cli >/dev/null 2>&1; then
  echo "Reloading FreeSWITCH XML"
  fs_cli -x "reloadxml"
else
  echo "fs_cli not found; please restart FreeSWITCH manually to apply changes." >&2
fi

echo "Uninstall complete. Backups (if any) are stored alongside original files with .bak.TIMESTAMP"
exit 0