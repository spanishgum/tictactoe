/*
*  COP5570    |    Parallel, Concurrent, Distributed Programming
*  Asg #4     |    Tic Tac Toe Game Server
*  Summer C   |    07/24/15
*
*     by Adam Stallard, Steven Rohr
*
*/

#include <iostream>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <strings.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <netdb.h>
#include <string>
#include <cstring>
#include <pthread.h>
#include <vector>
#include <sstream>

#define _read(a, b, c) if(read((a), (b), (c)))
#define _write(a, b, c) if(write((a), (b), (c)))

using namespace std;

const unsigned MAXBUFLEN = 512;
int sockfd;
bool good = true;

vector<string> Split(string s){
	vector<string> v;
	stringstream ss;
	string e;
	ss << s;
	ss >> e;
	while (ss) {
		v.push_back(e);
		ss >> e;
	}
	return v;
}

void SendToServer(string msg){
	int cpylen = 0;
	int itr = 0;
	const int msize = msg.size() + 1;
	string subs;

	msg += "\4"; // EOT char to signify message over.

	//Send data in 512 char chunks
	while(itr < msize) {
		if ((itr + 512) < (int)msg.size())
			cpylen = 512;
		else
			cpylen = msg.size() - itr;

		subs = "";
		for (int i = itr; (i < itr + cpylen) && (i < msize); i++)
			subs += msg[i];

		_write(sockfd, subs.c_str(), subs.size());
		itr += 512;
	}

}

void *process_connection(void *arg) {
  int n;
  char buf[MAXBUFLEN];
  pthread_detach(pthread_self());

  while (1) {
		n = read(sockfd, buf, MAXBUFLEN);

		// Connected
		if (n > 0) {
			buf[n] = '\0';
			cout << buf << flush;
		}

		// Disconnected
		else {
	    if (n == 0) // logout
				cout << "You have been disconnected from the server.\n";
	    else // (n < 0) broken socket
				cerr << "An unexpected error has occured.\n";

	    close(sockfd);
  		good = false;
	    exit(1);
		}
  }
}

int main(int argc, char **argv) {
    int rv, flag;
    struct addrinfo hints, *res, *ressave;
    pthread_t tid;

    if (argc != 3) {
			cerr << "echo_client server_name_or_ip port" << endl;
			exit(1);
    }

    cout << argv[1] << " " << argv[2] << endl;

    bzero(&hints, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(argv[1], argv[2], &hints, &res)) != 0) {
			cerr << "Invalid address: " << gai_strerror(rv) << endl;
			exit(1);
    }

    ressave = res;
    flag = 0;
    do {
			sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
			if (sockfd < 0)
		    continue;
			if (connect(sockfd, res->ai_addr, res->ai_addrlen) == 0) {
		    flag = 1;
		    break;
			}
			close(sockfd);
    } while ((res = res->ai_next) != NULL);

    freeaddrinfo(ressave);

    if (flag == 0) {
			cerr << "Cannot connect to server.\n";
			exit(1);
    }

    pthread_create(&tid, NULL, &process_connection, NULL);

    string uname;
    cout << "Enter username and password to login.\nUsername: ";
    while (good) {
    	getline(cin, uname);
			SendToServer(uname);
    }

    exit(0);
}
