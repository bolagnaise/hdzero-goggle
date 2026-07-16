#!/bin/sh

source /mnt/app/app/record/record-env.sh
/mnt/app/app/record/gogglecmd -rec quit
/mnt/app/app/record/gogglecmd -sds quit

#poll umount instead of a fixed 'sleep 2' + single try: it succeeds the
#moment record/sdstat have released the card (typically well under 1s),
#with a 5s cap
echo "Umounting SD Card"
UMOUNTED=0
i=0
while [ $i -lt 50 ]; do
    if umount /mnt/extsd 2> /dev/null; then
        UMOUNTED=1
        break
    fi
    usleep 100000
    i=$((i + 1))
done
if [ "$UMOUNTED" = "1" ]; then
    echo "Umounting SD Card: SUCCESS"
else
    echo "Umounting SD Card: FAILURE"
fi

BLKDEV=/dev/mmcblk0p1
if [ ! -b "$BLKDEV" ]; then
    BLKDEV=/dev/mmcblk0
fi

rm -f /tmp/fsck.result
if [ "$UMOUNTED" = "1" ]; then
    /bin/fsck.fat -y "$BLKDEV" > /tmp/fsck.log 2>&1
    RESULT=$?
else
    #never fsck a mounted filesystem - the old script did exactly that
    #whenever its fixed sleep wasn't long enough for umount to succeed,
    #risking corruption of a card that was fine
    echo "SD card busy, skipping check" > /tmp/fsck.log
    RESULT=0
fi
echo "fsck result: $RESULT" >> /tmp/fsck.log
echo $RESULT > /tmp/fsck.result

if [ "$UMOUNTED" = "1" ]; then
    echo "Mounting SD Card"
    mount "$BLKDEV" /mnt/extsd
    if [ $? -eq 0 ]; then
        echo "Mounting SD Card: SUCCESS"
    else
        echo "Mounting SD Card: FAILURE"
    fi
fi
sleep 1

/mnt/app/app/record/record &
/mnt/app/app/record/sdstat &
