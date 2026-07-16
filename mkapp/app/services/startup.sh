#!/bin/sh

if [ ! -f /mnt/app/services/scripts/install/install.done ]; then
	# First boot after a flash/OTA. The cheap symlink installers (dropbear,
	# dosfstools, tinycurl, bearssl - a dozen idempotent ln/mkdir calls)
	# run immediately so their binaries exist for early consumers (SD
	# repair, ssh); only the fork-heavy busybox applet pass is deferred
	# off the boot-critical path and run at lowest priority. install.done
	# is only written after every installer SUCCEEDED - failures leave it
	# absent so the (idempotent) pass retries on the next boot.
	(
		NICE="nice -n 19"
		command -v nice > /dev/null 2>&1 || NICE=""
		ok=1
		for FILE in /mnt/app/services/scripts/install/*.sh
		do
			case "$FILE" in *10_busybox.sh) continue ;; esac
			echo "Installing service: $FILE"
			$NICE /bin/sh "$FILE" || ok=0
		done
		sleep 20
		echo "Installing service: 10_busybox.sh"
		$NICE /bin/sh /mnt/app/services/scripts/install/10_busybox.sh || ok=0
		[ "$ok" = "1" ] && touch /mnt/app/services/scripts/install/install.done
	) &
fi

for FILE in $(ls /mnt/app/services/scripts/runtime/)
do
	echo "Starting service: $FILE"
	/mnt/app/services/scripts/runtime/$FILE &
done
