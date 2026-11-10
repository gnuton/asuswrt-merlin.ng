#!/bin/sh
# Keep the DDNS checkip-command and its stdout contract; C owns STUN children.
exec realip --script "$@"
