/*
*  COP5570    |    Parallel, Concurrent, Distributed Programming
*  Asg #4     |    Tic Tac Toe Game Server
*  Summer C   |    07/24/15
*
*     by Adam Stallard, Steven Rohr
*
*/


//C++ Libraries
#include <iostream>
#include <stdio.h> // added for stderr
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

//Our files

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
	while (ss){
		v.push_back(e);
		ss >> e;
	}
	return v;
}

void SendToServer(string msg){	
	//Format data 
	msg += "\4"; //End of transmission character to signify message over.

	int cpylen;
	int itr = 0;
	string buf;

	//Send data in 512 char chunks
	while(itr < msg.size()){
		if (itr + 512 < msg.size())
			cpylen = 512;
		else cpylen = msg.size() - 512;

		//strncpy(buf, (&msg.c_str() + itr), cpylen);
		buf = msg.substr(itr, itr + cpylen);
		write(sockfd, buf.c_str(), strlen(buf.c_str()));

		itr += 512;
	}//*/
	//write(sockfd, msg.c_str(), strlen(msg.c_str()));

	return;
}//*/

void *process_connection(void *arg) {
    int n;
    char buf[MAXBUFLEN];
    pthread_detach(pthread_self());
    while (1) {
		n = read(sockfd, buf, MAXBUFLEN);

		//Connected
		if (n > 0){
			buf[n] = '\0';
			cout << buf << flush;//<< "Recieved message from server: \"" << buf << "\"\n";
		}
		
		//Disconnected
		else {
			//One of these is logout, not an error. Find out which.
		    if (n == 0) {
				cout << "You have been disconnected from the server." << endl;
		    } 
		    else {
				cout << "An unexpected error has occured." << endl;
		    }
		    close(sockfd);
    		good = false;

		    exit(1);
		}	
    }
}

//const unsigned serv_port = 5100;

int main(int argc, char **argv) {
    int rv, flag;
    ssize_t n;
    struct addrinfo hints, *res, *ressave;
    pthread_t tid;

    if (argc != 3) {
		cout << "echo_client server_name_or_ip port" << endl;
		exit(1);
    }

    cout << argv[1] << " " << argv[2] << endl;

    bzero(&hints, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(argv[1], argv[2], &hints, &res)) != 0) {
		cout << "getaddrinfo wrong: " << gai_strerror(rv) << endl;
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
		fprintf(stderr, "Cannot connect to server.\n");
		exit(1);
    }

    pthread_create(&tid, NULL, &process_connection, NULL);

    string oneline;

    string uname, pass;

    cout << "Enter username and password to login.\nUsername: ";
    while (good){
    	getline(cin, oneline);
    	//printf("Sending data = %s\n", oneline.c_str());
		SendToServer(oneline);
    }

    printf("Logged out.\n");

    exit(0);
}
