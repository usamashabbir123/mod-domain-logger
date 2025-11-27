#!/usr/bin/env bash
# deploy.sh - build, install and enable mod_logfile_domain
# Usage: sudo ./deploy.sh

set -euo pipefail

FREESWITCH_PREFIX=${FREESWITCH_PREFIX:-/usr}
INSTALL_PREFIX=${INSTALL_PREFIX:-${FREESWITCH_PREFIX}}
MODULE_NAME=mod_logfile_domain
MODULE_SO="${MODULE_NAME}.so"
LIB_DIR="${INSTALL_PREFIX}/lib/freeswitch/mod"
CONF_DIR="${INSTALL_PREFIX}/etc/freeswitch/autoload_configs"
MODULE_SRC_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo "=== deploy.sh: mod_logfile_domain ==="
echo "FreeSWITCH prefix: ${FREESWITCH_PREFIX}"
echo "Install prefix: ${INSTALL_PREFIX}"

# Require root for installation steps
if [[ $EUID -ne 0 ]]; then
  echo "This script must be run as root (or with sudo)." >&2
  exit 1
fi

# Build using build.sh (which itself runs autoreconf/configure/make)
if [[ -x "${MODULE_SRC_DIR}/build.sh" ]]; then
  echo "Running build.sh..."
  pushd "${MODULE_SRC_DIR}" >/dev/null
  ./build.sh
  popd >/dev/null
else
  echo "build.sh missing or not executable in ${MODULE_SRC_DIR}. Please run ./build.sh manually or make it executable." >&2
  exit 1
fi

# Install module
echo "Installing module with make install..."
pushd "${MODULE_SRC_DIR}" >/dev/null
make install
popd >/dev/null

# Ensure lib and conf directories exist
mkdir -p "${LIB_DIR}"
mkdir -p "${CONF_DIR}"

# Install config file (automake should have installed conf_DATA already, but ensure copy)
if [[ -f "${MODULE_SRC_DIR}/conf/autoload_configs/logfile_domain.conf.xml" ]]; then
  echo "Installing configuration file to ${CONF_DIR}"
  cp -f "${MODULE_SRC_DIR}/conf/autoload_configs/logfile_domain.conf.xml" "${CONF_DIR}/logfile_domain.conf.xml"
  chown root:root "${CONF_DIR}/logfile_domain.conf.xml" || true
  chmod 0644 "${CONF_DIR}/logfile_domain.conf.xml" || true
fi

# Backup modules.conf.xml then ensure load entry exists
MODULES_CONF="${CONF_DIR%/}/modules.conf.xml"
if [[ -f "${MODULES_CONF}" ]]; then
  BACKUP="${MODULES_CONF}.bak.$(date +%s)"
  echo "Backing up ${MODULES_CONF} -> ${BACKUP}"
  cp -f "${MODULES_CONF}" "${BACKUP}"

  # Insert load line if not present
  if ! grep -q "load module=\"${MODULE_NAME}\"" "${MODULES_CONF}"; then
    echo "Adding load entry to ${MODULES_CONF}"
    # Insert just after <modules> tag
    awk -v mline="    <load module=\"${MODULE_NAME}\"/>" '
      BEGIN{added=0}
      {print}
      /<modules>/ && !added {print mline; added=1}
    ' "${MODULES_CONF}" > "${MODULES_CONF}.new"
    mv "${MODULES_CONF}.new" "${MODULES_CONF}"
  else
    echo "modules.conf.xml already contains load entry for ${MODULE_NAME}"
  fi
else
  echo "modules.conf.xml not found at ${MODULES_CONF}. Creating minimal file."
  cat > "${MODULES_CONF}" <<EOF
<?xml version="1.0"?>
<configuration name="modules.conf" description="Modules">
  <modules>
    <load module="${MODULE_NAME}"/>
  </modules>
</configuration>
EOF
fi

# Reload FreeSWITCH XML and load module
if command -v fs_cli >/dev/null 2>&1; then
  echo "Reloading FreeSWITCH XML and loading module via fs_cli..."
  fs_cli -x "reloadxml"
  fs_cli -x "load ${MODULE_NAME}"
  echo "Done. Check FreeSWITCH log for any errors: /var/log/freeswitch/freeswitch.log"
else
  echo "fs_cli not found; please restart FreeSWITCH manually to load module." >&2
fi

echo "Deployment complete. Module should be installed in: ${LIB_DIR}"
exit 0
