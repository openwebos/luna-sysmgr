#!/bin/sh

scriptdir=$(dirname $0)

. ${scriptdir}/luna-setup.sh

if [ $# -eq 0 ]; then
    luna_dbus_set_target 192.168.2.101
fi

echo "You're now LunaBus-Device Ready."
