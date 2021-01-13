#!/bin/bash
set -e

# DSL-AC68U
# Run this script after having reviewed the merge

BUILD_NAME=$1

echo "*** Adding files to GIT after a GPL Merge..."
if [ "$BUILD_NAME" == "DSL-AC68U" ]
then
    git add -f ../release/src/router/bwdpi_source/asus_sql/prebuild/DSL-AC68U/*
    git add -f ../release/src/router/bwdpi_source/prebuild/DSL-AC68U/*
    git add -f ../release/src/router/cfg_mnt/prebuild/DSL-AC68U/*
    git add -f ../release/src/router/dblog/daemon/prebuild/DSL-AC68U/*
    git add -f ../release/src/router/emf_arm/*
    git add -f ../release/src/router/et_arm/prebuilt/*
    git add -f ../release/src/router/httpd/prebuild/DSL-AC68U/*
    git add -f ../release/src/router/libletsencrypt/prebuild/DSL-AC68U/*
    git add -f ../release/src/router/lighttpd-1.4.39/prebuild/DSL-AC68U/*
    git add -f ../release/src/router/networkmap/prebuild/DSL-AC68U/*
    git add -f ../release/src/router/nt_center/actMail/prebuild/DSL-AC68U/*
    git add -f ../release/src/router/nt_center/lib/prebuild/DSL-AC68U/*
    git add -f ../release/src/router/nt_center/prebuild/DSL-AC68U/*
    git add -f ../release/src/router/rc/prebuild/DSL-AC68U/*
    git add -f ../release/src/router/sambaclient/prebuild/DSL-AC68U/*
    git add -f ../release/src/router/shared/prebuild/DSL-AC68U/*
    git add -f ../release/src/router/sw-hw-auth/prebuild/DSL-AC68U/*
    git add -f ../release/src/router/wb/prebuild/DSL-AC68U/*
    git add -f ../release/src/router/wl_arm/prebuilt/DSL-AC68U/*
    git add -f ../release/src/router/wlceventd/prebuilt/DSL-AC68U/*
    git add -f ../release/src/router/wlconf_arm/prebuilt/*
    git add -f ../release/src/router/wps_arm/*
    git add -f ../release/src/router/bwdpi_source/asus/prebuild/DSL-AC68U/*
    git add -f ../release/src/router/bwdpi_source/asus/prebuild/DSL-AC68U/*
    git add -f ../release/src/router/asusnatnl/natnl/prebuild/DSL-AC68U/*
    git add -f ../release/src/router/asd/prebuild/DSL-AC68U/*
    git add -f ../release/src/router/amas-utils/prebuild/DSL-AC68U/*
    git add -f ../release/src/router/ahs/prebuild/DSL-AC68U/*
    git add -f ../release/src/router/acsd_arm/prebuilt/*
    git add -f ../release/src/router/aaews/prebuild/*
else
    echo "Unrecognized BUILD_NAME!"
    exit
fi



