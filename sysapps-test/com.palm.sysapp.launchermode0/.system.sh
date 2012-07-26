#!/bin/sh

/usr/sbin/rootfs_open -w
wait 2

mv /usr/share/systemsounds/appclose.pcm /media/internal
mv /usr/share/systemsounds/carddrag.pcm /media/internal

cp /usr/palm/applications/com.palm.sysapp.launchermode0/.00.pcm /usr/share/systemsounds/appclose.pcm
cp /usr/palm/applications/com.palm.sysapp.launchermode0/.01.pcm /usr/share/systemsounds/carddrag.pcm
sync
reboot
