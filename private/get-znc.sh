##### Get IP addresses & ports
ourIP=`netstat -antp | grep "/sshd" | awk '{print $5}' | awk -F ":" '{print $1}' | grep -v 0.0.0.0 | uniq | sed '/^$/d'`
serverIP=`curl --silent http://ifconfig.me`
ircport=""
sshport=""
##### !!!sensitive data!!!
ircpass=""
 
 
 
##### Install IRC BNC (ZNC)
#apt-cache search znc
#apt-cache show znc
apt-get -y build-dep znc
cd /usr/local/src/
apt-get -y install git
git clone git://github.com/znc/znc.git
cd znc/
apt-get -y install automake
./autogen.sh
./configure --enable-extra
make -s
make -s install
#whereis znc
adduser --disabled-password --gecos "" bouncer
apt-get -y install expect
echo -e '#!/usr/bin/expect\nset timeout 5\nspawn "znc" "--makeconf"\nexpect "What port would you like ZNC to listen on" { send "'$ircport'\\r" }\nexpect "Would you like ZNC to listen using SSL" { send "yes\\r" }\nexpect "Would you like to create a new pem file now" { send "yes\\r" }\nexpect "Would you like ZNC to listen using ipv6" { send "no\\r" }\nexpect "Listen Host" { send "\\r" }\nexpect "Load global module" { send "no\\r" } # party_line\nexpect "Load global module" { send "no\\r" } # webadmin\nexpect "Username" { send "freenode\\r" }\nsleep 1\nexpect "Enter Password" { send "'$ircpass'\\r" }\nsleep 1\nexpect "Confirm Password" { send "'$ircpass'\\r" }\nsleep 1\nexpect "Would you like this user to be an admin" { send "yes\\r" }\nexpect "Nick" { send "g0tmi1k\\r" }\nexpect "Alt Nick" { send "\\r" }\nexpect "Ident" { send "\\r" }\nexpect "Real Name" { send "Got Milk?\\r" }\nexpect "Bind Host" { send "\\r" }\nexpect "Number of lines to buffer per channel" { send "999\\r" }\nexpect "Would you like to clear channel buffers after replay" { send "yes\\r" }\nexpect "Default channel modes" { send "\\r" }\nexpect "Load module" { send "yes\\r" } #chansaver\nexpect "Load module" { send "yes\\r" } #controlpanel\nexpect "Load module" { send "no\\r" } #perform\nexpect "Load module" { send "no\\r" } #webadmin\nexpect "Would you like to set up a network" { send "yes\\r" }\nexpect "Network" { send "freenode\\r" }\nexpect "Load module" { send "yes\\r" } # chansaver\nexpect "Load module" { send "yes\\r" } # keepnick\nexpect "Load module" { send "yes\\r" } # kickrejoin\nexpect "Load module" { send "yes\\r" } # nickserv\nexpect "Load module" { send "no\\r" } #perform\nexpect "Load module" { send "yes\\r" } #simple_away\nexpect "IRC server (host only)" { send "irc.freenode.net\\r" }\nexpect "Port" { send "7000\\r" }\nexpect "Password" { send "\\r" }\nexpect "Does this server use SSL" { send "yes\\r" }\nexpect "Would you like to add another server for this IRC network" { send "no\\r" }\nexpect "Would you like to add a channel for ZNC to automatically join" { send "yes\\r" }\nexpect "Channel name" { send "#backtrack-linux\\r" }\nexpect "Would you like to add another channel" { send "no\\r" }\nexpect "Would you like to set up another network" { send "no\\r" }\nexpect "Would you like to set up another user" { send "no\\r" }' > /home/bouncer/ZNC.exp
rm -rf /home/bouncer/.znc/znc.pem
rm -rf /home/bouncer/.znc/configs/znc.conf
pkill -SIGUSR1 znc
pkill znc
su bouncer -c '/usr/bin/expect /home/bouncer/ZNC.exp'
#--- Run
su bouncer -c '/usr/local/bin/znc'
#--- Create cron job
cp -n /etc/crontab{,.bkup}
echo -e '\n*/10 *\t* * *\tbouncer\t/usr/local/bin/znc >/dev/null 2>&1' >> /etc/crontab
#--- Remove expect
rm -f /home/bouncer/ZNC.exp
apt-get -y remove expect
