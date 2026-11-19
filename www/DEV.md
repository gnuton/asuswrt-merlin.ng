<p align="center">
    <img src="http://nw-dlcdnet.asus.com/plugin/productIcons/DSL-AX82U.png" alt="logo" width="200" height="150">
</p>

<h3 align="center">
    <a href="https://github.com/RMerl/asuswrt-merlin/wiki/About-Asuswrt">AsusWrt Merlin</a> 
    for 
    DSL Asus Routers
</h3>

<p align="center">
  A powerful third party alternative firmware.
  <br>
  <a href="https://github.com/RMerl/asuswrt-merlin.ng/wiki/About-Asuswrt"><strong>Explore Asus Merlin docs»</strong></a>
  <br>
  <br>
  <a href="https://github.com/Asuswrt-Merlin/asuswrt-merlin.ng-3004.gnuton/releases/latest">Download latest firmware</a>
  ·
  <a href="https://github.com/Asuswrt-Merlin/asuswrt-merlin.ng-3004.gnuton/issues/new?assignees=&labels=enhancement&template=feature_request.md">Request feature</a>
  ·
  <a href="https://github.com/Asuswrt-Merlin/asuswrt-merlin.ng-3004.gnuton/issues/new?assignees=&labels=bug&template=bug_report.md">Report bug</a>
  ·
  <a href="https://www.snbforums.com/threads/gnutons-merlin-builds-for-dsl-router-386-1_2-released.70980/">Support forum</a>
  ·
  <a href="https://gitter.im/asuswrt/merlin-dsl">Chat</a>
</p>

---------------------

## I am a Dev or a Tester!!!

If you are a dev o a tester you are welcome!
In this page you will find all info needed to build the firmware in your local machine or on the cloud and more

## Build the firmware
To build the firmware on your local machine please you need to follow these steps:

```bash
git clone git@github.com:Asuswrt-Merlin/asuswrt-merlin.ng-3004.gnuton.git
cd asuswrt-merlin.ng-3004.gnuton
git checkout -b gnuton-master origin/gnuton-master

docker run -it --rm -v "$PWD:/build" gnuton/asuswrt-merlin-toolchains-docker:latest-ubuntu-20_04 /bin/bash

#RT/DSL-AX82U
cd release/src-rt-5.02axhnd.675x
make dsl-ax82u

#DSL-AC68U
cd release/src-rt-6.x.4708
make dsl-ac68u

# TROUBLESHOOTING
# If you have cloned repository outside docker you may need to run the following commands
# to avoid permission denied errors
# 1. get your uid and gid for your user in the host
host$ id
uid=12345(antonio) gid=67890(users)

# 2. Craete the users group in docker
docker$ groupadd  -g 678980 users
# 3. Create the user
docker$ useradd myuser -u 12345 -g 67890 -m -s /bin/bash
```

## Branching
### Master branch
* [gnuton-master](https://github.com/Asuswrt-Merlin/asuswrt-merlin.ng-3004.gnuton/tree/gnuton-master) is the MASTER BRANCH. All stable and pre-releases are built from here.

### DEV_* branches
DEV prefixed branches are temporary feature branches. They can come and go any time. Eventually they may get merged to dsl-master.
Currently these are the most important ones:
* REPEATER MODE - Adds all operational wireless modes available to RT corrispective routers but not losing the ability to work as DSL router too. [Download Image](https://github.com/Asuswrt-Merlin/asuswrt-merlin.ng-3004.gnuton/releases/tag/gnuton-snapshot-feature-repeater)

### ARCHIVED_* branches -
These branches are and won't be actively maintained anymore, they are mostly based on old 384 firmware. 
* [OLD REPEATER MODE](https://github.com/Asuswrt-Merlin/asuswrt-merlin.ng-3004.gnuton/tree/ARCHIVED-dsl-feature-repeater) 
* [IP SEC](https://github.com/Asuswrt-Merlin/asuswrt-merlin.ng-3004.gnuton/tree/dsl-feature-ipsec)
