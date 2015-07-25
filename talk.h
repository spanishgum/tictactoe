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
#include <stdlib.h>
#include <stdio.h>
#include <iomanip>
#include <sys/socket.h>

#include "user.h"

using namespace std;

extern pthread_mutex_t accept_lock;
extern pthread_mutex_t games_lock;
extern pthread_mutex_t users_lock;
//extern


/*struct user{
	bool online = false;
	string name;
	string passwd;
	int cli_sockfd;
	bool inGame = false;
	bool turn = false;
};//*/

extern vector<user> users;

bool isnum(string s){
	for (unsigned int i = 0; i < s.size(); i++){
		if (!isdigit(s[i]))
			return false;
	}
	return true;
}

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

	v[v.size() - 1] = v[v.size() - 1].substr(0, v[v.size() - 1].size() - 1);
	return v;
}

void SendToClient(user& u){
	//Update client prompt. Use for noop commands.
	if (u.online){
		stringstream ss;
		ss << "< telnet " << u.clientcounter << " >: ";
		string msg = ss.str();
		u.clientcounter = u.clientcounter + 1;
		//strncpy(buf, msg.c_str(), msg.size());
		write(u.cli_sockfd, msg.c_str(), msg.size());


		cout << "Done sending message to client." << endl;
	}
}

bool SendToClient(user& u, string msg){
	//Sends msg to user u from the server.
	//u should contain client ID stuff so we can talk to them.
	//I expect all the ports should just stay open while a user is connected.

	//Format data
	if (u.online){
		if (msg.size() != 0){
			int cpylen = 0;
			int itr = 0;

			const int msize = (const int) msg.size();


			cout << "Message length = " << msg.size() << " characters." << endl;
			//Send data in 512 char chunks
			while(itr < msize){
				cout << "Beginning write loop." << endl;
				string subs;

				//for(int i = 0; i < cpylen; i++)
				//	buf[i] = '\0';

				if ((itr + 512) < (int)msg.size())
					cpylen = 512;
				else {
					cpylen = msg.size() - itr;
				}

				subs = "";
				for (int i = itr; i < itr + cpylen && i < msize; i++){
					subs += msg[i];
				}

				//strncpy(buf, subs.c_str(), cpylen);

				cout << "Attempting to write " << cpylen << " characters (" << itr << " - " << itr + cpylen << ")." << endl;

				//buf[strlen(buf)] = '\0';
				//if(
				write(u.cli_sockfd, subs.c_str(), subs.size());//)
				//cout << "Written data." << endl;

				itr += 512;
				cout << "End loop." << endl;
			}
		}

		SendToClient(u);

		return true;
	}
	return false;
}

bool SendToClient(string msg, user& u){
	return SendToClient(u, msg);
}

bool Parse(string line, string uname){ //user& u){
	vector<string> v = Split(line);
	line = line.substr(0, line.size() - 1);
	pthread_mutex_lock(&users_lock);

	user *u = NULL;
	for (unsigned int i = 0; i < users.size(); i++)
		if (users[i].name == uname){
			u = &users[i];
			break;
		}

	if (u == NULL){
		cout << "Error: " << uname << " is no longer a user.\n";
		return true;
	}


	if (v[0].size() == 0){
		string msg = "";
		SendToClient(*u); //Send to client new prompt
		//noop
	}
	else if (v[0] == "who"){
		//List online users
		int nOnlineUsers = 0;
		for (unsigned int i = 0; i < users.size(); i++){
			if (users[i].online){
				nOnlineUsers++;
			}
		}

		stringstream ss;
		ss << "Total " << to_string(nOnlineUsers) << " users currently online.\n";


		for (unsigned int i = 0; i < users.size(); i++){
			if (users[i].online){
				ss << users[i].name << "\n";
			}
		}

		string msg = ss.str();
		SendToClient(*u, msg);
	}
	else if (v[0] == "stats" && v.size() > 1){
		//List information about user v[1]
		user & us = users[0];
		for (unsigned int i = 0; i < users.size(); i++){
			us = users[i];
			if (us.name == v[1])
				break;
		}

		SendToClient(*u, us.stats());
	}
	else if (v[0] == "game"){
		//List all running games (with id, players, etc)
	}
	else if (v[0] == "observe" && v.size() > 1){
		//Join game v[1] as observer
	}
	else if (v[0] == "unobserve"){
		//Unobserve most recently observed game. v[1] is optional gameID parameter
	}
	else if (v[0] == "match" && v.size() > 3){
		//Attempt to start game with user v[1] as v[2]. Optional turn time is v[3].
	}
	/*else if (v[0].size() == 2 &&
		(v[0][0] == 'A' || v[0][0] == 'B' || v[0][0] == 'C') &&
		(v[0][1] == '0' || v[0][1] == '1' || v[0][1] == '2')){
		if ((*u).playing && ((*u).match->player[turn%2] == (*u).name)){
			//Make move
		}
		else if ((*u).playing && !(*u).turn){
			printf("Error: It is not your turn.\n");
		}
		else{
			printf("Error: You are not in any games.\n");
		}
	}//*/
	else if (v[0] == "resign"){
		//Resign most recently started game. v[1] is optional gameID parameter.
	}
	else if (v[0] == "refresh"){
		//Refresh the screen
	}
	else if (v[0] == "shout" && v.size() > 1){
		string msg;
		bool seenSpace = false;
		for (unsigned int i = 0; i < line.size(); i++){
			if (!seenSpace && line[i] == ' '){
				seenSpace = true;
			}
			else if (seenSpace){
				msg += line[i];
			}
		}

		//stringstream ss;
		msg = "!!! " + (*u).name + " !!!: " + msg + "\n";
		//ss >> msg;


		for (unsigned int i = 0; i < users.size(); i++){
			if (users[i].online){
				if (users[i].name == (*u).name)
					SendToClient(users[i], msg);
				else SendToClient(users[i], "\n" + msg);
			}
		}
		//Send msg to all from user u
	}
	else if (v[0] == "tell" && v.size() > 2){
		string msg;
		bool seenSpace1 = false;
		bool seenSpace2 = false;
		for (unsigned int i = 0; i < line.size(); i++){
			if (!seenSpace1 && line[i] == ' '){
				seenSpace1 = true;
			}
			else if (!seenSpace2 && line[i] == ' '){
				seenSpace2 = true;
			}
			else if (seenSpace1 && seenSpace2){
				msg += line[i];
			}
		}


		//stringstream ss;
		msg = "### " + (*u).name + " ###: " + msg + "\n";
		//ss >> msg;

		for (unsigned int i = 0; i < users.size(); i++){
			if (users[i].name == v[1]){
				string mymsg = msg;
				if (users[i].name != (*u).name)
					msg = "\n" + msg;
				SendToClient(users[i], msg);
				break;
			}
		}

		//Send msg to v[1] from u
	}
	else if ((v[0] == "kibitz" || v[0] == "'") &&
		(isnum(v[1])) && v.size() > 3){
		string msg;
		bool seenSpace1 = false;
		bool seenSpace2 = false;
		for (unsigned int i = 0; i < line.size(); i++){
			if (!seenSpace1 && line[i] == ' '){
				seenSpace1 = true;
			}
			else if (!seenSpace2 && line[i] == ' '){
				seenSpace2 = true;
			}
			else if (seenSpace1 && seenSpace2){
				msg += line[i];
			}
		}

		//Send msg to all observing game v[1] from u


	}
	else if ((v[0] == "kibitz" || v[0] == "'") && v.size() > 2){
		string msg;
		bool seenSpace = false;
		for (unsigned int i = 0; i < line.size(); i++){
			if (!seenSpace && line[i] == ' '){
				seenSpace = true;
			}
			else if (seenSpace){
				msg += line[i];
			}
		}

		//Send msg to all observing from user u

		msg = "$$$ " + (*u).name + " $$$: " + msg + "\n";


	}
	else if (v[0] == "quiet"){
		//Set u to quiet mode
	}
	else if (v[0] == "nonquiet"){
		//Set u to not quiet mode
	}
	else if (v[0] == "block" && v.size() > 1){
		//Block v[1]
	}
	else if (v[0] == "ublock" && v.size() > 1){
		//Unblock v[1]
	}
	else if (v[0] == "listmail"){
		//Print all mail headers for u
	}
	else if (v[0] == "readmail" && v.size() > 1){
		//Print message v[1] for u
	}
	else if (v[0] == "deletemail" && v.size() > 1){
		//Delete message v[1] for u
	}
	else if (v[0] == "mail" && v.size() > 3){
		string msg;
		bool seenSpace1 = false;
		bool seenSpace2 = false;
		for (unsigned int i = 0; i < line.size(); i++){
			if (!seenSpace1 && line[i] == ' '){
				seenSpace1 = true;
			}
			else if (!seenSpace2 && line[i] == ' '){
				seenSpace2 = true;
			}
			else if (seenSpace1 && seenSpace2){
				msg += line[i];
			}
		}

		//Send msg as mail to v[1] from u
	}
	else if (v[0] == "info" && v.size() > 1){
		string msg;
		bool seenSpace = false;
		for (unsigned int i = 0; i < line.size(); i++){
			if (!seenSpace && line[i] == ' '){
				seenSpace = true;
			}
			else if (seenSpace){
				msg += line[i];
			}
		}

		//Set u's status to msg
	}
	else if (v[0] == "passwd" && v.size() > 1){
		stringstream ss;
		string msg;
		bool set = false;



		for (unsigned int i = 0; i < users.size(); i++){
			if (users[i].name == (*u).name){
				cout << "Set " << (*u).name << "\'s password to " << v[1] << "\n";
				users[i].passwd = v[1];
				set = true;
				break;
			}
		}
		if (set)
			msg =  "Your password is now: " + v[1] + "\n";
		else
			msg =  "Could not update your password.\n";

		SendToClient((*u), msg);


		//Set u's password to v[1]
	}
	else if (v[0] == "exit" || v[0] == "quit" || v[0] == "bye"){
		(*u).online = false;
		pthread_mutex_unlock(&users_lock);
		return true;
	}
	else if (v[0] == "help" || v[0] == "?"){
		stringstream ss;
		string msg;
		ss << left << setw(25) <<  "[...] optional field, <......> required field" << "\n"
		   << left << setw(25) <<  "who " 						<< "# List all online users" << "\n"
		   << left << setw(25) <<  "stats [name] " 				<< "# Display user information" << "\n"
		   << left << setw(25) <<  "game " 						<< "# list all current games" << "\n"
		   << left << setw(25) <<  "observe <game_num> " 		<< "# Observe a game" << "\n"
		   << left << setw(25) <<  "unobserve " 				<< "# Unobserve a game" 	<< "\n"
		   << left << setw(25) <<  "match <name> <b|w> [t]" 	<< "# Try to start a game" 	<< "\n"
		   << left << setw(25) <<  "<A|B|C><1|2|3> " 			<< "# Make a move in a game" << "\n"
		   << left << setw(25) <<  "resign " 					<< "# Resign a game" << "\n"
		   << left << setw(25) <<  "refresh " 					<< "# Refresh a game" 	<< "\n"
		   << left << setw(25) <<  "shout <msg> " 				<< "# shout <msg> to every one online" << "\n"
		   << left << setw(25) <<  "tell <name> <msg> " 		<< "# tell user <name> message" 	<< "\n"
		   << left << setw(25) <<  "kibitz <msg>" 				<< "# Comment on a game when observing" << "\n"
		   << left << setw(25) <<  "’ <msg> " 					<< "  # Comment on a game" << "\n"
		   << left << setw(25) <<  "quiet " 					<< "# Quiet mode, no broadcast messages"<< "\n"
		   << left << setw(25) <<  "nonquiet " 					<< "# Non-quiet mode" << "\n"
		   << left << setw(25) <<  "block <id> " 				<< "# No more communication from <id>" 	<< "\n"
		   << left << setw(25) <<  "unblock <id> " 				<< "# Allow communication from <id>" 	<< "\n"
		   << left << setw(25) <<  "listmail " 					<< "# List the header of the mails" 	<< "\n"
		   << left << setw(25) <<  "readmail <msg_num> " 		<< "# Read the particular mail" << "\n"
		   << left << setw(25) <<  "deletemail <msg_num>" 		<< "# Delete the particular mail" 	<< "\n"
		   << left << setw(25) <<  "mail <id> <title> " 		<< "# Send id a mail" << "\n"
		   << left << setw(25) <<  "info <msg> " 				<< "# change your information to <msg>"	<< "\n"
		   << left << setw(25) <<  "passwd <new> " 				<< "# change password" << "\n"
		   << left << setw(25) <<  "exit " 						<< "# quit the system" << "\n"
		   << left << setw(25) <<  "quit " 						<< "# quit the system" << "\n"
		   << left << setw(25) <<  "help " 						<< "# print this message" << "\n"
		   << left << setw(25) <<  "? " 						<< "# print this message" << "\n";

		msg = ss.str();
		SendToClient((*u), msg);

		cout << "Done printing help." << "\n";
		//Print help
	}
	else{
		string s = "Error: That is not a supported command.\r\n";
		SendToClient(s.c_str(), (*u));
	}
	//else if (v[0] == ""){}

	pthread_mutex_unlock(&users_lock);
	return false;
}
