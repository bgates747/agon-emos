#!/bin/sh
set -eu
: "${AGONDEV_TOOLCHAIN:?Set the selected AgonDev toolchain path}"
for fixture_name in stock eirene scalar otirx ours; do
    make -C "$1/$fixture_name/fixture" AGONDEV_TOOLCHAIN="$AGONDEV_TOOLCHAIN"
done
