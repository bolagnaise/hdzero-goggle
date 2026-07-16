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
APPLETS=$($BB --list-full 2> /dev/null)
if [ -z "$APPLETS" ]; then
    # guard against a future bundled busybox built without --list-full
    echo "ERROR: $BB --list-full returned nothing - applet links NOT installed"
    exit 1
fi
echo "$APPLETS" | while read -r applet; do
    [ -n "$applet" ] || continue

    # NEVER redirect early-boot-critical paths at /mnt/app: the kernel and
    # rcS need them BEFORE /mnt/app is mounted. A dangling /sbin/init or
    # /bin/sh permanently bricks the unit (recoverable only by reflashing
    # the OS - the links live on the rootfs and survive app updates).
    case "$applet" in
        linuxrc|bin/sh|bin/ash|bin/mount|bin/umount|bin/login|sbin/init|sbin/mdev|sbin/getty|sbin/sulogin|sbin/switch_root|sbin/reboot|sbin/halt|sbin/poweroff|sbin/watchdog) continue ;;
    esac

    # keep the old generated script's guard semantics: anything already
    # pointing at a busybox (including the rootfs busybox) is left alone
    case "$(readlink "/$applet" 2> /dev/null)" in
        *busybox*) continue ;;
    esac

    ln -sfn "$BB" "/$applet"
done
