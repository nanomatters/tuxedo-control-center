#!/bin/bash

# In case TFC service is active, deactivate
systemctl stop tuxedofancontrol > /dev/null 2>&1 || true
systemctl disable tuxedofancontrol > /dev/null 2>&1 || true

DIST_DATA=/opt/tuxedo-control-center/resources/dist/tuxedo-control-center/data/dist-data

rm /usr/share/applications/tuxedo-control-center.desktop || true
cp ${DIST_DATA}/tuxedo-control-center.desktop /usr/share/applications/tuxedo-control-center.desktop || true

mkdir -p /etc/skel/.config/autostart || true
cp ${DIST_DATA}/tuxedo-control-center-tray.desktop /etc/skel/.config/autostart/tuxedo-control-center-tray.desktop || true

cp ${DIST_DATA}/com.uniwill.uccd.policy /usr/share/polkit-1/actions/com.uniwill.uccd.policy || true
cp ${DIST_DATA}/com.uniwill.uccd.conf /usr/share/dbus-1/system.d/com.uniwill.uccd.conf || true

cp ${DIST_DATA}/com.uniwill.tomte.policy /usr/share/polkit-1/actions/com.uniwill.tomte.policy || true

# Copy and enable services
cp ${DIST_DATA}/uccd.service /etc/systemd/system/uccd.service || true
cp ${DIST_DATA}/uccd-sleep.service /etc/systemd/system/uccd-sleep.service || true
systemctl daemon-reload
systemctl enable uccd uccd-sleep
systemctl restart uccd

# set up udev rules
mv ${DIST_DATA}/99-webcam.rules /etc/udev/rules.d/99-webcam.rules
udevadm control --reload-rules && udevadm trigger

# ---
# Original electron-builder after-install.tpl
# ---
ln -sf '/opt/tuxedo-control-center/tuxedo-control-center' '/usr/bin/tuxedo-control-center' || true

# SUID chrome-sandbox for Electron 5+
chmod 4755 '/opt/tuxedo-control-center/chrome-sandbox' || true

update-mime-database /usr/share/mime || true
update-desktop-database /usr/share/applications || true
