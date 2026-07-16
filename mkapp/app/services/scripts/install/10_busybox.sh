#!/bin/sh

BB=/mnt/app/services/busybox/busybox

# Link every applet the shipped busybox provides. --list-full prints the
# canonical install path per applet (bin/ls, usr/bin/head, ...), and
# ln -sfn is idempotent, so the old generated script with a per-applet
# `readlink | grep` guard (two extra processes per applet, ~770 forks per
# install pass) is unnecessary — and this list can never drift from the
# binary again. Note busybox's own `--install -s` is NOT equivalent: it
# refuses to overwrite existing non-link binaries, which the old script
# deliberately did.
$BB --list-full | while read -r applet; do
    [ -n "$applet" ] && ln -sfn "$BB" "/$applet"
done
