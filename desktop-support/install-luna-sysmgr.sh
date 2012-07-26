#!/bin/bash

echo "Installing support files for luna-sysmgr in /etc/palm"
BASE_DIR="${HOME}/luna-desktop-binaries/luna-sysmgr"

mkdir -p /etc/palm

ln -sf  ${BASE_DIR}/desktop-support /etc/palm/luna-applauncher

ln -sf -T ${BASE_DIR}/conf/luna.conf /etc/palm/luna.conf
ln -sf -T ${BASE_DIR}/conf/luna-desktop.conf /etc/palm/luna-platform.conf
ln -sf -T ${BASE_DIR}/conf/defaultPreferences.txt /etc/palm/defaultPreferences.txt
ln -sf ${BASE_DIR} /etc/palm/luna-sysmgr
ln -sf ${BASE_DIR}/conf/launcher3 /etc/palm/launcher3
ln -sf ${BASE_DIR}/conf /etc/palm/schemas