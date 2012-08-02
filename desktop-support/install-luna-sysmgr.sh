#!/bin/bash
# @@@LICENSE
#
#      Copyright (c) 2012 Hewlett-Packard Development Company, L.P.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# LICENSE@@@


export BASE="${HOME}/luna-desktop-binaries"
export BASE_DIR="${BASE}/luna-sysmgr"

if [ -n "$1" ] && [ "$1" != "remove" ] ; then
    echo "Parameter $1 not recognized"
else
    if [ -d /etc/palm ] && [ -h /etc/palm/luna.conf ] ; then
        unlink /etc/palm/luna-applauncher
        unlink /etc/palm/luna.conf
        unlink /etc/palm/luna-platform.conf
        unlink /etc/palm/defaultPreferences.txt
        unlink /etc/palm/luna-sysmgr
        unlink /etc/palm/launcher3
        unlink /etc/palm/schemas
        if [ "$1" == "remove" ] ; then
            echo "Removed support files for luna-sysmgr from /etc/palm"
        fi
        if [ -h /usr/share/ls2/roles ] ; then
            unlink /usr/share/ls2/roles
            unlink /etc/palm/pubsub_handlers/com.palm.appinstaller
            rmdir  /etc/palm/pubsub_handlers
            unlink /etc/ls2/ls-public.conf
            unlink /etc/ls2/ls-private.conf
            if [ "$1" == "remove" ] ; then
                echo "Removed support files for luna-sysmgr from /usr/share/ls2 and /etc/ls2"
            fi
        fi
    elif [ "$1" == "remove" ] ; then
        echo "Nothing to remove"
    fi

    if [ "$1" != "remove" ] ; then
        echo "Install support files for luna-sysmgr in /etc/ls2, /etc/palm and /usr/share/ls2"

        mkdir -p /etc/palm

        ln -sf  ${BASE_DIR}/desktop-support /etc/palm/luna-applauncher

        ln -sf -T ${BASE_DIR}/conf/luna.conf /etc/palm/luna.conf
        ln -sf -T ${BASE_DIR}/conf/luna-desktop.conf /etc/palm/luna-platform.conf
        ln -sf -T ${BASE_DIR}/conf/defaultPreferences.txt /etc/palm/defaultPreferences.txt
        ln -sf ${BASE_DIR} /etc/palm/luna-sysmgr
        ln -sf ${BASE_DIR}/conf/launcher3 /etc/palm/launcher3
        ln -sf ${BASE_DIR}/conf /etc/palm/schemas
        if [ -d ${BASE}/ls2/roles/prv ] ; then
            mkdir -p /usr/share/ls2
            ln -sf ${BASE}/ls2/roles /usr/share/ls2/roles
            mkdir -p /etc/palm/pubsub_handlers
            ln -sf -T ${BASE}/pubsub_handlers/com.palm.appinstaller /etc/palm/pubsub_handlers/com.palm.appinstaller
            mkdir -p /etc/ls2
            ln -sf -T ${BASE}/ls2/ls-private.conf /etc/ls2/ls-private.conf
            ln -sf -T ${BASE}/ls2/ls-public.conf /etc/ls2/ls-public.conf
            
        fi
    fi
fi
