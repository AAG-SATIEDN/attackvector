#define LISTEN_PORT "8081" //local listening port

#define MAXLEN 3000 //max buffer size for recv()

#ifndef _WIN32
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#else
#define _WIN32_WINNT 0x501
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <cstdint>
#define u_int8_t uint8_t
#ifndef __MINGW32__
#define snprintf _snprintf_s
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "pthreadVC2.lib")
#endif
#define bcopy(b1,b2,len) (memmove((b2), (b1), (len)), (void) 0)
#endif
#include <pthread.h>
#include <iostream>
#include <stdlib.h>
#include <fstream>
using namespace std;
ofstream mylog("LOG");

/*void* get_in_addr(struct sockaddr* sa)
{
	return &(((struct sockaddr_in*)sa)->sin_addr;
}*/

const char * myinet_ntop4(const struct in_addr *addr, char *buf, socklen_t len) //for windows XP support
{
	const u_int8_t *ap = (const u_int8_t *)&addr->s_addr;
	char tmp[16]; // max length of ipv4 addr string
	int fulllen;
	
	/*
	 * snprintf returns number of bytes printed (not including NULL) or
	 * number of bytes that would have been printed if more than would
	 * fit
	 */
	fulllen = snprintf(tmp, sizeof(tmp), "%d.%d.%d.%d",
					   ap[0], ap[1], ap[2], ap[3]);
	if (fulllen >= (int)len) {
		return NULL;
	}
	
	bcopy(tmp, buf, fulllen + 1);
	
	return buf;
}
int sendall(int s, char *buf, int *len)
{
	int total = 0; // how many bytes we've sent
	int bytesleft = *len; // how many we have left to send
	int n=0;
	while(total < *len) {
		n = send(s, buf+total, bytesleft, 0);
		if (n == -1) { 
			perror("sending Error:");
#ifdef _WIN32
			cerr << WSAGetLastError() << "\n\n";
			mylog << "sending error:" << WSAGetLastError() << "\n";
#endif
			break; }
	total += n;
	bytesleft -= n;
	n=0;
	}
	*len = total; // return number actually sent here
	return n==-1?-1:0; // return -1 on failure, 0 on success
}
bool IsConnect(const char* msg, int len)
{
	char co[8]="CONNECT";
	int i=0;
	for(i=0; co[i]!='\0'; i++) {
		if(i>=len) return false;
		if(msg[i]!=co[i]) return false;
	}
	return true;
}
bool IsGet(const char* msg, int len)
{
	char co[4]="GET";
	int i=0;
	for(i=0; co[i]!='\0'; i++) {
		if(i>=len) return false;
		if(msg[i]!=co[i]) return false;
	}
	return true;
}
bool IsHead(const char* msg, int len)
{
	char co[5]="HEAD";
	int i=0;
	for(i=0; co[i]!='\0'; i++) {
		if(i>=len) return false;
		if(msg[i]!=co[i]) return false;
	}
	return true;
}
bool IsSiteGet(const char* msg, int len, const char* site)
{
	//assuming it is a GET request
	//if(!IsGet(msg, len)) return false;
	int i, j;
	for(i=11; msg[i]!='/' ; i++) if(i>=len) return false;
	i--;
	
	for(j=strlen(site)-1; j>=0 && i>=11; i--, j--) if(msg[i]!=site[j]) return false;
	return true;
}
bool IsSiteConnect(const char* msg, int len, const char* site)
{
	//assuming it is a CONNECT request
	//if(!IsConnect(msg, len)) return false;
	int i, j;
	for(i=8; msg[i]!=':' ; i++) if(i>=len) return false;
	i--;
	
	for(j=strlen(site)-1; j>=0 && i>=11; i--, j--) if(msg[i]!=site[j]) return false;
	return true;
}
bool IsSiteHead(const char* msg, int len, const char* site)
{
	//assuming it is a HEAD request
	//if(!IsHead(msg, len)) return false;
	int i, j;
	for(i=12; msg[i]!='/' ; i++) if(i>=len) return false;
	i--;
	
	for(j=strlen(site)-1; j>=0 && i>=12; i--, j--) if(msg[i]!=site[j]) return false;
	return true;
}
bool IsWantedGet(const char* msg, int len, char** sites, int sitesNo)
{
	if(!IsGet(msg, len)) return false;
	for(int i=0; i<sitesNo; i++){
		if(IsSiteGet(msg, len, sites[i])) return true;
	}
	return false;
}
bool IsWantedConnect(const char* msg, int len, char** sites, int sitesNo)
{
	if(!IsConnect(msg, len)) return false;
	for(int i=0; i<sitesNo; i++){
		if(IsSiteConnect(msg, len, sites[i])) return true;
	}
	return false;
}
bool IsWantedHead(const char* msg, int len, char** sites, int sitesNo)
{
	if(!IsHead(msg, len)) return false;
	for(int i=0; i<sitesNo; i++){
		if(IsSiteHead(msg, len, sites[i])) return true;
	}
	return false;
}
bool ModifyHeader(const char* from,const char* to, char* msg, int& len)
{
	//checking that msg starts with "from"
	int i, dif;
	for(i=0; from[i]!='\0'; i++)
		if(msg[i]!=from[i]) return false;
	//replacing. . .
	if(strlen(from)>=strlen(to))
	{
		dif = strlen(from) - strlen(to);
		len-=dif;
		for(i=0; i<strlen(to); i++)
			msg[i] = to [i];
		for(;i<len; i++) msg[i] = msg[i+dif];
	}
	if(strlen(to)>strlen(from))
	{
		dif = strlen(to) - strlen(from);
		len+=dif;
		//msg[len] = '\0';
		for(i=len-1; i>=strlen(to); i--)
			msg[i] = msg[i-dif];
		for(;i>=0; i--) msg[i] = to[i];
	}
	return true;
}
void ResolveDomainInGet(char* msg, int& len) //this function resolves domain name in GET request and substitutes it with its IP
{
	//assuming msg is a GET requset to a domain dn
	//getting domain name from request
	struct addrinfo hints, *res; //for getaddrinfo()
	struct sockaddr_in *ip; //to get the IP from results
	void* addr; //for help in casting
	char dn[255],ipstr[16],from[300],to[300]; //dn(for storing domain name), ipstr(for storing ip address), from and to are to be passed to ModifyHeader() to substitute the ip
	char get[] = "GET http://";
	int i, status;
	for(i=11; i<len && msg[i]!='/' && i<265; i++)
		dn[i-11] = msg[i];
	dn[i-11] = '\0'; //the domain name contained in the request is now stored in dn
	memset(&hints, 0 ,sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	for(i=0; i<5; i++){
		if(status=getaddrinfo(dn, NULL, &hints, &res) == 0) break;
		cerr << "DNS error!!\n";
		mylog << "DNS error!!\n";
		if(i<4){
			cerr << "Retrying. . .\n";
			mylog << "Retrying. . .\n";
		}
		else{
			cerr << "No DNS server available. . .\n";
			cerr << "this software won't be able to help you :(\n";
			cout << "if the issue persists, and you can open other websites, please report this issue. . .\n";
			cout << "I will fix that next release. . .\n\n";
			mylog << "No DNS server available. . .\n";
#ifdef _WIN32
			WSACleanup();
#endif
			exit(-1);
		}
	}
	ip = (struct sockaddr_in*) res->ai_addr;
	addr = &(ip->sin_addr);
	//inet_ntop(AF_INET, addr, ipstr, sizeof ipstr);
	myinet_ntop4(&(ip->sin_addr), ipstr, sizeof ipstr);
	freeaddrinfo(res);
	//IP of dn is now stored as a string in ipstr
	//buliding ModifyHeader() call to substitute the IP in the request
	for(i=0; get[i]!='\0'; i++){
		from[i] = get[i];
		to[i] = get[i];
	}
	for(i=0; dn[i]!='\0'; i++) from[i+11] = dn[i];
	from[i+11] = '\0';
	for(i=0; ipstr[i]!='\0'; i++) to[i+11] = ipstr[i];
	to[i+11] = '\0';
	ModifyHeader(from, to, msg, len);
}
void ResolveDomainInConnect(char* msg, int& len) //this function resolves domain name in CONNECT request and substitutes it with its IP
{
	//assuming msg is a CONNECT requset to a domain dn
	//getting domain name from request
	struct addrinfo hints, *res; //for getaddrinfo()
	struct sockaddr_in *ip; //to get the IP from results
	void* addr; //for help in casting
	char dn[255],ipstr[16],from[300],to[300]; //dn(for storing domain name), ipstr(for storing ip address), from and to are to be passed to ModifyHeader() to substitute the ip
	char connec[] = "CONNECT ";
	int i, status;
	for(i=8; i<len && msg[i]!=':' && i<265; i++)
		dn[i-8] = msg[i];
	dn[i-8] = '\0'; //the domain name contained in the request is now stored in dn
	memset(&hints, 0 ,sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	for(i=0; i<5; i++){
		if(status=getaddrinfo(dn, NULL, &hints, &res) == 0) break;
		cerr << "DNS error!!\n";
		mylog << "DNS error!!\n";
		if(i<4){
			cerr << "Retrying. . .\n";
			mylog << "Retrying. . .\n";
		}
		else{
			cerr << "No DNS server available. . .\n";
			cerr << "this software won't be able to help you :(\n";
			cout << "if the issue persists, and you can open other websites, please report this issue. . .\n";
			cout << "I will fix that next release. . .\n\n";
			mylog << "No DNS server available. . .\n";
#ifdef _WIN32
			WSACleanup();
#endif
			exit(-1);
		}
	}
	ip = (struct sockaddr_in*) res->ai_addr;
	addr = &(ip->sin_addr);
	//inet_ntop(AF_INET, addr, ipstr, sizeof ipstr);
	myinet_ntop4(&(ip->sin_addr), ipstr, sizeof ipstr);
	freeaddrinfo(res);
	//IP of dn is now stored as a string in ipstr
	//buliding ModifyHeader() call to substitute the IP in the request
	for(i=0; connec[i]!='\0'; i++){
		from[i] = connec[i];
		to[i] = connec[i];
	}
	for(i=0; dn[i]!='\0'; i++) from[i+8] = dn[i];
	from[i+8] = '\0';
	for(i=0; ipstr[i]!='\0'; i++) to[i+8] = ipstr[i];
	to[i+8] = '\0';
	ModifyHeader(from, to, msg, len);
}
void ResolveDomainInHead(char* msg, int& len) //this function resolves domain name in GET request and substitutes it with its IP
{
	//assuming msg is a GET requset to a domain dn
	//getting domain name from request
	struct addrinfo hints, *res; //for getaddrinfo()
	struct sockaddr_in *ip; //to get the IP from results
	void* addr; //for help in casting
	char dn[255],ipstr[16],from[300],to[300]; //dn(for storing domain name), ipstr(for storing ip address), from and to are to be passed to ModifyHeader() to substitute the ip
	char hea[] = "HEAD http://";
	int i, status;
	for(i=12; i<len && msg[i]!='/' && i<265; i++)
		dn[i-12] = msg[i];
	dn[i-12] = '\0'; //the domain name contained in the request is now stored in dn
	memset(&hints, 0 ,sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	for(i=0; i<5; i++){
		if(status=getaddrinfo(dn, NULL, &hints, &res) == 0) break;
		cerr << "DNS error!!\n";
		mylog << "DNS error!!\n";
		if(i<4){
			cerr << "Retrying. . .\n";
			mylog << "Retrying. . .\n";
		}
		else{
			cerr << "No DNS server available. . .\n";
			cerr << "this software won't be able to help you :(\n";
			cout << "if the issue persists, and you can open other websites, please report this issue. . .\n";
			cout << "I will fix that next release. . .\n\n";
			mylog << "No DNS server available. . .\n";
#ifdef _WIN32
			WSACleanup();
#endif
			exit(-1);
		}
	}
	ip = (struct sockaddr_in*) res->ai_addr;
	addr = &(ip->sin_addr);
	//inet_ntop(AF_INET, addr, ipstr, sizeof ipstr);
	myinet_ntop4(&(ip->sin_addr), ipstr, sizeof ipstr);
	freeaddrinfo(res);
	//IP of dn is now stored as a string in ipstr
	//buliding ModifyHeader() call to substitute the IP in the request
	for(i=0; hea[i]!='\0'; i++){
		from[i] = hea[i];
		to[i] = hea[i];
	}
	for(i=0; dn[i]!='\0'; i++) from[i+12] = dn[i];
	from[i+12] = '\0';
	for(i=0; ipstr[i]!='\0'; i++) to[i+12] = ipstr[i];
	to[i+12] = '\0';
	ModifyHeader(from, to, msg, len);
}
struct MyPack{
	struct addrinfo* resu;
	int fd;
	char** wanted;
	int wantedNo;
};

void* HandleClient(void* passing)
{
	//cerr << "opened thread\n";
	//bool tampered = false;
	char buf[MAXLEN];
	char** sites;
	int sitesNo;
	MyPack* passed = (MyPack*) passing;
	struct addrinfo *res = passed->resu;
	int server_fd, st, numbytes, maxfd;
	int client_fd = passed->fd;
	sites = passed-> wanted;
	sitesNo = passed-> wantedNo;
	fd_set readfd, master;
	FD_ZERO(&master);
	delete passed;
	server_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	maxfd = (((server_fd)>(client_fd))? (server_fd) : (client_fd));
	if((st=connect(server_fd, res->ai_addr, res->ai_addrlen))!=0){
		cout << "ERROR!!(connecting to remote proxy)\n" << gai_strerror(st) << "\n\n";
		cout << "Possible reasons:\n";
		cout << "* Wrong proxy address/port specified in \"CONFIG\" file, check that and try again.\n";
		cout << "* No network connection, check your connection and try again.\n";
		mylog << "ERROR!!(connecting to remote proxy)\n" << gai_strerror(st) << "\n\n";
#ifdef _WIN32
			WSACleanup();
#endif
		exit(1);
	}
	FD_SET(server_fd, &master);
	FD_SET(client_fd, &master);
	while(1)
	{
		readfd= master;
		if(select(maxfd+1, &readfd, NULL, NULL, NULL) <= 0)
		{
			cerr <<  "Select error!!!\n\n";
			mylog <<  "Select error!!!\n\n";
			break;
		}
		if(FD_ISSET(client_fd, &readfd)){ //if the message is from the client to the proxy server
			numbytes = recv(client_fd, buf, MAXLEN, 0);
			if(numbytes <= 0) break; //if connection is closed
			/* MESSAGE MANIPULATION GOES HERE */
			/*if(ModifyHeader("CONNECT www.youtube.com:443 HTTP/1.1", "CONNECT www.google.com:443 HTTP/1.1", buf, numbytes)){
				//tampered = true;
				cout << "Youtube access detected!\tStomache Cover\n\n";
			}
			if(ModifyHeader("CONNECT s.youtube.com:443 HTTP/1.1", "CONNECT video-stats.l.google.com:443 HTTP/1.1", buf, numbytes)){
				//tampered = true;
				cout << "Youtube video detected!\tStomache Cover\n\n";
			}
			if(ModifyHeader("CONNECT s2.youtube.com:443 HTTP/1.1", "CONNECT video-stats.l.google.com:443 HTTP/1.1", buf, numbytes)){
				//tampered = true;
				cout << "Youtube video detected!\tStomache Cover\n\n";
			}
			if(ModifyHeader("CONNECT youtube.com:443 HTTP/1.1", "CONNECT google.com:443 HTTP/1.1", buf, numbytes)){
				//tampered = true;
				cout << "Youtube access detected!\tStomache Cover\n\n";
			}*/
			if(IsWantedConnect(buf, numbytes, sites, sitesNo)){
				buf[numbytes] = '\0';
				mylog << buf << "\n\nAfter Resolving:\n";
				ResolveDomainInConnect(buf, numbytes);
				buf[numbytes] = '\0';
				mylog << buf << "\n\n\n";
				cout << "Request disguised stomache cover\n";
			}
			if(IsWantedGet(buf, numbytes, sites, sitesNo)){
				buf[numbytes] = '\0';
				mylog << buf << "\n\nAfter Resolving:\n";
				ResolveDomainInGet(buf, numbytes);
				buf[numbytes] = '\0';
				mylog << buf << "\n\n\n";
				//cout << buf << "\n\n";
			}
			if(IsWantedHead(buf, numbytes, sites, sitesNo)){
				buf[numbytes] = '\0';
				mylog << buf << "\n\nAfter Resolving:\n";
				ResolveDomainInHead(buf, numbytes);
				buf[numbytes] = '\0';
				mylog << buf << "\n\n\n";
				//cout << buf << "\n\n";
			}
			/*if(IsGet(buf,numbytes)){
				buf[numbytes] = '\0';
				mylog << buf << "\n\n";
			}
			if(IsConnect(buf,numbytes)){
				buf[numbytes] = '\0';
				mylog << buf << "\n\n";
			}*/
			//mylog << "new buf from client:\n";
			//buf[numbytes] = '\0';
			//mylog << buf << "\n\n";
			/* MESSAGE MANIPULATION ENDS HERE */
			if(sendall(server_fd, buf, &numbytes) == -1){
				cerr << "Sending error!!!\n";
				mylog << "Sending error!!!\n\n";
				cerr << "Closing Thread!!\n\n";
				break;
				//exit(-1);
			}
		}
		if(FD_ISSET(server_fd, &readfd)){ //if the message is from the proxy server to the client
			numbytes = recv(server_fd, buf, MAXLEN, 0);
			if(numbytes <= 0) break; //if connection is closed
			/* MESSAGE MANIPULATION GOES HERE */
			//mylog << "new buf from server:\n";
			//buf[numbytes] = '\0';
			//mylog << buf << "\n\n";
			/* MESSAGE MANIPULATION ENDS HERE */
			if(sendall(client_fd, buf, &numbytes) == -1){
				cerr << "Sending error!!!\n\n";
				mylog << "Sending error!!!\n\n";
				cerr << "Closing Thread!!";
				break;
				//exit(-1);
			}
			//cerr << ".";
		}
	}
#ifndef _WIN32
	close(server_fd);
	close(client_fd);
#else
	closesocket(server_fd);
	closesocket(client_fd);
#endif
	//cerr << "Closed thread\n";
	return NULL;
}

int main()
{
#ifdef _WIN32
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2,0), &wsaData) != 0) {
		fprintf(stderr, "WSAStartup failed.\n");
		exit(1);
	}
#endif
	int listener_fd; //listening socket for accepting new connections
	int newfd; //newly accepted fd
	int st; //to store errno returned from some function calls, used in bind
	char ** sites; //sites list input from CONFIG file, this is the list of sites that has to undergo stomache cover
	int sitesNo; //no of sites in the previous list, also input from CONFIG file, to make allocating memory easier
	char PARENT_HTTP_ADDR[255], PARENT_HTTP_PORT[6];
	int i;
	MyPack *toPass;
	ifstream fin("CONFIG");
	if(!fin){
		cout << "Error opening CONFIG file\n\n";
		mylog << "Error opening CONFIG file\n\n";
#ifdef _WIN32
			WSACleanup();
#endif
		exit(-1);
	}
	fin >> PARENT_HTTP_ADDR >> PARENT_HTTP_PORT;
	//struct sockaddr_storage remoteaddr; //client address for future development
	//socklen_t addrlen;
	pthread_t thread; //to store current thread
	struct addrinfo hints, *res;
	//assigning local listening port to res, to create a socket fd and bind it
	memset(&hints, 0, sizeof hints);
	hints.ai_family= AF_INET;
	hints.ai_socktype= SOCK_STREAM;
	//hints.ai_flags= AI_PASSIVE; //fill my ip for me (for accepting remote connections in future)
	getaddrinfo("127.0.0.1", LISTEN_PORT, &hints, &res);
	listener_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
#ifndef _WIN32
	int yes =1;
#else
	char yes ='1';
#endif
	if (setsockopt(listener_fd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int)) == -1) {
		cerr << "setsockopt";
		mylog << "setsockopt";
#ifdef _WIN32
			WSACleanup();
#endif
		exit(1);
	}
	fin >> sitesNo;
	sites = new char*[sitesNo];
	for(i=0; i<sitesNo; i++){
		sites[i] = new char[255];
		fin >> sites[i];
	}
	cout << "listening on 127.0.0.1:" << LISTEN_PORT << "\n";
	cout << "Set your browser to use this proxy\n\n";
	if((st=bind(listener_fd, res->ai_addr, res->ai_addrlen))!=0){ cerr << "FATAL ERROR!!(binding on local port 8081)\n" << gai_strerror(st) << "\n\n";
	mylog << "FATAL ERROR!!(binding on local port 8081)\n" << gai_strerror(st) << "\n\n";
#ifdef _WIN32
			WSACleanup();
#endif
	exit(1);}
	//socket created and bound successfully, free res variable to use it again
	freeaddrinfo(res);
	//now assigning remote proxy address and port to res to use it in every connection created to it
	memset(&hints, 0, sizeof hints);
	hints.ai_family= AF_INET;
	hints.ai_socktype= SOCK_STREAM;
	//hints.ai_flags= AI_PASSIVE;
	getaddrinfo(PARENT_HTTP_ADDR, PARENT_HTTP_PORT, &hints, &res);
	while(1)
	{
		if((st=listen(listener_fd, 10))!=0){ cerr << "FATAL ERROR!!(listening)\n" << gai_strerror(st) << "\n\n";
		mylog << "FATAL ERROR!!(listening)\n" << gai_strerror(st) << "\n\n";
#ifdef _WIN32
			WSACleanup();
#endif
		exit(1);}
		newfd= accept(listener_fd, NULL, NULL);
		toPass = new MyPack;
		toPass-> resu = res;
		toPass-> fd = newfd;
		toPass-> wanted = sites;
		toPass-> wantedNo = sitesNo;
		st = pthread_create(&thread, NULL, HandleClient, (void*)  toPass);
		if(st){
			cerr << "ERROR:\nUnable to create thread!!!\n\n";
			mylog << "ERROR:\nUnable to create thread!!!\n\n";
#ifdef _WIN32
			WSACleanup();
#endif
			exit(-1);
		}
	}
	cout << "UNSPECIFIED ERROR!!!!\nexiting. . .\n\n";
	mylog << "UNSPECIFIED ERROR!!!!\nexiting. . .\n\n";
	freeaddrinfo(res);
	for(i=0; i<sitesNo; i++){
		delete [] sites[i];
	}
	delete [] sites;
#ifndef _WIN32
	close(listener_fd);
#else
	closesocket(listener_fd);
	WSACleanup();
#endif
	pthread_exit(NULL);
}
