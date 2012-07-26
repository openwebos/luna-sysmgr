#!/bin/bash

STAGING_DIR="${HOME}/luna-desktop-binaries/staging"
BIN_DIR="${STAGING_DIR}/bin"
LIB_DIR="${STAGING_DIR}/lib"
ETC_DIR="${STAGING_DIR}/etc"

export LD_PRELOAD=/lib/i386-linux-gnu/libSegFault.so
export LD_LIBRARY_PATH=${LIB_DIR}:${LD_LIBRARY_PATH}
export PATH=${BIN_DIR}:${PATH}

echo "Starting LunaSysMgr ..."
./staging/lib/LunaSysMgr  &> /tmp/LunaSysMgr.log
