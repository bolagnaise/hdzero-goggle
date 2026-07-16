#!/bin/sh

if [ ! -f /mnt/app/services/scripts/install/install.done ]; then
	# First boot after a flash/OTA: defer the install pass until well after
	# the app has reached video, and run it at lowest priority so its forks
	# don't compete with bring-up on this small SoC. Installers run
	# sequentially, honoring their numeric prefixes. install.done is only
	# written after every installer has FINISHED - it used to be touched
	# while they were still running in the background, so a power cut
	# mid-install left the services half-installed forever.
	(
		sleep 20
		for FILE in /mnt/app/services/scripts/install/*.sh
		do
			echo "Installing service: $FILE"
			nice -n 19 /bin/sh "$FILE"
		done
		touch /mnt/app/services/scripts/install/install.done
	) &
fi

for FILE in $(ls /mnt/app/services/scripts/runtime/)
do
	echo "Starting service: $FILE"
	/mnt/app/services/scripts/runtime/$FILE &
done
