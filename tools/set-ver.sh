#!/bin/bash
export CIRCLE_TAG=3004.388.4_0-gnuton0_beta1
export TAG=$(echo "${CIRCLE_TAG}" | sed 's/^[^.]*\.//')
export MAJOR_VER=$(echo "${TAG}" | sed -E 's/([0-9.]*)[_-]?(.*)/\1/')
export MINOR_VER=$(echo "${TAG}" | sed -E 's/([0-9.]*)[_-]?(.*)/\2/')
sed -i "s/SERIALNO=.*/SERIALNO=${MAJOR_VER}/g" release/src-rt/version.conf
sed -i "s/EXTENDNO=.*/EXTENDNO=${MINOR_VER}/g" release/src-rt/version.conf

cat release/src-rt/version.conf
