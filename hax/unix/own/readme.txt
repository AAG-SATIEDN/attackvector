to use this:

1. edit ownthem.sh to include the right IPs
2. generate a linux/shell_reverse_tcp payload to point to your host, save it to resources/backdoor
3. edit resources/callback.sh to call back to your host
4. edit resources/credentials.txt if you choose to add more or less credentials
5. generate an ssh key and stick id_dsa and id_dsa.pub in resources/ 
   (once the box is owned, the rooter uses the key to connect again)

Run with ./ownthem.sh

You can verify initial success when a copy of /etc/shadow is saved into the output with the IP of the host.
