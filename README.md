# ASUS Merlin firmware for DSL-AC68U

[![CircleCI](https://circleci.com/gh/gnuton/asuswrt-merlin.ng/tree/dsl-ac68u.svg?style=svg)](https://circleci.com/gh/gnuton/asuswrt-merlin.ng/tree/dsl-ac68u)

Latest release:
![GitHub release](https://img.shields.io/github/release/gnuton/asuswrt-merlin.ng.svg) 
![GitHub Release Date](https://img.shields.io/github/release-date/gnuton/asuswrt-merlin.ng.svg)
![GitHub Releases](https://img.shields.io/github/downloads/gnuton/asuswrt-merlin.ng/latest/total.svg)

All releases:
![GitHub All Releases](https://img.shields.io/github/downloads/gnuton/asuswrt-merlin.ng/total.svg)

Latest Merlin firmware version: ![GitHub release](https://img.shields.io/github/tag/RMerl/asuswrt-merlin.ng.svg)


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

## Can you please add/unlock this X feature?
The Merlin firmware is popular for its quality and stability and not for thoudands of features. In any case for fun I can release some snapshots with unlocked features. Here below you can find the current ones:
* REPEATER MODE - status: experimental, it works for me, but still a bit rough around the edges


## How can I contribute?
Support asus merlin, send me patch if it's just for DSL-AC68U, tell me you are interested in this project by starring/forking it on github.

### Why do you use circleCI to build this firmware?
Because we are in 201x and Circle CI is an excellent CI solution which builds and create releases almost automatically and in a trasparent way.

### Why do you build This firmware in a docker image?
Because it makes the build faster since it doesn't need to install toolchains every time and because it makes the build reproducible everywhere even not in the cloud.

### Hey man I forked your repo and...
#### How can I build Asus Merlin for DSL-AC68U or other routers by myself?
TODO

#### Hey man the code is not up-to-date, how can I update it by myself?
In case I have not done it yet, ping me or just please feel free to do it and send me a PR.

#### BTW Here are the steps to update the merlin upstream code and the GPL pre-builds and files:
1. Merge the Merlin.ng mainline in the dsl-ac68u branch using git
```bash
   ========================
   // Clone gnuton repo and setup upstream
   git clone git@github.com:gnuton/asuswrt-merlin.ng.git
   cd asuswrt-merlin.ng/
   git remote add upstream git@github.com:RMerl/asuswrt-merlin.ng.git
   git fetch upstream

   // Update mainline branch
   git checkout -b mainline origin/mainline
   git rebase upstream/mainline
   git push origin


   // Update DSL
   git checkout -b dsl-ac68u origin/dsl-ac68u
   git branch dsl-ac68u-test
   git checkout dsl-ac68u-test
   git merge mainline
   // Fix conflicts if any
   git commit -m "a message" -a
   // Push to the cloud and let the CI build it for you
   git push origin dsl-ac68u-test
   // if everything fine merge the changes to dsl-ac68u branch
   git checkout dsl-ac68u
   git rebase dsl-ac68u-test
   git push --delete origin dsl-ac68u-test && git branch -d dsl-ac68u-test
   ========================
```
2. Download the latest DSL-AC68U firmware from https://www.asus.com/Networking/DSLAC68U/HelpDesk_Download/
3. uncompress it
4. cd dir_where_asuswrt_gpl_code_is
5. export MERLIN_SRC=path_to_dsl-ac68u_git_working_copy
6. run $MERLIN_SRC/tools/tools/copy-prebuilt
7. git commit just the modified files belonging to DSL-AC68U
8. build and test it

#### I setup circleci for my repo how can I create releases?
CircleCI has bee configured so that it create release for stable releases and pre-releases for unstable ones.

##### Unstable builds or pre-releases
CircleCI builds snapshots from commit tagged with gnuton-snapshot-.*. So if you add a new feature that you wanna test, you can build the pre-release with

```bash
# Create and push tag -> this trigger the pre-release creation
git tag gnuton-snapshot-my-feature
git push --tag

# If you make some changes and you wanna update the pre-release
... make your changes and commit them ...
git push
git tag gnuton-snapshot-my-feature -f
git push --tag -f

# when you do not need the release/tag animore you can remove it from github and from the repo
git tag -d gnuton-snapshot-my-feature
git push --delete origin gnuton-snapshot-my-feature
```
##### Stable builds or releases
Stable builds are triggered by tags matching this pattern. [0-9]+.*gnuton. (eg: 380.0-gnuton1)
```
// To create and push the tag in order to build the release
git tag 390.0-gnuton1
git push --tag
```
## Where can I get support for this build
Mail me or follow https://www.snbforums.com/threads/finally-got-merlin-fw-to-work-on-dsl-ac68u.48702/

## Big thanks
- to Asus for the GPL releases
- Asus Merlin devs
