# ASUS Merlin firmware for DSL-AC68U

[![CircleCI](https://circleci.com/gh/gnuton/asuswrt-merlin.ng/tree/dsl-ac68u.svg?style=svg)](https://circleci.com/gh/gnuton/asuswrt-merlin.ng/tree/dsl-ac68u)

## What is this repo for?
This repo holds code and builds of Asus Merlin for the ASUS DSL-AC68U

## Why?
Asus DSL-AC68U is not officially supported by Asus Merlin; So this is my attempt to make happy people having this router and willing to run the great Merlin custom firmware on it.

## Any chance to get official support?
I dunno. If  Asus keeps on giving us up-to-date GPL source code, if these builds get tested enough and last but not least the asus merlin dev accept to support this new router, maybe we can see an official support.

## Where can I find the binaries to flash on my router?
You can find them on the release page of github

## Where can I find the code for the DSL-AC68U?
Please have a look at the dsl branch

## What's the status of this project?
It's an experimental project, since it has not been extensively tested yet. If you find any issue please report it in github. 

## Can I trust the binaries?
I do not give you any warranty, but the building process happens entirely on the cloud so what you download is what you get just from github, so no funny business.

## How can I contribute?
Support asus merlin, send me patch if it's just for DSL-AC68U, tell me you are interested in this project but starring it on github.

## Why do you use circleCI to build this firmware?
Because we are in 201x and Circle CI is an excellent CI solution which builds and create releases almost automatically and in a trasparent way.

## Why do you build This firmware in a docker image?
Because it make the build faster since it doesn't need to install toolchains every time and because it makes the build reproducible everywhere even not in the cloud.

## Big thanks
- to Asus for the GPL releases
- Asus Merlin devs
