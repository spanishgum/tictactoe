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
#include <stdio.h>
#include <pthread.h>
#include <sstream>
#include "talk.h"
#include "archive.h"

#define _read(a, b, c) if(read((a), (b), (c)))
#define _write(a, b, c) if(write((a), (b), (c)))

using namespace std;

const unsigned MAXBUFLEN = 512;

pthread_mutex_t accept_lock = PTHREAD_MUTEX_INITIALIZER;
extern pthread_mutex_t games_lock;
extern pthread_mutex_t users_lock;

int serv_sockfd;

extern bool Parse(string, user&);

extern vector<user> users;

static bool exit_sequence = false;


string ClientGet(int cli_sockfd) {
  int n;
  string str = "";
  char buf[512];

  while((n = read(cli_sockfd, buf, MAXBUFLEN)) > 0) {
    if (buf[n] == '\4') {
      buf[n] = '\0';
      str += buf;
      return str;
    }
    else
      str += buf;
  }
  return str;
}


void *scheduled_maintenance(void *arg) {
  time_t stopwatch = 0, abs_time = time(NULL);
  cerr << "Server maintenance thread initiated.\n";

  do {
    pthread_mutex_lock(&accept_lock);
    if (exit_sequence) break;
    pthread_mutex_unlock(&accept_lock);

    stopwatch += difftime(time(NULL), abs_time);
    abs_time = time(NULL);

    if (stopwatch > 300) {
      pthread_mutex_lock(&games_lock);
      pthread_mutex_lock(&users_lock);
      cerr << "Initiating scheduled server backup.\n";
      save_server();
      cerr << "Backup complete.\n";
      pthread_mutex_lock(&games_lock);
      pthread_mutex_lock(&users_lock);
      stopwatch = 0;
    }
    sleep(25);
  } while (1);

  cerr << "Server maintenance thread exiting.\n";
  pthread_exit(0);
}


void *one_thread(void *arg) {
  int cli_sockfd;
  struct sockaddr_in cli_addr;
  socklen_t sock_len;
  int tid = *((int *)arg);
  free(arg);

  ssize_t n = 0;
  char buf[MAXBUFLEN];
  string msg;

  for (;;) {

  	sock_len = sizeof(cli_addr);
  	pthread_mutex_lock(&accept_lock);
  	cli_sockfd = accept(serv_sockfd, (struct sockaddr *)&cli_addr, &sock_len);
  	pthread_mutex_unlock(&accept_lock);

  	cout << "\nthread " << tid << ": \n"
  	  << "remote client IP == " << inet_ntoa(cli_addr.sin_addr)
  	  << ", port == " << ntohs(cli_addr.sin_port) << "\n\n";

    user * usr = NULL;
    stringstream ss;
    string uname, psswrd, cmd = "";
    bool nousr = true, logout = false, loggedin = false;

    do {

      if (loggedin) {
        cout << "Thread " << tid << ": Recieved " << n
          << " characters from user " << usr->name << endl << flush;
        cout << "Thread " << tid
          << ": User is logged in. Awaiting input.\n";

        pthread_mutex_lock(&users_lock);
        for (unsigned int i = 0; i < users.size(); i++)
          if (users[i].name == uname) {
            usr = &users[i];
            break;
          }
        pthread_mutex_unlock(&users_lock);

        // Logged in user talking to us
        if (usr != NULL) {
          if (buf[n - 1] == '\4') {
            buf[n] = '\0';

            // Commit message
            cmd += buf;
            cmd[cmd.size() - 1] = '\0'; // Overwrite EOT with NULL so that printf doesn't have a stroke.
            logout = Parse(cmd, *usr);
            cmd = "";
            memset(buf, 0, strlen(buf));
          }
          else {
            buf[n] = '\0';
            //Portion of message
            cmd += buf;
            memset(buf, 0, strlen(buf));
          }

          if (logout) break;
        }
      }
      else {
        cout << "Thread " << tid << ": User attempting to log in!\n";
        n = read(cli_sockfd, buf, MAXBUFLEN);
        if (n > 0)
          buf[n] = '\0';
        else {
          cerr << "User exited before logging in.\n";
          break;
        }

        uname = buf;
        uname.pop_back();
        memset(buf, 0, strlen(buf));

        if (uname.compare("guest") != 0) {
          // Login - check password
          msg = "Password: ";
          _write(cli_sockfd, msg.c_str(), strlen(msg.c_str()));

          cout << "Client attempting login.\n";
          n = read(cli_sockfd, buf, MAXBUFLEN);
          if (n > 0)
            buf[n] = '\0';
          else {
            cerr << "Error reading from client. Closing connection\n";
            break;
          }

          buf[n] = '\0';
          psswrd = buf;
          psswrd = psswrd.substr(0, psswrd.size() - 1);
          memset(buf, 0, strlen(buf));

          pthread_mutex_lock(&users_lock);
          for (unsigned int i = 0; i < users.size() && !loggedin; i++) {
            if (users[i].name == uname) {
              nousr = false;

              if (users[i].passwd == psswrd) {
                usr = &users[i];
                msg = "You are now logged in as " + uname + "\n";

                users[i].cli_sockfd = cli_sockfd;
                users[i].online = true;
                users[i].clientcounter = 0;

                if(SendToClient(*usr, msg));
                loggedin = true;
              }
              else {
                msg = "Incorrect password.\n";
                _write(cli_sockfd, msg.c_str(), strlen(msg.c_str()));
                logout = true;
              }

              break;
            }
          }
          pthread_mutex_unlock(&users_lock);

          if (nousr) {
            msg = "No such user.\n";
            _write(cli_sockfd, msg.c_str(), strlen(msg.c_str()));
            logout = true;
          }
        }
        else {
          cout << "Registration sequence.\n";
          msg = "You are now in registration mode.\n";
          _write(cli_sockfd, msg.c_str(), strlen(msg.c_str()));
          while (!logout && (n = read(cli_sockfd, buf, MAXBUFLEN)) > 0) {
            cout << "User entered: \"" << buf << "\"\n";

            //Registration
            if (buf[n - 1] == '\4') {
              buf[n] = '\0';

              //Commit message
              cmd += buf;
              cmd[cmd.size() - 1] = '\0'; //Overwrite EOT with NULL so that printf doesn't have a stroke.
              vector<string> v = Split(cmd);
              for (unsigned int i = 0; i < v.size(); i++)
                cout << "v[" << i << "] = " << v[i] << " (" << v[i].size() << ")\n";

              if (v[0] == "register") {
                cout << "User attempting registration.\n";
                bool okay = true;
                if (v.size() != 3) {
                  okay = false;
                  msg = "Format: register <username> <password>\n";
                  _write(cli_sockfd, msg.c_str(), strlen(msg.c_str()));
                }
                else {
                    if (v[1].size() == 0 || v[1].size() > 15) {
                      okay = false;
                      msg = "Username must be between 1 and 15 characters.\n";
                      _write(cli_sockfd, msg.c_str(), strlen(msg.c_str()));
                    }
                    if (v[2].size() == 0 || v[2].size() > 20) {
                      okay = false;
                      msg = "Password must be between 1 and 20 characters.\n";
                      _write(cli_sockfd, msg.c_str(), strlen(msg.c_str()));
                    }
                }

                if (okay) {
                  pthread_mutex_lock(&users_lock);
                  for (unsigned int i = 0; i < users.size(); i++){
                    if (users[i].name == v[1]){
                      okay = false;
                      msg = "Username has been taken. Please try another.\n";
                      _write(cli_sockfd, msg.c_str(), strlen(msg.c_str()));
                      break;
                    }
                  }
                  if (okay) {
                    user u;
                    u.name = v[1];
                    u.passwd = v[2];
                    u.online = false;
                    users.push_back(u);

                    msg = "You have been registered. You may now exit and login.\n";
                    _write(cli_sockfd, msg.c_str(), strlen(msg.c_str()));
                  }
                  pthread_mutex_unlock(&users_lock);
                }
              }
              else if (v[0] == "quit" || v[0] == "exit" || v[0] == "bye") {
                msg = "Goodbye.\r\n";
                _write(cli_sockfd, msg.c_str(), strlen(msg.c_str()));
                logout = true;
              }
              else if (v[0] == "?" || v[0] == "help") {
                cout << "User requesting help.\n";
                msg = "Register: register <username> <password>\nQuit: [quit|exit]\n";
                _write(cli_sockfd, msg.c_str(), strlen(msg.c_str()));
              }
              else if (v[0] == "fuck" || v[0] == "shit") {
                msg = "Don't swear!\r\n";
                _write(cli_sockfd, msg.c_str(), strlen(msg.c_str()));
              }
              else {
                msg = "Invalid command detected. In guest mode you may only register or quit.\n";
                _write(cli_sockfd, msg.c_str(), strlen(msg.c_str()));
              }
              cmd = "";
            }
            else {
              //Portion of message
              cmd += buf;
            }
            memset(buf,0,strlen(buf));
            //Register user v[1] with password v[2]
          }
        }
      }

    } while (loggedin && !logout && ((n = read(cli_sockfd, buf, MAXBUFLEN)) > 0));

  	if (n == 0) {
      if (usr != NULL) {
        usr->online = false;
        cout << usr->name << " has logged out.\n";
        usr = NULL;
      }
  	}
    else
	    cerr << "Getting new connection.\n";

  	close(cli_sockfd);
    pthread_mutex_lock(&accept_lock);
    if (exit_sequence) break;
    pthread_mutex_unlock(&accept_lock);
  }

  cerr << "Pthread " << tid << " exiting.\n";
  pthread_exit(0); // happens at exit sequence
}

int main(int argc, char *argv[]) {
  struct sockaddr_in serv_addr;
  int port, number_thread;
  pthread_t tid;
  int *tid_ptr;
  string response;

  if (argc != 3) {
  	cerr << "Usage: " << argv[0] << " <port> <num_threads>\n";
  	exit(1);
  }

  int load_res = load_server();
  if (load_res) {
    cout << "Would you like to continue? (y/n)\n" << flush;
    getline(cin, response);
    if (tolower(response[0]) == 'n')
      exit(0);
  }

  port = atoi(argv[1]);
  number_thread = atoi(argv[2]);
  cout << "\nConnecting tictactoe server to port " << port << " with "
    << number_thread << " threads\n\n";

  serv_sockfd = socket(AF_INET, SOCK_STREAM, 0);
  bzero((void*)&serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_addr.sin_port = htons(port);

  bind(serv_sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
  listen(serv_sockfd, 5);

  for (int i = 0; i < number_thread; ++i) {
  	tid_ptr = new int;
  	*tid_ptr = i;
  	pthread_create(&tid, NULL, &one_thread, (void *)tid_ptr);
    cout << "thread " << i << " created\n" << flush;
  }
  pthread_create(&tid, NULL, &scheduled_maintenance, (void *)NULL);


  for (;;) {
    string line;
    getline(cin, line);
    if (line.compare("quit") == 0 || line.compare("exit") == 0) {
      cout << "Initiating server shut down sequence.\n";
      pthread_mutex_lock(&accept_lock);
      exit_sequence = true;
      pthread_mutex_unlock(&accept_lock);
      break;
    }
  }

  pthread_mutex_lock(&games_lock);
  pthread_mutex_lock(&users_lock);
  save_server();
  pthread_mutex_unlock(&users_lock);
  pthread_mutex_unlock(&games_lock);

  return 0;
}
