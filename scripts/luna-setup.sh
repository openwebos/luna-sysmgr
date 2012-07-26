#!/bin/sh

# Sets up the luna environment
#
# The following variables are exported:
#	LUNA_SIM_DIR
#	LUNA_DBUS_TARGET
#	LUNA_DBUS_SERVICES_DIR
#	DBUS_SESSION_BUS_ADDRESS

# default simulator path
SIM_DIR=/opt/Palm/luna/desktop-binaries

# dbus targets
DBUS_LOCAL=local
DBUS_LOCAL_PATH=/tmp/lunabus
DBUS_LOCAL_PATH_SYS=/tmp/lunabus_private
DBUS_QEMU=qemu
DBUS_QEMU_PORT=5544
DBUS_QEMU_PORT_SYS=5545
DBUS_REMOTE_PORT=4444
DBUS_REMOTE_PORT_SYS=4445

# dbus services dir
DBUS_SERVICES_DIR=/var/opt/luna/dbus-1/services

# luna apps dir
#LUNA_APPS_DIR="$(cat /etc/palm/luna.conf | grep ^ApplicationPath= | sed 's/^ApplicationPath=//')"
LUNA_APPS_DIR="/var/luna/applications"

#
# abs_path <path>
#
# Returns the absolute path
#
abs_path()
{
	echo "$(cd "$(dirname "$1")" && pwd)/$(basename "$1")" || exit 1
}

#
# luna_dbus_print_help
#
#   print common (target-specific) help text
#
luna_dbus_target_help()
{
    echo "where target is one of:"
    echo "    local       uses the local dbus session (default)"
    echo "    qemu        tethers dbus to the nova-emulator"
    echo "    device      tethers dbus to a (USB) connected device"
    echo "    host:port   tethers dbus to the device at host:port"
}

#
# luna_dbus_set_target [target]
#
#   where target is one of: local, qemu, or host:port
#
# Sets up the DBUS_SESSION_BUS_ADDRESS variable.
#
luna_dbus_set_target()
{
	# no args = default (local)
	if [ $# -eq 0 ]
	then
		export LUNA_DBUS_TARGET=$DBUS_LOCAL
    else
		export LUNA_DBUS_TARGET="$1"
	fi

	# check if we have a valid target
	if [ -z "$LUNA_DBUS_TARGET" ]
	then
		echo "Invalid target: $@"
		exit 1
	fi

	# split into host and port
	DBUS_HOST=${LUNA_DBUS_TARGET%%:*}
	DBUS_PORT=${LUNA_DBUS_TARGET#*:}
	if [ "$DBUS_PORT" = "$DBUS_HOST" ]
	then
		DBUS_PORT=
	fi

	# pick default port for qemu
	if [ "$DBUS_HOST" = "$DBUS_QEMU" ]
	then
		DBUS_PORT=$DBUS_QEMU_PORT
		DBUS_PORT_SYS=$DBUS_QEMU_PORT_SYS
	fi

	# make DBUS address
	if [ "$DBUS_HOST" = "$DBUS_LOCAL" ]
	then
		DBUS_ADDR="unix:path=$DBUS_LOCAL_PATH"
		DBUS_ADDR_SYS="unix:path=$DBUS_LOCAL_PATH_SYS"
	else
		if [ -z "$DBUS_PORT" ]
		then
			DBUS_PORT=$DBUS_REMOTE_PORT
			DBUS_PORT_SYS=$DBUS_REMOTE_PORT_SYS
		fi
		DBUS_ADDR="tcp:host=$DBUS_HOST,port=$DBUS_PORT"
		DBUS_ADDR_SYS="tcp:host=$DBUS_HOST,port=$DBUS_PORT_SYS"
	fi

	export DBUS_HOST
	export DBUS_SESSION_BUS_ADDRESS="$DBUS_ADDR"
	export DBUS_SYSTEM_BUS_ADDRESS="$DBUS_ADDR_SYS"
}

#
# luna_dbus_get_target
#
# Prints out the current dbus device.
#
luna_dbus_get_target()
{
	echo $LUNA_DBUS_TARGET
}

#
# luna_dbus_session_address
#
# Prints out the current dbus session address.
#
luna_dbus_session_address()
{
	echo $DBUS_SESSION_BUS_ADDRESS
}

#
# find_dbus_process
#
# Returns the pid of the dbus process (or empty string if not found)
#
find_dbus_process() {
	ps -o pid,cmd -C dbus-daemon |grep luna-dbus.conf |sed 's/^[ ]*//' |cut -d' ' -f1
}

find_dbus_process_sys() {
	ps -o pid,cmd -C dbus-daemon |grep luna-dbus_private.conf |sed 's/^[ ]*//' |cut -d' ' -f1
}

#
# luna_sim_set_dir </path/to/simulator>
#
# Sets location to the luna simulator (desktop binaries).
#
luna_sim_set_dir()
{
	# no args = default (local)
	if [ $# -eq 0 ]
	then
		export LUNA_SIM_DIR="$SIM_DIR"
    else
		export LUNA_SIM_DIR="$1"
	fi

	# Added purple-2 specifically so libaim.so (and libicq.so) can find liboscar.so
	# (and libxmpp.so can find libxmpp.so).  Other libs in purple-2 are libpurple
	# plug-ins loaded explicitly with a static path (since g_module_open unfortunately
	# does not look on LD_LIBRARY_PATH).
	export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$LUNA_SIM_DIR:$LUNA_SIM_DIR/purple-2

	# This gives us nice diagnostic dump if something (that we run) crashes
	export LD_PRELOAD=/lib/libSegFault.so
}

#
# luna_sim_get_dir
#
# Prints out the current dbus device.
#
luna_sim_get_dir()
{
	echo $LUNA_SIM_DIR
}

#
# luna_dbus_set_services_dir </path/to/services>
#
# Sets the location of the dbus services.
#
luna_dbus_set_services_dir()
{
#TODO we actually have to modify the entry in luna-dbus.conf to point to the new dir...

	# no args = default (local)
	if [ $# -eq 0 ]
	then
		export LUNA_DBUS_SERVICES_DIR="$DBUS_SERVICES_DIR"
    else
		export LUNA_DBUS_SERVICES_DIR="$1"
	fi
}

#
# luna_dbus_services
#
# Prints out the current location of the dbus services.
#
luna_dbus_get_services_dir()
{
	echo $LUNA_DBUS_SERVICES_DIR
}

# set defaults
luna_sim_set_dir "$SIM_DIR"
luna_dbus_set_target "$DBUS_LOCAL"
luna_dbus_set_services_dir "$DBUS_SERVICES_DIR"


