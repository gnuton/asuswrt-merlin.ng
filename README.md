# ASUS Merlin firmware for DSL-AC68U

[![CircleCI](https://circleci.com/gh/gnuton/asuswrt-merlin.ng/tree/dsl-ac68u.svg?style=svg)](https://circleci.com/gh/gnuton/asuswrt-merlin.ng/tree/dsl-ac68u)

## What is this repo for?
This repo hosts the code and builds of Asus Merlin for the ASUS DSL-AC68U

## Why?
Asus DSL-AC68U is not officially supported by Asus Merlin; So this is my attempt to make happy people having this router and willing to run the great Merlin custom firmware on it.

## Any chance to get official Merlin support?
I dunno. If  Asus keeps on giving us up-to-date GPL source code, if these builds get tested enough and last but not least the asus merlin dev accept to support this device, maybe we can see an official support.
In the worst case you can find support here :D

## Where can I find the firmware to flash on my router?
You can find all the binaries in the release page of this github repo.

## How can I flash this firmware on my DSL-AC68U?
Download the latest firmware from the relase page then browse your router web page and upload it from there.

## Where can I find the code for the DSL-AC68U?
Please have a look at the dsl-ac68u branch. This code is based on the Merlin mainline branch.

## What's the status of this project?
As for now it's in an experimental state, but it should works fine or at least it does for me.

## Can this firmware damage my router?
It should not, but I do not take any responsability if that happens or if you f*ck up something. 

## Can I trust the binaries?
The building process entirely happens on the cloud. Hence what you download from the release page is just what you see in this github.

## How can I contribute?
Support asus merlin, send me patch if it's just for DSL-AC68U, tell me you are interested in this project by starring/forking it on github.

## Why do you use circleCI to build this firmware?
Because we are in 201x and Circle CI is an excellent CI solution which builds and create releases almost automatically and in a trasparent way.

## Why do you build This firmware in a docker image?
Because it make the build faster since it doesn't need to install toolchains every time and because it makes the build reproducible everywhere even not in the cloud.

## How can I build Asus Merlin for DSL-AC68U or other routers by myself?
TODO

## Hey man the code is not up-to-date, how can I do it by myself?
In case I have not done it yet, ping me or just please feel free to do it and send me a PR.

BTW Here are the steps to perform this action:
1. Merge the Merlin.ng mainline in the dsl-ac68u branch
2. Download the latest DSL-AC68U firmware from https://www.asus.com/Networking/DSLAC68U/HelpDesk_Download/
3. uncompress it
4. cd dir_where_asuswrt_gpl_code_is
5. export MERLIN_SRC=path_to_dsl-ac68u_git_working_copy
6. run $MERLIN_SRC/tools/tools/copy-prebuilt
7. git commit just the modified files belonging to DSL-AC68U
8. build and test it

## Big thanks
- to Asus for the GPL releases
- Asus Merlin devs
