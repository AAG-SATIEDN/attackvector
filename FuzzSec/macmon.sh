#!/bin/bash

#Save all monitor interfaces in a list
################################################################################
xterm -geometry 0x0 -e "iwconfig |grep "Mode:Monitor" >> /tmp/mon.txt" & wait

#Menu
################################################################################
echo    "-----------------------------------------------"
echo    "*            MacMon  v1.5 -- b33f             *"
echo    "*           -Have a anonymous day-            *"
echo    "-----------------------------------------------"
echo    "*"
echo    "* (1) Random MAC - Monitor Mode"
echo    "* (2) Set specific MAC - Monitor Mode"
echo    "* (3) Display list of vendor specific MAC"
echo    "*     [The vendor-search is case sensitive]"
echo    "* (4) Reverse look-up MAC address"
echo    "*"
echo -n "* Select option (1/2/3): "
read -e MACMON
echo    "-----------------------------------------------"

#(1) Random MAC -> MON [All monitor interfaces will be taken down!!]
################################################################################
if [ $MACMON = 1 ]; then

airmon-ng |sed 's/^/* /'
echo    "-----------------------------------------------"
echo    "*"
echo -n "* Select Interface: "
read -e IFACE
echo    "*"
echo    "-----------------------------------------------"
echo    "*"

for list in $(cat /tmp/mon.txt |cut -d" " -f1); do
airmon-ng stop $list &>/dev/null
done
rm /tmp/mon.txt &>/dev/null

echo    "* [>] Setting device to monitor mode"
airmon-ng stop $IFACE &>/dev/null
airmon-ng start $IFACE &>/dev/null

wait

ifconfig $IFACE down
ifconfig mon0 down

echo    "* [>] Faking MAC"
echo    "*"
macchanger -r $IFACE |sed 's/^/* /'
macchanger -r mon0 |sed 's/^/* /'

ifconfig $IFACE up
ifconfig mon0 up
echo    "*"
echo    "* [>] Done"
echo    "-----------------------------------------------"

#(2) Specific MAC -> MON [All monitor interfaces will be taken down!!]
################################################################################
elif [ $MACMON = 2 ]; then

airmon-ng |sed 's/^/* /'
echo    "-----------------------------------------------"
echo    "*"
echo -n "* Select Interface: "
read -e IFACE
echo -n "* Define the MAC: "
read -e MAC
echo    "*"
echo    "-----------------------------------------------"
echo    "*"

for list in $(cat /tmp/mon.txt |cut -d" " -f1); do
airmon-ng stop $list &>/dev/null
done
rm /tmp/mon.txt &>/dev/null

echo    "* [>] Setting device to monitor mode"
airmon-ng stop $IFACE &>/dev/null
airmon-ng start $IFACE &>/dev/null

wait

ifconfig $IFACE down
ifconfig mon0 down

echo    "* [>] Faking MAC"
echo    "*"
macchanger --mac=$MAC $IFACE |sed 's/^/* /'
macchanger --mac=$MAC mon0 |sed 's/^/* /'

ifconfig $IFACE up
ifconfig mon0 up
echo    "*"
echo    "* [>] Done"
echo    "-----------------------------------------------"

#(3) Vendor specific MAC lookup
################################################################################
elif [ $MACMON = 3 ]; then

echo    "*"
echo -n "* Search for vendor name: "
read -e VENDOR

xterm -hold -wf -e "macchanger --list=$VENDOR"&

echo    "*"
echo    "* [>] Done"
echo    "-----------------------------------------------"


#(3) Reverce lookup MAC
################################################################################
elif [ $MACMON = 4 ]; then

echo -n "* Enter MAC: "
read -e SMAC

SMAC=$(echo "$SMAC" | sed 's/:/-/g')
SMAC=$(echo "$SMAC" | cut -c1-8)
export XMAC=`grep -i $SMAC /usr/local/etc/aircrack-ng/airodump-ng-oui.txt`

echo    "*"
echo    "* $XMAC"
echo    "-----------------------------------------------"
fi