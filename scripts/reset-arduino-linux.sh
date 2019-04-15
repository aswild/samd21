#!/bin/bash

msg() {
	if [[ $quiet != y ]]; then
		echo "$*"
	fi
}

die() {
	echo "$*" >&2
	exit 1
}

while getopts "q" opt; do
	case $opt in
		q)  quiet=y ;;
		\?) die "Invalid option" ;;
	esac
done
shift $(($OPTIND - 1))

DEVICE=${1:-/dev/ttyACM0}
if [[ $DEVICE != /dev/* ]]; then
    DEVICE="/dev/$DEVICE"
fi

[[ -e $DEVICE ]] || die "$DEVICE not found"
DEVICE_NOLINK="$(readlink -f $DEVICE)"

usb_product_file="$(readlink -f /sys/class/tty/${DEVICE_NOLINK#/dev/}/device)/../product"
if [[ -f $usb_product_file ]]; then
    if grep -qi sam-ba $usb_product_file; then
        msg "already in SAM-BA bootloader"
        exit 0
    fi
else
    msg "warning: didn't find ${DEVICE_NOLINK#/dev/} in sysfs"
fi

msg "resetting $DEVICE..."
stty -F $DEVICE 1200
set +x
while [[ -r $DEVICE ]]; do : ; done
while [[ ! -r $DEVICE ]]; do : ; done
exit 0
