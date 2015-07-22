/*
*  COP5570    |    Parallel, Concurrent, Distributed Programming
*  Asg #4     |    Tic Tac Toe Game Server
*  Summer C   |    07/24/15
*
*     by Adam Stallard, Steven Rohr
*
*/


#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <pthread.h>
#include <sstream>

#include <talk.h>

using namespace std;

//const unsigned port = 5100;
const unsigned MAXBUFLEN = 512;
pthread_mutex_t accept_lock = PTHREAD_MUTEX_INITIALIZER;
int serv_sockfd;
extern bool Parse(string, user);
extern vector<user> users;

void *one_thread(void *arg) {
    int cli_sockfd;
    struct sockaddr_in cli_addr;
    socklen_t sock_len;
    ssize_t n;
    char buf[MAXBUFLEN];

    int tid = *((int *)arg);
    free(arg);

    cout << "thread " << tid << " created" << endl;

    //TODO: Need to figure out who this is. 
    //Maybe put it in the handshake? User sends uid or something.
    user * usr;

    for (;;) {
        bool logout = false;
    	sock_len = sizeof(cli_addr);
    	pthread_mutex_lock(&accept_lock);
    	cli_sockfd = accept(serv_sockfd, (struct sockaddr *)&cli_addr, &sock_len);
    	pthread_mutex_unlock(&accept_lock);

    	//cout << "thread " << tid << ": ";
    	//cout << "remote client IP == " << inet_ntoa(cli_addr.sin_addr);
    	//cout << ", port == " << ntohs(cli_addr.sin_port) << endl;

        string cmd = "";

    	while (!logout && (n = read(cli_sockfd, buf, MAXBUFLEN)) > 0) {
    	    //buf[n] = '\0';
    	    //cout << buf << endl;
    	    //write(cli_sockfd, buf, strlen(buf));

            //Login sequence
            if (buf[0] == '\6'){
                usr = NULL;

                string uname, psswrd;
                bool nousr = true;
                stringstream ss;

                for (int i = 1; i < n; i++)
                    ss << buf[i];
                ss >> uname >> psswrd;

                for (int i = 0; i < users.size(); i++)
                    if (users[i].name == uname){
                        nousr = false;
                        if (users[i].passwd == psswrd){
                            usr = users[i];
                            sprintf(buf, "You are now logged in as %s.\r\n", users[i].name);
                            *(usr).cli_sockfd = cli_sockfd
                            write(cli_sockfd, buf, strlen(buf));
                        }
                        else{
                            buf = "Incorrect password.\r\n"
                            write(cli_sockfd, buf, strlen(buf));
                        }
                        break;
                    }

                if (nousr){
                    buf = "No such user.\r\n"
                    write(cli_sockfd, buf, strlen(buf));
                }
                //else printf("%s has logged in.\n", *(usr).name);
            }
            //Guest sequence
            else if (buf[0] == '\2'){
                //Enter guest mode.
                while (!logout && (n = read(cli_sockfd, buf, MAXBUFLEN)) > 0) {
                    if (buf[n - 1] == '\4'){
                        //Commit message
                        cmd += buf;
                        cmd[cmd.size() - 1] = '\0'; //Overwrite EOT with NULL so that printf doesn't have a stroke.
                        //Parse(cmd, *usr);
                        vector<string> = Split(cmd);
                        if (v[0] == "register"){
                            bool okay = true;
                            if (v.size() < 3){
                                okay = false;
                                buf = "Format: register <username> <password>\r\n"
                                write(cli_sockfd, buf, strlen(buf));
                            }
                            else{
                                if (v[1].size() == 0 || v[1].size() > 15){
                                    okay = false;
                                    buf = "Username must be between 1 and 15 characters.\r\n"
                                    write(cli_sockfd, buf, strlen(buf));
                                }
                                if (v[2].size() == 0 || v[2].size() > 20){
                                    okay = false;
                                    buf = "Password must be between 1 and 20 characters.\r\n"
                                    write(cli_sockfd, buf, strlen(buf));
                                }
                            }

                            if (okay){
                                for (int i = 0; i < users.size(); i++){
                                    if (users[i] == v[1]){
                                        okay = false;
                                        buf = "Username has been taken. Please try another.\r\n"
                                        write(cli_sockfd, buf, strlen(buf));
                                        break;
                                    }
                                }
                                if (okay){
                                    user u(v[1], v[2]);
                                    users.push_back(u);

                                    buf = "You have been registered. You may now exit and login.\r\n"
                                    write(cli_sockfd, buf, strlen(buf));
                                }
                            }
                        }
                        else if (v[0] == "quit" || v[0] == "exit" || v[0] == "bye"){
                            buf = "Goodbye.\r\n"
                            write(cli_sockfd, buf, strlen(buf));
                            logout = true;
                        }
                        else if (v[0] == "fuck" || v[0] == "shit"){
                            buf = "No swearing!\r\n"
                            write(cli_sockfd, buf, strlen(buf));
                        }
                        cmd = "";
                    }
                    else{
                        //Portion of message
                        cmd += buf;
                    }
                    
                    //Register user v[1] with password v[2] 
                }
            }
            //Logged in user talking to us
            else if (usr != NULL){
                if (buf[n - 1] == '\4'){
                    //Commit message
                    cmd += buf;
                    cmd[cmd.size() - 1] = '\0'; //Overwrite EOT with NULL so that printf doesn't have a stroke.
                    logout = Parse(cmd, *usr);
                    cmd = "";
                }
                else{
                    //Portion of message
                    cmd += buf;
                }
            }
            /*else{
                printf("Recieved bad message (no user).\n");
            }//*/
    	}


    	if (n == 0) {
            if (usr != NULL)
    	       printf("%s has logged out.\n", *(usr).name);
            //else printf("No user connected.\n");
    	} 
        else {
    	    printf("An unexpected error has occured.\n");
    	}

    	close(cli_sockfd);
    }
}

int main(int argc, char *argv[]) {
    struct sockaddr_in serv_addr;
    int port, number_thread;
    pthread_t tid;
    int *tid_ptr;

    if (argc != 3) {
    	fprintf(stderr, "%s port number_thread\n", argv[0]);
    	exit(1);
    }

    port = atoi(argv[1]);
    number_thread = atoi(argv[2]);

    cout << "port = " << port << " number of threads == " <<
	number_thread << endl;
    serv_sockfd = socket(AF_INET, SOCK_STREAM, 0);

    bzero((void*)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(port);

    bind(serv_sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

    listen(serv_sockfd, 5);

    int i;
    for (i = 0; i < number_thread; ++i) {
    	tid_ptr = (int *)malloc(sizeof(int));
    	*tid_ptr = i;
    	pthread_create(&tid, NULL, &one_thread, (void *)tid_ptr);
    }

    for (;;)
	pause();
}
