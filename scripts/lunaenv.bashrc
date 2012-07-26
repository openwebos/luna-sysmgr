#!/bin/sh

scriptdir=$(dirname $0)

. ${scriptdir}/luna-setup.sh

./luna-dbus start

if [ $# -eq 0 ]; then
    luna_dbus_set_target local
fi

echo "You're now LunaBus Ready."
