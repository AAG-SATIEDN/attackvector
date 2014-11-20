import com.trilead.ssh2.* from: lib/trilead-ssh2-build213.jar;

sub ssh_connect
{
   local('$conn $sess $data $handle @data $pass');

   # create a connection
   $conn = [new Connection: $host, 22];
   [$conn connect];

   # authenticate
   if ($user ne "" && $pass ne "") 
   {
      try 
      {     
         [$conn authenticateWithPassword: $user, $pass];
      }
      catch $exception 
      {
         $conn = [new Connection: $host, 22];
         [$conn connect];
         [$conn authenticateWithPassword: $user, ""];
      }
   }
   else
   {
      [$conn authenticateWithPublicKey: $user, [new java.io.File: $key], $null];
   }

   # execute the command
   $sess = [$conn openSession];
   return %(\$sess, \$conn);
}

sub ssh_exec {
   local('$sess $handle');
   $sess = $1['$sess'];

   [$sess execCommand: $2];
   $handle = [SleepUtils getIOHandle: [$sess getStdout], [$sess getStdin]];
   $1['$handle'] = $handle;

   return $handle;
}

sub ssh_close {
   sleep(1000);
   local('$sess $conn');
   $sess = $1['$sess'];
   $conn = $1['$conn'];   

   closef($1['$handle']);
   [$sess close];
   [$conn close];
}

