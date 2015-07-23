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
#include <pthread.h>

//Our files

using namespace std;
const unsigned MAXBUFLEN = 512;
pthread_mutex_t logout_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t login_lock = PTHREAD_MUTEX_INITIALIZER;

int sockfd;

void SendToServer(string msg){	
	//Format data 
	msg += '\4'; //End of transmission character to signify message over.

	int cpylen;
	int itr = 0;
	char buf[512];

	//Send data in 512 char chunks
	while(itr < msg.size()){
		if (itr + 512 < msg.size())
			cpylen = 512;
		else cpylen = msg.size() - 512;

		strncpy(buf, (&msg.c_str() + itr), cpylen);
		write(sockfd, buf, strlen(buf));

		itr += 512;
	}

	return;
}//*/

void *process_connection(void *arg) {
    int n;
    char buf[MAXBUFLEN];
    pthread_detach(pthread_self());
    while (1) {
		n = read(sockfd, buf, MAXBUFLEN);

		//Login
		if (buf[0] == '\16'){
			for (int i = 1; i < strlen(buf); i++)
				buf[i - 1] = buf[i];
			buf[n] = '\0';

	    	pthread_mutex_unlock(&logout_lock);
		}
		//Logout
		else if (buf[0] == '\3'){
			for (int i = 1; i < strlen(buf); i++)
				buf[i - 1] = buf[i];
			buf[n] = '\0';

	    	pthread_mutex_unlock(&logout_lock);
		}
		//Other valid message
		else if (n > 0){
			buf[n] = '\0';
			cout << buf << endl;
		}
		
		//Failstate
		if (n <= 0) {
		    if (n == 0) {
				cout << "Error: Server offline." << endl;
		    } else {
				cout << "An unexpected error has occured." << endl;
		    }
		    close(sockfd);

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
		fprintf(stderr, "cannot connect\n");
		exit(1);
    }

    pthread_create(&tid, NULL, &process_connection, NULL);

    string oneline;

    string uname, pass;

    pthread_mutex_lock(&login_lock);
    pthread_mutex_lock(&logout_lock);

    //Login sequence
    printf("Enter Username and Password to Login.");
    printf("Username: "); cin >> uname;

    if (uname != "guest"){
	    printf("Password: "); cin >> pass;
	    string line = "\6 " + uname + " " + pass;
    	SendToServer(line);
    }
    else{
    	SendToServer("\2");

    	while(!logout_lock){
    		printf("Enter Username and Password to Register.");
    		printf("Username: "); cin >> uname;
    		printf("Password: "); cin >> pass;

    		oneline = uname + " " + pass;
    		SendToServer(oneline);
    	}
    }
    

    if (login_lock)
	    while (1){
			getline(cin, oneline);
		    //write(sockfd, oneline.c_str(), oneline.length());
			SendToServer(oneline);

		    if (logout_lock){
		    	printf("Goodbye.\n");
		    	break;
		    }
	    }

    exit(0);
}
