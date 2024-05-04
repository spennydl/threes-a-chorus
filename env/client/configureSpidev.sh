#!/bin/bash

set -ex

spigroup="spiusr"
spiuser="debian"
spimode="0660"

print_usage() {
    cat <<EOF
usage: configureSpidev.sh [--user USER] [--group GROUP] [--mode MODE]

Adds a udev rule that will ensure that spidev devices are created with a given
mode and are owned by a given group, and adds the given user to the group.

Accepts the following arguments:

    --user {USER}   : The user to add to the group. Defaults to debian.
    --group {GROUP} : The group to create and use. Defaults to spiusr.
    --mode {MODE}   : The mode, as a string of numbers, to apply to spidev
                      files. Defaults to 0660
EOF
}

# we need to be root
if [[ "$(id -u)" -ne 0 ]]; then
    echo "Error: Root permissions are required"
    exit 1
fi

# parse args
if [[ $# -gt 0 ]]; then
    while (( $# )); do
        case "$1" in
            "--user")
                shift
                spiuser="$1"
            ;;
            "--group")
                shift
                spigroup="$1"
            ;;
            "--mode")
                shift
                spimode="$1"
            ;;
            "--usage"|"-h"|"--help")
                print_usage
                exit 0
            ;;
            *)
                echo "Error: invalid switch $1"
                print_usage
                exit 1
        esac
        shift
    done
fi

# write the udev rule
echo "KERNEL==\"spidev\*\", GROUP=\"$spigroup\", MODE=\"$spimode\"" > /etc/udev/rules.d/82-spi-noroot.rules

# add group if it doesn't exist
getent group "$spigroup" || groupadd "$spigroup"

# add user to group if not in group
getent group "$spigroup" | grep -qw "$spiuser" || gpasswd -a "$spiuser" "$spigroup"
