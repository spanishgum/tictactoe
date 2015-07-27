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

#define _read(a, b, c) if(read((a), (b), (c)))
#define _write(a, b, c) if(write((a), (b), (c)))

bool DEBUG_LEV = 0;

using namespace std;

extern pthread_mutex_t accept_lock;
extern pthread_mutex_t games_lock;
extern pthread_mutex_t users_lock;

extern vector<user> users;


// string utilitu
bool isnum(string s) {
	for (unsigned int i = 0; i < s.size(); i++)
		if (!isdigit(s[i]))
			return false;
	return true;
}

// string utility
vector<string> Split(string s) {
	vector<string> v;
	stringstream ss;
	string e;
	ss << s;
	ss >> e;
	while (ss) {
		v.push_back(e);
		ss >> e;
	}
	// v.back() = v.back().substr(0, v.back().size() - 1);
	v.back().pop_back();
	return v;
}

// simple client prompt - no data
void SendToClient(user& u) {
	if (u.online) {
		stringstream ss;
		ss << "<" << u.name << ": " << u.clientcounter << "> ";
		string msg = ss.str();
		++u.clientcounter;
		_write(u.cli_sockfd, msg.c_str(), msg.size());
	}
}

// sends actual response data to client
bool SendToClient(user& u, string msg) {
	// Sends msg to user u from the server.
	int cpylen = 0;
	int itr = 0;
	const int msize = (const int) msg.size();
	string subs;

	if (u.online) {
		if (msize > 0) {

			if (DEBUG_LEV)
				cout << "Message length = " << msg.size() << " characters.\n";
			//Send data in 512 char chunks
			while(itr < msize) {

				if (DEBUG_LEV)
					cout << "Beginning write loop." << endl;

				if ((itr + 512) < (int)msg.size())
					cpylen = 512;
				else
					cpylen = msg.size() - itr;

				subs = "";
				for (int i = itr; (i < itr + cpylen) && (i < msize); i++)
					subs += msg[i];

				if (DEBUG_LEV)
					cout << "Attempting to write " << cpylen
						<< " characters (" << itr << " - " << itr + cpylen << ").\n";

				_write(u.cli_sockfd, subs.c_str(), subs.size());

				itr += 512;
			}
		}

		SendToClient(u);
		return true;
	}
	return false;
}

// send data to multiple users in string list
void SendToClients(vector<string> ulist, string msg) {
	for (unsigned int i = 0; i < ulist.size(); ++i)
		for (unsigned int j = 0; j < users.size(); ++j)
			if (ulist[i] == users[j].name)
				SendToClient(users[j], msg);
}


// interpret a msg from the user
bool Parse(string line, user& u) {
	vector<string> v = Split(line);
	// line.pop_back();
	line = line.substr(0, line.size() - 1);

	stringstream ss; // use for output result
	stringstream ss2;
	user *oth_usr = 0; // use for other user

	pthread_mutex_lock(&users_lock);

	if (v[0].size() == 0); // Sends prompt only
	else if (v[0] == "who" || v[0] == "whom") {
		int nOnlineUsers = 0;
		for (unsigned int i = 0; i < users.size(); i++)
			if (users[i].online)
				nOnlineUsers++;

		ss << "Total of " << to_string(nOnlineUsers)
			<< " user(s) currently online.\n";

		for (unsigned int i = 0; i < users.size(); i++)
			if (users[i].online)
				ss << users[i].name << "\n";
	}
	else if (v[0] == "stats" || v[0] == "stat") {
		bool found = false;
		if (v.size() > 1) {
			if (v[1] == "")
				ss << u.stats();
				// SendToClient(u, u.stats());
			else {
				vector<user>::iterator us;
				for (us = users.begin(); us != users.end(); ++us)
					if (us->name == v[1]) {
						ss << us->stats();
						// SendToClient(u, us->stats());
						found = true;
					}
				if (!found)
					ss << "User does not exist.\n";
					// SendToClient(u, "User does not exist\n");
			}
		}
		else
			ss << u.stats();
	}
	else if (v[0] == "game" || v[0] == "games") {
		if (v.size() > 1) {
			if (isnum(v[1])) {
				if ((unsigned int) _stoi(v[1]) < games.size())
					ss << games[_stoi(v[1])].metadata();
				else
					ss << "Invalid game id.\n";
			}
			else ss << show_games();
		}
		else ss << show_games();
	}
	else if (v[0] == "observe") {
		if (v.size() < 2)
			ss << u.observe(0);
		else if (isnum(v[1]))
			ss << u.observe(_stoi(v[1]));
		else
			ss << "observe <game_id>.\nEnter 'game' for game listing\n";
	}
	else if (v[0] == "unobserve") {
		if (v.size() < 2)
			ss << u.unobserve(-1);
		else if (isnum(v[1]))
			ss << u.unobserve(_stoi(v[1]));
		else
			ss << "unobserve <game_id>.\n";
	}
	else if (v[0] == "match") {
		ss << game_matcher(u, v, oth_usr);
		if (oth_usr) {
			ss2 << "\n" << u.name << " has accepted your match.\n"
				<< "You are now playing in game " << (u.match)->id << "\n\n"
				<< (u.match)->print_board() << "\n";
			if (oth_usr->online)
				SendToClient((*oth_usr), ss2.str());
		}
	}
	else if (v[0] == "resign") {
		ss << game_resign(u, &oth_usr); //************************PROBLEMS HERE*************************************
		if (oth_usr) { // this was a valid request - inform opponent
			ss2 << "\n" << u.name << " resigned.\n";
			if (oth_usr->online)
				SendToClient((*oth_usr), ss2.str());
		}
	}
	else if (v[0] == "refresh") {
		ss << u.game_update();
	}
	else if (v[0] == "shout" && v.size() > 1) {
		string msg;
		bool seenSpace = false;
		for (unsigned int i = 0; i < line.size(); i++) {
			if (!seenSpace && line[i] == ' ')
				seenSpace = true;
			else if (seenSpace)
				msg += line[i];
		}
		msg = "!!! " + u.name + " !!!: " + msg + "\n";
		for (unsigned int i = 0; i < users.size(); i++) {
			if (users[i].online) { //Send msg to all from user u
				if (users[i].name == u.name)
					SendToClient(users[i], msg);
				else SendToClient(users[i], "\n" + msg);
			}
		}
	}
	else if (v[0] == "tell" && v.size() > 2) {
		string msg;
		bool seenSpace1 = false;
		bool seenSpace2 = false;
		for (unsigned int i = 0; i < line.size(); i++) {
			if (!seenSpace1 && line[i] == ' ')
				seenSpace1 = true;
			else if (!seenSpace2 && line[i] == ' ')
				seenSpace2 = true;
			else if (seenSpace1 && seenSpace2)
				msg += line[i];
		}
		msg = "### " + u.name + " ###: " + msg + "\n";
		for (unsigned int i = 0; i < users.size(); i++) {
			if (users[i].name == v[1]) { 	//Send msg to v[1] from u
				string mymsg = msg;
				if (users[i].name != u.name)
					msg = "\n" + msg;
				SendToClient(users[i], msg);
				break;
			}
		}
	}
	else if ((v[0] == "kibitz") || (v[0] == "'")) {
		vector<game *> glist;
		vector<game *>::iterator g;
		vector<string> ulist;
		if (v.size() < 2)
			ss << "Ignoring empy message.\n";
		else {
			bool only_one = isnum(v[1]); // first check
cerr << "--> " << only_one << "\n";
			only_one &= ((unsigned int) _stoi(v[1]) >= games.size() + 1); // sec check
cerr << "--> " << only_one << "\n";
			only_one &= (v.size() > 2); // third check
cerr << "--> " << only_one << "\n";
			ss2 << "\n Kibitz* " << u.name << ": ";
			if (only_one) {
				ss2 << line.substr(v[0].length() + v[1].length() + 1, line.length() - 1) << "\n";
				SendToClients(games[_stoi(v[1])].observing, ss2.str());
			}
			else {
				ss2 << line.substr(v[0].length() + 1, line.length() - 1) << "\n";
				for (g = u.watching.begin(); g != u.watching.end(); ++g)
					SendToClients((*g)->observing, ss2.str());
			}
		}
	}
	else if (v[0] == "quiet") {
		if (u.quiet)
			ss << "You are already in quiet mode.\n";
		else
			ss << "You are now in quiet mode.\n";
	}
	else if (v[0] == "nonquiet") {
		if (!u.quiet)
			ss << "You are already in nonquiet mode.\n";
		else
			ss << "You are now in nonquiet mode.\n";
	}
	else if (v[0] == "block" && v.size() > 1) {
		//Block v[1]
	}
	else if (v[0] == "ublock" && v.size() > 1) {
		//Unblock v[1]
	}
	else if (v[0] == "listmail") {
		//Print all mail headers for u
	}
	else if (v[0] == "readmail" && v.size() > 1) {
		//Print message v[1] for u
	}
	else if (v[0] == "deletemail" && v.size() > 1) {
		//Delete message v[1] for u
	}
	else if (v[0] == "mail" && v.size() > 3) {
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
	else if (v[0] == "info" && v.size() > 1) {
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
	else if (v[0] == "passwd" && v.size() > 1) {
		bool set = false;
		for (unsigned int i = 0; i < users.size(); i++)
			if (users[i].name == u.name) {
				if (DEBUG_LEV)
					cout << "Set " << u.name << "\'s password to " << v[1] << "\n";
				users[i].passwd = v[1];
				set = true;
				break;
			}
		if (set) ss << "Your password is now: " + v[1] + "\n";
		else ss << "Could not update your password.\n";
	}
	else if (v[0] == "exit" || v[0] == "quit" || v[0] == "bye") {
		u.online = false;
		pthread_mutex_unlock(&users_lock);
		return true;
	}
	else if (v[0] == "help" || v[0] == "?") {
		ss <<  "\n[...] optional field\n<......> required field\n"
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
		   << left << setw(25) <<  "? " 						<< "# print this message" << "\n\n";

		if (DEBUG_LEV)
			cout << "Done printing help." << "\n";
	}
	else{
		ss << "Error: That is not a supported command.\r\n";
	}

	SendToClient(u, ss.str());
	pthread_mutex_unlock(&users_lock);
	return false;
}
