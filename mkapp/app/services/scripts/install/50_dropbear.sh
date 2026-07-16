#!/bin/sh

ln -sfn /mnt/app/services/dropbear/dropbearmulti /bin/dropbear
ln -sfn /mnt/app/services/dropbear/dropbearmulti /bin/scp
ln -sfn /mnt/app/services/dropbear/dropbearmulti /bin/ssh
ln -sfn /mnt/app/services/dropbear/dropbearmulti /bin/dropbearkey

# Host keys are generated lazily: dropbear is started with -R (see
# page_wifi.c), which creates them on the first client connection instead
# of burning 1-2s of CPU on RSA keygen during the first-boot install pass.
mkdir -p /etc/dropbear
