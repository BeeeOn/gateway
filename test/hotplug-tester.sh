#! /bin/sh

FIFO_PATH="/var/run/beeeon-gateway.hotplug"

if [ -n "$1" ]; then
	FIFO_PATH="$1"
fi

if [ ! -p "${FIFO_PATH}" ]; then
	mkfifo "${FIFO_PATH}" || exit -1
fi

good_add()
{
	cat <<__EOF >"${FIFO_PATH}"
ACTION=add
SUBSYSTEM=any
NAME=example1
NODE=/dev/example1
TYPE=example_type
DRIVER=example_drv
ID_SERIAL=3486798
__EOF
}

good_remove()
{
	cat <<__EOF >"${FIFO_PATH}"
ACTION=remove
SUBSYSTEM=any
NAME=example1
NODE=/dev/example1
TYPE=example_type
DRIVER=example_drv
ID_SERIAL=3486798
__EOF
}

good_add_remove()
{
	cat <<__EOF >"${FIFO_PATH}"
ACTION=add
SUBSYSTEM=any
NAME=example1
NODE=/dev/example1
TYPE=example_type
DRIVER=example_drv
ID_SERIAL=3486798

ACTION=remove
SUBSYSTEM=any
NAME=example1
NODE=/dev/example1
TYPE=example_type
DRIVER=example_drv
ID_SERIAL=3486798
__EOF
}

bad_event()
{
	cat <<__EOF >"${FIFO_PATH}"
anyline
OTHERline
__EOF
}

bad_duplicate_action()
{
	cat <<__EOF >"${FIFO_PATH}"
ACTION=add
SUBSYSTEM=any
NAME=example1
NODE=/dev/example1
TYPE=example_type
DRIVER=example_drv
ID_SERIAL=3486798
ACTION=remove
SUBSYSTEM=any
NAME=example1
NODE=/dev/example1
TYPE=example_type
DRIVER=example_drv
ID_SERIAL=3486798
__EOF
}

good_add
good_remove
good_add_remove
bad_event
bad_duplicate_action
