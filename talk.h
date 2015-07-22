/*
*  COP5570    |    Parallel, Concurrent, Distributed Programming
*  Asg #4     |    Tic Tac Toe Game Server
*  Summer C   |    07/24/15
*
*     by Adam Stallard, Steven Rohr
*
*
*  This unit of code will be used to interpret input and output.
*
*  The server will send a top lvl parser some user input and
*   the parser will respond with the type of request that was
*   sent, or possible error msgs.
*
*  After recieving the interpretation, the server will call the
*   appropriate functions which will handle contiguos input
*   and output until the request is fully satisfied.
*
*
*
*
*/

#include <string>
#include <vector>
#include <sstream>
#include <sys/socket.h>

using namespace std;

extern pthread_mutex_t accept_lock = PTHREAD_MUTEX_INITIALIZER;
extern vector<user> users;

vector<string> Split(string s){
	vector<string> v;
	stringstream ss; 
	string e;
	ss << s;
	ss >> e;
	while (ss){
		v.pusb_back(e);
		ss >> e;
	}
	return v;
}


bool SendToClient(user u, string msg){
	//Sends msg to user u from the server.
	//u should contain client ID stuff so we can talk to them.
	//I expect all the ports should just stay open while a user is connected.
	
	//Format data 
	if (u.connected){
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
			write(u.cli_sockfd, buf, strlen(buf));

			itr += 512;
		}

		return true;  
	} 
	return false;
}

bool SendToClient(string msg, user u){
	return SendToClient(u, msg);
}

bool Parse(string line, user u){
	vector<string> v = Split(line);

	if (v[0] == "who"){
		//List online users
	}
	else if (v[0] == "stats"){
		//List information about user v[1]
	}
	else if (v[0] == "game"){
		//List all running games (with id, players, etc)
	}
	else if (v[0] == "observe"){
		//Join game v[1] as observer
	}
	else if (v[0] == "unobserve"){
		//Unobserve most recently observed game. v[1] is optional gameID parameter
	}
	else if (v[0] == "match"){
		//Attempt to start game with user v[1] as v[2]. Optional turn time is v[3].
	}
	else if (v[0].size() == 2 && 
		(v[0][0] == "A" || v[0][0] == "B" || v[0][0] == "C") &&
		(v[0][1] == "0" || v[0][1] == "1" || v[0][1] == "2")){
		if (u.inGame && u.myTurn){
			//Make move
		}
		else if (u.inGame && !u.myTurn){
			printf("Error: It is not your turn.\n");
		}
		else{
			printf("Error: You are not in any games.\n");
		}
	}
	else if (v[0] == "resign"){
		//Resign most recently started game. v[1] is optional gameID parameter.
	}
	else if (v[0] == "refresh"){
		//Refresh the screen
	}
	else if (v[0] == "shout"){
		string msg;
		bool seenSpace = false;
		for (int i = 0; i < line.size(); i++){
			if (!seenSpace && line[i] == " "){
				seenSpace = true;
			}
			else if (seenSpace){
				msg += line[i];
			}
		}

		//Send msg to all from user u
	}
	else if (v[0] == "tell"){
		string msg;
		bool seenSpace1 = false;
		bool seenSpace2 = false;
		for (int i = 0; i < line.size(); i++){
			if (!seenSpace1 && line[i] == " "){
				seenSpace1 = true;
			}
			else if (!seenSpace2 && line[i] == " "){
				seenSpace2 = true;
			}
			else if (seenSpace1 && seenSpace2){
				msg += line[i];
			}
		}

		//Send msg to v[1] from u
	}
	else if (v[0] == "kibitz"){
		string msg;
		bool seenSpace = false;
		for (int i = 0; i < line.size(); i++){
			if (!seenSpace && line[i] == " "){
				seenSpace = true;
			}
			else if (seenSpace){
				msg += line[i];
			}
		}

		//Send msg to all observing from user u
	}
	else if (v[0] == "'"){
		string msg;
		bool seenSpace = false;
		for (int i = 0; i < line.size(); i++){
			if (!seenSpace && line[i] == " "){
				seenSpace = true;
			}
			else if (seenSpace){
				msg += line[i];
			}
		}

		//Send msg to all observing from user u
	}
	else if (v[0] == "quiet"){
		//Set u to quiet mode
	}
	else if (v[0] == "nonquiet"){
		//Set u to not quiet mode
	}
	else if (v[0] == "block"){
		//Block v[1]
	}
	else if (v[0] == "ublock"){
		//Unblock v[1]
	}
	else if (v[0] == "listmail"){
		//Print all mail headers for u
	}
	else if (v[0] == "readmail"){
		//Print message v[1] for u
	}
	else if (v[0] == "deletemail"){
		//Delete message v[1] for u
	}
	else if (v[0] == "mail"){
		string msg;
		bool seenSpace1 = false;
		bool seenSpace2 = false;
		for (int i = 0; i < line.size(); i++){
			if (!seenSpace1 && line[i] == " "){
				seenSpace1 = true;
			}
			else if (!seenSpace2 && line[i] == " "){
				seenSpace2 = true;
			}
			else if (seenSpace1 && seenSpace2){
				msg += line[i];
			}
		}

		//Send msg as mail to v[1] from u
	}
	else if (v[0] == "info"){
		string msg;
		bool seenSpace = false;
		for (int i = 0; i < line.size(); i++){
			if (!seenSpace && line[i] == " "){
				seenSpace = true;
			}
			else if (seenSpace){
				msg += line[i];
			}
		}

		//Set u's status to msg
	}
	else if (v[0] == "passwd"){
		//Set u's password to v[1]
	}
	else if (v[0] == "exit" || v[0] == "quit"){
		//Logout
		return true;
	}
	else if (v[0] == "help" || v[0] == "?"){
		//Print help
	}
	else{
		printf("Error: \"%s\" is not a supported command.\n", v[0]);
	}
	//else if (v[0] == ""){}

	return false;
}