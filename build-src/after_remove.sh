#!/bin/bash

# Stop, disable and remove services
systemctl disable uccd uccd-sleep || true
systemctl stop uccd || true
rm /etc/systemd/system/uccd.service || true
rm /etc/systemd/system/uccd-sleep.service || true
systemctl daemon-reload || true

# Remove log and config files if pure uninstall
if [ "$1" = "remove" ] || [ "$1" = "purge" ] || [ "$1" = "0" ]; then
    rm -rf /var/log/tcc/ || true
    rm -rf /var/log/tccd/ || true
    rm -rf /etc/tcc/ || true
fi

# Remove policy kit and desktop files
rm /usr/share/polkit-1/actions/com.uniwill.uccd.policy || true
rm /usr/share/polkit-1/actions/com.uniwill.tomte.policy || true
rm /usr/share/applications/tuxedo-control-center.desktop || true
rm /etc/skel/.config/autostart/tuxedo-control-center-tray.desktop || true
rm /usr/share/dbus-1/system.d/com.uniwill.uccd.conf || true

# remove udev rule
rm /etc/udev/rules.d/99-webcam.rules || true

# Delete the link to the binary
rm -f '/usr/bin/tuxedo-control-center'
