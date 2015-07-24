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


#include "talk.h"

#define _read(a, b, c) if(read((a), (b), (c)))
#define _write(a, b, c) if(write((a), (b), (c)))


using namespace std;

//const unsigned port = 5100;
const unsigned MAXBUFLEN = 512;
pthread_mutex_t accept_lock = PTHREAD_MUTEX_INITIALIZER;
int serv_sockfd;
extern bool Parse(string, user);
extern vector<user> users;

pthread_mutex_t users_lock = PTHREAD_MUTEX_INITIALIZER;

string ClientGet(int cli_sockfd){
    int n;
    string str = "";
    char buf[512];

    while((n = read(cli_sockfd, buf, MAXBUFLEN)) > 0){ /***** added the > 0 part, test to make sure this is okay ***/
        if (buf[n] == '\4'){
            buf[n] = '\0';
            str += buf;
            return str;
        }
        else{
            str += buf;
        }
    }
    return str;
}

void *one_thread(void *arg) {
    int cli_sockfd;
    struct sockaddr_in cli_addr;
    socklen_t sock_len;
    ssize_t n = 0; /*  added = 0 because n could be used uninitialized below */
    char buf[MAXBUFLEN];

    int tid = *((int *)arg);
    free(arg);
    bool logout;
    int ccounter;

    string msg;

    cout << "thread " << tid << " created" << endl;

    //TODO: Need to figure out who this is.
    //Maybe put it in the handshake? User sends uid or something.
    user * usr;

    for (;;) {
        ccounter = 0;
    	sock_len = sizeof(cli_addr);
    	pthread_mutex_lock(&accept_lock);
    	cli_sockfd = accept(serv_sockfd, (struct sockaddr *)&cli_addr, &sock_len);
    	pthread_mutex_unlock(&accept_lock);

    	//cout << "thread " << tid << ": ";
    	//cout << "remote client IP == " << inet_ntoa(cli_addr.sin_addr);
    	//cout << ", port == " << ntohs(cli_addr.sin_port) << endl;

        string cmd = "";

        usr = NULL;

        string uname, psswrd;
        bool nousr = true;
        stringstream ss;
        logout = false;

        bool loggedin = false;


        //for (int i = 1; i < n; i++)
        //    ss << buf[i];
        //ss >> uname;



	   do {
    	    //buf[n] = '\0';
    	    //cout << buf << endl;
    	    //write(cli_sockfd, buf, strlen(buf));
            if (loggedin){
                cout << "User is logged in. Awaiting input.\n";


                //Logged in user talking to us
                if (usr != NULL){
                    if (buf[n - 1] == '\4'){
                        buf[n] = '\0';

                        //Commit message
                        cmd += buf;
                        cmd[cmd.size() - 1] = '\0'; //Overwrite EOT with NULL so that printf doesn't have a stroke.
                        logout = Parse(cmd, *usr);
                        cmd = "";
                        memset(buf,0,strlen(buf));
                    }
                    else{
                        buf[n] = '\0';

                        //Portion of message

                        cmd += buf;
                        memset(buf,0,strlen(buf));
                    }

                    if (logout)
                        break;
                }
            }
            else{
                cout << "User attempting to log in!\n";
                //msg = "Enter username and password to log in.\r\nUsername: ";
                //write(cli_sockfd, msg.c_str(), strlen(buf));
                n = read(cli_sockfd, buf, MAXBUFLEN);
                if (n > 0)
                    buf[n] = '\0';
                else {
                    cout << "Error reading from user.\nExiting.\n";
                    close(cli_sockfd);
                    exit(0);
                }

                uname = buf;
                uname = uname.substr(0, uname.size() - 1);
                memset(buf,0,strlen(buf));

                //uname = ClientGet(cli_sockfd);

                //cout << "Recieved username = \"" << uname.c_str() << "\"\n";
                /*for (int i = 0; i < uname.size(); i++){
                    cout << "\tuname[" << i << "] = \'" << uname[i] << "\'\n";
                }//*/

                if (strcmp(uname.c_str(), "guest") != 0){
                    //Login

                    msg = "Password: ";
                    _write(cli_sockfd, msg.c_str(), strlen(msg.c_str()));

                    cout << "Not registration sequence.\n";

                    n = read(cli_sockfd, buf, MAXBUFLEN);

                    if (n > 0)
                        buf[n] = '\0';
                    else {
                        cout << "Error reading from user.\nExiting.\n";
                        close(cli_sockfd);
                        //exit(0);
                    }
                    buf[n] = '\0';
                    psswrd = buf;
                    psswrd = psswrd.substr(0, psswrd.size() - 1);
                    memset(buf,0,strlen(buf));
                    //psswrd = ClientGet(cli_sockfd);//buf;

                    for (unsigned int i = 0; i < users.size() && !loggedin; i++)
                        if (users[i].name == uname){
                            nousr = false;
                            if (users[i].passwd == psswrd){
                                usr = &users[i];
                                msg = "You are now logged in as " + uname + "\n";

                                pthread_mutex_lock(&users_lock);
                                usr->cli_sockfd = cli_sockfd;
                                usr->online = true;
                                pthread_mutex_unlock(&users_lock);


                                _write(cli_sockfd, msg.c_str(), strlen(msg.c_str()));
                                loggedin = true;
                            }
                            else{
                                msg = "Incorrect password.\n";
                                _write(cli_sockfd, msg.c_str(), strlen(msg.c_str()));
                                logout = true;
                            }
                            break;
                        }

                    if (nousr){
                        msg = "No such user.\n";
                        _write(cli_sockfd, msg.c_str(), strlen(msg.c_str()));
                        logout = true;
                    }
                }
                else{
                    cout << "Registration sequence.\n";
                    msg = "You are now in registration mode.\n";
                    _write(cli_sockfd, msg.c_str(), strlen(msg.c_str()));
                    while (!logout && (n = read(cli_sockfd, buf, MAXBUFLEN)) > 0) {
                        cout << "User entered: \"" << buf << "\"\n";

                        //Registration
                        if (buf[n - 1] == '\4'){
                            buf[n] = '\0';
                            //string tmp;
                            //tmp = "";
                            //tmp = buf;
                            //tmp = tmp.substr(0, tmp.size() - 1);
                            //strcpy(buf, tmp.c_str());

                            //Commit message
                            cmd += buf;
                            cmd[cmd.size() - 1] = '\0'; //Overwrite EOT with NULL so that printf doesn't have a stroke.
                            //Parse(cmd, *usr);
                            vector<string> v = Split(cmd);
                            for (unsigned int i = 0; i < v.size(); i++){
                                cout << "v[" << i << "] = " << v[i] << " (" << v[i].size() << ")\n";
                            }

                            if (v[0] == "register"){
                                cout << "User attempting registration.\n";
                                bool okay = true;
                                if (v.size() != 3){
                                    okay = false;
                                    msg = "Format: register <username> <password>\n";
                                    _write(cli_sockfd, msg.c_str(), strlen(msg.c_str()));
                                }
                                else{
                                    if (v[1].size() == 0 || v[1].size() > 15){
                                        okay = false;
                                        msg = "Username must be between 1 and 15 characters.\n";
                                        _write(cli_sockfd, msg.c_str(), strlen(msg.c_str()));
                                    }
                                    if (v[2].size() == 0 || v[2].size() > 20){
                                        okay = false;
                                        msg = "Password must be between 1 and 20 characters.\n";
                                        _write(cli_sockfd, msg.c_str(), strlen(msg.c_str()));
                                    }
                                }

                                if (okay){
                                    for (unsigned int i = 0; i < users.size(); i++){
                                        if (users[i].name == v[1]){
                                            okay = false;
                                            msg = "Username has been taken. Please try another.\n";
                                            _write(cli_sockfd, msg.c_str(), strlen(msg.c_str()));
                                            break;
                                        }
                                    }
                                    if (okay){
                                        user u;
                                        u.name = v[1];
                                        u.passwd = v[2];
                                        users.push_back(u);

                                        msg = "You have been registered. You may now exit and login.\n";
                                        _write(cli_sockfd, msg.c_str(), strlen(msg.c_str()));
                                    }
                                }
                            }
                            else if (v[0] == "quit" || v[0] == "exit" || v[0] == "bye"){
                                msg = "Goodbye.\r\n";
                                _write(cli_sockfd, msg.c_str(), strlen(msg.c_str()));
                                logout = true;
                            }
                            else if (v[0] == "?" || v[0] == "help"){
                                cout << "User requesting help.\n";
                                msg = "Register: register <username> <password>\nQuit: [quit|exit]\n";
                                _write(cli_sockfd, msg.c_str(), strlen(msg.c_str()));
                            }
                            else if (v[0] == "fuck" || v[0] == "shit"){
                                msg = "Don't swear!\r\n";
                                _write(cli_sockfd, msg.c_str(), strlen(msg.c_str()));
                            }
                            else{
                                msg = "Invalid command detected. In guest mode you may only register or quit.\n";
                                _write(cli_sockfd, msg.c_str(), strlen(msg.c_str()));
                            }
                            cmd = "";
                        }
                        else{
                            //Portion of message
                            cmd += buf;
                        }
                        memset(buf,0,strlen(buf));
                        //Register user v[1] with password v[2]
                    }
                }
            }

            if (loggedin){
                stringstream ss;
                ss << "< telnet " << ccounter << " > ";
                ccounter++;

                msg = ss.str();
                _write(cli_sockfd, msg.c_str(), strlen(msg.c_str()));
            }
        } while (loggedin && !logout && ((n = read(cli_sockfd, buf, MAXBUFLEN)) > 0));

    	if (n == 0) {
            if (usr != NULL){
                usr->online = false;
                printf("%s has logged out.\n", usr->name.c_str());
                usr = NULL;
            }
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

    user test;
    test.name = "test";
    test.passwd = "test";
    users.push_back(test);

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

    for (;;){
        string line;
        getline(cin, line);
        if (line == "quit"){
            //Do shutdown stuff
            break;
        }
    }
	cout << "Server shutting down.\n";
    //pause();
    return 0;
}
