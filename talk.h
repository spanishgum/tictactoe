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

		if (!u.writing_mail)
			SendToClient(u);
		return true;
	}
	return false;
}

// search by string name then send - unless blocked
void SendToClient(string uname, string msg, user &usr) {
	vector<user>::iterator u;
	for (u = users.begin(); u != users.end(); ++u)
		if (u->name == uname)
			if (!u->is_blocked(usr.name))
				SendToClient(*u, msg);
}

// search by string name then send - no exceptions
void SendToClient(string uname, string msg) {
	vector<user>::iterator u;
	for (u = users.begin(); u != users.end(); ++u)
		if (u->name == uname)
			SendToClient(*u, msg);
}

// send data to other users in string list - used by kibitz
void SendToClients(user &u, vector<string> ulist, string msg) {
	for (unsigned int i = 0; i < ulist.size(); ++i)
		for (unsigned int j = 0; j < users.size(); ++j)
			if (ulist[i] == users[j].name)
				if (ulist[i] != u.name)
					if (!users[j].quiet)
						if (!users[j].is_blocked(u.name))
				 			SendToClient(users[j], "\n" + msg);
}

// read mail body data into user outgoing mail
bool ParseBody(string line, user& u) {
	stringstream ss, ss2;
	line.back() = '\n';
	pthread_mutex_lock(&users_lock);
	if (u.outgoing) {
		u.outgoing->body += line;
		if (line == ".\n") {
			u.writing_mail = false;
			ss << "\n" << "Sending mail to " << u.outgoing->to << ".\n";
			ss2 << "\n\n" << "New message from " << u.name << "!\n\n";
			send_mail(u);
			SendToClient(u.outgoing->to, ss2.str(), u);
			SendToClient(u, ss.str());
			delete u.outgoing;
			u.outgoing = 0;
		}
	}
	else {
		u.writing_mail = false;
		u.outgoing = 0;
		cerr << "Client sync error when writing mail.\n";
		ss << "Sorry, something seems to be wrong."
			<<" We've lost your outgoing file.\n";
		SendToClient(u, ss.str());
	}
	pthread_mutex_unlock(&users_lock);
	return false;
}


// interpret a msg from the user
bool Parse(string line, user& u) {
	if (u.writing_mail) {
		return ParseBody(line, u);
	}
	vector<string> v = Split(line);
	if (!line.empty()) line.pop_back();
	if (v.size() < 2) line = line.substr(v[0].length());
	else line = line.substr(v[0].length() + 1);
	// if (v.size() > 1) line = line.substr(1);

	stringstream ss, ss2; // output to clients
	unsigned int id = 0; // indexer
	user *oth_usr = 0; // function ref params
	vector<user>::iterator usr; // search user vects
	vector<string>::iterator names; // search string vects


	pthread_mutex_lock(&users_lock);

	if (v[0].size() == 0); // Sends prompt only
	else if (v[0] == "cls" || v[0] == "clear") {
		ss << "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n";
	}
	else if (v[0] == "who" || v[0] == "whom" || v[0] == "w") {
		int nOnlineUsers = 0;
		for (unsigned int i = 0; i < users.size(); i++)
			if (users[i].online)
				nOnlineUsers++;

		ss << "\nTotal of " << to_string(nOnlineUsers)
			<< " user(s) currently online.\n";

		for (unsigned int i = 0; i < users.size(); i++)
			if (users[i].online)
				ss << " " << users[i].name << "\n";
	}
	else if (v[0] == "stats" || v[0] == "stat" || v[0] == "s") {
		bool found = false;
		if (v.size() > 1) {
			if (v[1] == "")
				ss << u.stats();
			else {
				for (usr = users.begin(); usr != users.end(); ++usr)
					if (usr->name == v[1]) {
						ss << usr->stats();
						found = true;
					}
				if (!found)
					ss << "User does not exist.\n";
			}
		}
		else
			ss << u.stats();
	}
	else if (v[0] == "game" || v[0] == "games" || v[0] == "g") {
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
	else if (v[0] == "observe" || v[0] == "o") {
		if (v.size() < 2)
			ss << u.observe(0);
		else if (isnum(v[1]))
			ss << u.observe(_stoi(v[1]));
		else
			ss << "observe <game_id>.\nEnter 'game' for game listing\n";
	}
	else if (v[0] == "unobserve" || v[0] == "u") {
		if (v.size() < 2)
			ss << u.unobserve(-1);
		else if (isnum(v[1]))
			ss << u.unobserve(_stoi(v[1]));
		else
			ss << "unobserve <game_id>.\n";
	}
	else if (v[0] == "match" || v[0] == "m") {
		ss << game_matcher(u, v, &oth_usr);
		if (oth_usr) {
			if ((u.match)->pending) {
				ss2 << "\n\n" << u.name << " has sent you a new match request!\n"
					<< "To accept: match <";
				if (u.name == (u.match)->player[0]) ss2 << "w";
				else ss2 << "b";
				ss2 << "> <" << (u.match)->timer[1] << ">\n"
					<< "Or you modify the game details.\n\n";
				SendToClient((*oth_usr), ss2.str());
			}
			else {
				ss2 << "\n" << u.name << " has accepted your match.\n"
					<< "You are now playing in game " << (u.match)->id << "\n\n"
					<< (u.match)->print_board() << "\n";
				SendToClient((*oth_usr), ss2.str());
			}
		}
	}
	else if (v[0] == "resign" || v[0] == "res") {
		ss << game_resign(u, &oth_usr);
		if (oth_usr) { // this was a valid request - inform opponent
			ss2 << "\n" << u.name << " resigned.\n";
			SendToClient((*oth_usr), ss2.str());
		}
	}
	else if (v[0] == "refresh" || v[0] == "r") {
		ss << u.game_update();
		if (u.match) {
			string opp = u.get_oppon();
			for (unsigned int i = 0; i < users.size(); ++i)
				if (users[i].name == opp)
					if (!users[i].online) {
						ss << "\n\nOpponent logged out, you win!\n";
						game_fin(u);
					}
		}
	}
	else if (v[0] == "shout" || v[0] == "sh") {
		ss << "\n!!! " << u.name << " !!!: "
			<< line << "\n\n";
		for (unsigned int i = 0; i < users.size(); i++)
			if (users[i].online)
				if (!users[i].quiet)
					if (users[i].name != u.name)
						if (!users[i].is_blocked(u.name))
							SendToClient(users[i], "\n" + ss.str());
	}
	else if (v[0] == "tell" || v[0] == "t") {
		if (v.size() < 2) {
			ss << "Usage: tell <user> <msg>\n";
		}
		else if (u.name == v[1])
			ss << "Soo.. your talking to yourself now?\n";
		else {
			ss2 << "\n\n### " << u.name << " ###: "
				<< line.substr(v[1].length() + 1) << "\n\n";
			for (unsigned int i = 0; i < users.size(); i++)
				if (users[i].online)
					if (users[i].name == v[1])
						if (!users[i].is_blocked(u.name))
							SendToClient(users[i], ss2.str());
		}
	}
	else if (v[0] == "kibitz" || v[0] == "k" || v[0] == "'") {
		vector<game *>::iterator g;
		if (v.size() < 2)
			ss << "Ignoring empty message.\n";
		else if (u.watching.empty())
			ss << "You must be observing a game to do that.\n";
		else {
			bool only_one = isnum(v[1]); // first check
			if (only_one) {
				id = strtol(v[1].c_str(), NULL, 10);
				only_one &= (id < games.size()); // sec check
				only_one &= (v.size() > 2); // third check
			}
			else id = (u.watching[0])->id;
			ss2 << "\n Kibitz* " << u.name << ": ";
			if (only_one) {
				ss2 << line.substr(v[1].length() + 1) << "\n\n";
				SendToClients(u, games[id].observing, ss2.str());
			}
			else {
				ss2 << line << "\n\n";
				for (g = u.watching.begin(); g != u.watching.end(); ++g)
					SendToClients(u, (*g)->observing, ss2.str());
			}
			ss << ss2.rdbuf();
		}
	}
	else if (v[0] == "quiet" || v[0] == "q") {
		if (u.quiet)
			ss << "You are already in quiet mode.\n";
		else {
			u.quiet = true;
			ss << "You are now in quiet mode.\n";
		}
	}
	else if (v[0] == "nonquiet" || v[0] == "nq") {
		if (!u.quiet)
			ss << "You are already in nonquiet mode.\n";
		else {
			u.quiet = false;
			ss << "You are now in nonquiet mode.\n";
		}
	}
	else if (v[0] == "block" || v[0] == "b") {
		if (v.size() < 2) {
			ss << "\n Blocked users: ";
			for (names = u.blocked.begin(); names != u.blocked.end(); ++names)
				ss << *names << " ";
			ss << "\n\n";
		}
		else if (v[1] == u.name) {
			ss << "You can't block yourself.\n";
		}
		else {
			switch(u.block(v[1])) {
				case -2:
					ss << "You have reached your blocking limit.\n";
					break;
				case -1:
					ss << "User " << v[1] << " does not exist.\n";
					break;
				case 0:
					ss << "You are already blocking " << v[1] << ".\n";
					break;
				case 1:
					ss << "User " << v[1] << " blocked.\n";
					break;
				default:
					break;
			}
		}
	}
	else if (v[0] == "unblock" || v[0] == "ub") {
		if (v.size() < 2) {
			ss << "Usage: unblock <user>\n";
		}
		else if (v[1] == u.name) {
			ss << "You can't (un)block yourself.\n";
		}
		else {
			switch(u.unblock(v[1])) {
				case -1:
					ss << "User " << v[1] << " does not exist.\n";
					break;
				case 0:
					ss << "You are not blocking " << v[1] << ".\n";
					break;
				case 1:
					ss << "User " << v[1] << " unblocked.\n";
					break;
				default:
					break;
			}
		}
	}
	else if (v[0] == "listmail" || v[0] == "lm") {
		if (u.inbox.size())
			ss << u.list_mail();
		else
			ss << "Mailbox empty.\n";
	}
	else if (v[0] == "readmail" || v[0] == "rm") {
		id = 0;
		if (v.size() > 1) {
			if (isnum(v[1])) id = strtol(v[1].c_str(), NULL, 10);
			if (u.inbox.size() < id) id = 0;
		}
		if (u.inbox.size())
			ss << u.read_mail(id);
		else
			ss << "Mailbox empty.\n";
	}
	else if (v[0] == "deletemail" || v[0] == "dm") {
		if (v.size() < 2)
			ss << "Usage: deletemail <m_id>\n";
		else {
			if (isnum(v[1])) {
				id = strtol(v[1].c_str(), NULL, 10);
				if (u.del_mail(id))
					ss << "Message deleted.\n";
				else
					ss << "Invalid message number.\n";
			}
			else
				ss << "Usage: deletemail <m_id>\n";
		}
	}
	else if (v[0] == "mail" || v[0] == "ml") {
		if(v.size() < 2)
			ss << "Usage: mail <user> <title>\n";
		else {
			for (usr = users.begin(); usr != users.end(); ++usr)
				if (usr->name == v[1]) {
					u.writing_mail = true;
					u.outgoing = new mail("", u.name, "", v[1]);
					if (v.size() > 2)
						(u.outgoing)->title = line.substr(v[1].length() + 1);
					ss << "\nPlease input mail body, finishing with '.' "
						<< "at the beginning of a line.\n\n\n";
					break;
				}
			if (!u.writing_mail)
				ss << "User " << v[1] << " does not exist.\n";
		}
	}
	else if (v[0] == "info" || v[0] == "i") {
		if (v.size() < 2)
			u.info = "";
		else u.info = line;
		ss << "Info changed.\n";
	}
	else if (v[0] == "passwd" || v[0] == "pswd") {
		bool set = false;
		if (v.size() < 2)
			ss << "\nUsage: passwd <text>\n"
				<< "Note: <text> may not contain whitespace.\n\n";
		else {
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
	}
	else if (v[0] == "exit" || v[0] == "quit" || v[0] == "bye") {
		u.online = false;
		pthread_mutex_unlock(&users_lock);
		return true;
	}
	else if (v[0] == "help" || v[0] == "h" || v[0] == "?") {
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
	else if (v[0].length() == 2) {
		string opp = u.get_oppon();
		ss << u.move(v[0]);
		ss2 << (u.match)->print_board() << "\n";
		if (u.match) {
			if ((u.match)->fin){
				game_fin(u);
				ss2 << "\nBetter luck next time.\n" << u.name << " won the game.\n";
			}
			else {
				for (unsigned int i = 0; i < users.size(); ++i)
					if (users[i].name == opp)
						if (!users[i].online) {
							ss << "Opponent logged out, you win!\n";
							game_fin(u);
						}
			}
		}
		SendToClient(opp, ss2.str());
	}
	else {
		ss << "Error: That is not a supported command.\r\n";
	}

	SendToClient(u, ss.str());
	pthread_mutex_unlock(&users_lock);
	return false;
}
