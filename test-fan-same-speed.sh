#!/usr/bin/env bash
set -euo pipefail

# Simple test: toggle same-speed mode via DBus and verify worker logs
SVC=com.tuxedocomputers.uccd
OBJ=/com/tuxedocomputers/uccd
IFACE=com.tuxedocomputers.uccd

echo "This test script is deprecated — same-speed is now stored in profile settings."
echo "Edit a profile with the UCC UI and toggle 'Same fan speed for all fans', then apply the profile and check journalctl for fan behavior."