/*
*  COP5570    |    Parallel, Concurrent, Distributed Programming
*  Asg #4     |    Tic Tac Toe Game Server
*  Summer C   |    07/24/15
*
*     by Adam Stallard, Steven Rohr
*
*/

#ifndef _USER_H
#define _USER_H

#include <vector>
#include <ctime>
#include <sstream>
#include <string>
#include "mail.h"
#include "game.h"

using namespace std;

class user;

pthread_mutex_t users_lock = PTHREAD_MUTEX_INITIALIZER;
vector<user> users;

class user {
	public:
		string name;
		string passwd;
		string info;
		double rating;
		int wins;
		int loses;
		bool quiet;
		bool online;
		vector<string> blocked;
		vector<game *> watching;
		vector<mail> inbox;
		game *match;
		bool playing;

		int clientcounter;
		int cli_sockfd;

		//****************** OPERATORS *************************//
		friend bool operator ==(user &l, user &r) {
			if ((l.name).compare(r.name) == 0)
				return true;
			else return false;
		}

		friend ostream& operator <<(ostream &ofs, user u) {
			// basic data
			ofs << u.name << "\n" << u.passwd << "\n" << u.info << "\n"
				<< u.wins << "\n" << u.loses << "\n" << u.quiet << "\n";

			// blocked list
			for (vector<string>::iterator b = u.blocked.begin();
				b != u.blocked.end(); ++b)
				ofs << (*b) << " ";
			ofs << "\n";

			// mail
			ofs << u.inbox.size() << "\n"; // metadata
			for (vector<mail>::iterator i = u.inbox.begin();
				i != u.inbox.end(); ++i)
				ofs << (*i);

			// form feed to separate users
			ofs << "\f...\n";

			return ofs;
		}

		friend istream& operator >>(istream &ifs, user &u) {
			stringstream form, tmp;
			string line, data, sub_data;
			mail m = mail();
			int num_lines = 0;
			unsigned int num_msgs;

			// init values not in file
			u.playing = false;
			u.match = 0;
			u.clientcounter = 0;

			// grab user form
			do {
				getline(ifs, line);
				form << line << "\n"; // \n removed, add it back
				++num_lines;
			} while (line.compare("\f...") != 0); // end of user marker

			// double check user meta data quality
			if (num_lines < 8) {
				cerr << "Back up data file may be corrupted. . .\n"
					<< "\tAborting load for curr user object\n";
				return ifs;
			}

			// input basic user data
			getline(form, data); u.name = data;
			getline(form, data); u.passwd = data;
			getline(form, data); u.info = data;
			getline(form, data); u.wins = _stoi(data);
			getline(form, data); u.loses = _stoi(data);
			u.rating = (double)u.wins / (u.wins + u.loses);
			getline(form, data); u.quiet = (bool) _stoi(data);

			// input blocked list
			getline(form, data); // line contains blocked users
			tmp << data; // put line into stream
			while (getline(tmp, data, ' ')) // tokenize
				u.blocked.push_back(data);

			// input mail data
			getline(form, data); num_msgs = _stoi(data);
			for (unsigned int i = 0; i < num_msgs; ++i) {
				form >> m;
				u.inbox.push_back(m);
			}

			// double check mail data quality
			getline(form, data);
			if (data.compare("\f...") != 0) {
				cerr << "Back up data file may be corrupted. . .\n"
					<< "\tLoad for curr user ended unexpectedly\n";
				return ifs;
			}

			return ifs;
		}


		//****************** UTILITIES *************************//
		static int _stoi(string data) { // wrapper - prevent stoi abort
			string sub = "";
			for(char c : data) {
				if (isdigit(c)) sub += c;
				else if (c == '-') {
					if (sub == "") sub += c;
					else break;
				}
				else break; // only grab leading digits in string
			}
			return stoi(sub);
		}


		//****************** CONSTRUCTOR ***********************//
		user(string u_name = "", string u_passwd = "") {
			name = u_name;
			passwd = u_passwd;
			info = "";
			rating = wins = loses = quiet = 0;
			clientcounter = 0;
			online = false;
			match = 0;
		}



		//****************** OPERATORS *************************//
		string stats() {
			stringstream ss;
			ss << "\n User: " << name
				<< "\n Info: " << info
				<< "\n Rating: " << rating
				<< "\n Wins: " << wins
				<< ", Loses: " << loses
				<< "\n Quiet: " << (quiet ? "Yes" : "No")
				<< "\n Blocked users:";
			if (blocked.empty())
				ss << " <none>";
			else {
				for (unsigned int i = 0; i < blocked.size(); ++i)
					ss << " " << blocked[i];
			}
			ss << "\n\n " << name
				<< " is currently "
				<< (online ? "online.\n\n" : "offline.\n\n");

			return ss.str();
		}



		//****************** GAMING ****************************//
		int req_match(vector<string> v) {
			int req = g_request(name, v);
			switch (req) {
				case -1: // pending
					break;
				default: // game_id
					accept_match(&games[req]);
					break;
			}
			return req;
		}

		void accept_match(game *g) {
			match = g;
			playing = true;
			watching.push_back(g);
		}

		string quit_match(int *valid) {
			stringstream ss;
			if (!playing)
				ss << "You are not currently in a match.\n";
			else {
				if (match == NULL) {
					cerr << "Warning: User object out of sync.\n"
						<< "\tUpdating gaming status.\n";
						playing = false;
					ss << "You are not currently in a match.\n";
				}
				else { // playing set, match exists
					*valid = 1;
					match->quit(name);
					clear_game(0);
				}
			}
			return ss.str();
		}

		string observe(int gid) {
			stringstream ss;
			vector<game *>::iterator g;
			if ((unsigned int) gid < games.size()) {
				ss << games[gid].add_observer(name);
				for (g = watching.begin(); g != watching.end(); ++g)
					if ((*g)->id == gid) return ss.str();
				watching.push_back(&games[gid]);
			}
			else
				ss << "The game does not exist.\n";
			return ss.str();
		}

		string unobserve(int gid) {
			stringstream ss;
			vector<game *>::iterator g;
			if (gid == -1) {
				if (watching.empty())
					ss << "You are not observing anything\n";
				else {
					g = watching.begin();
					ss << (*g)->rem_observer(name);
					watching.erase(g);
				}
			}
			else {
				for (g = watching.begin(); g != watching.end(); ++g)
					if ((*g)->id == gid) {
						ss << (*g)->rem_observer(name);
						watching.erase(g);
						return ss.str();
					}
				if ((*g)->id != gid)
					ss << "You are not watching game " << gid << "\n";
			}
			return ss.str();
		}

		string game_update() {
			stringstream ss;
			vector<game *>::iterator g;
			for (g = watching.begin(); g != watching.end(); ++g)
				ss << "Game " << (*g)->id << ":\n"
					<< (*g)->print_board() << "\n";
			if (watching.size() == 0)
				ss << "You are not observing any games.\n";
			return ss.str();
		}

		string watch_list() {
			stringstream ss;
			vector<game *>::iterator g;
			for (g = watching.begin(); g != watching.end(); ++g)
			 	ss << (*g)->id << " ";
			return ss.str();
		}

		bool is_watching(int gid) {
			vector<game *>::iterator g;
			for (g = watching.begin(); g != watching.end(); ++g)
				if ((*g)->id == gid) return true;
			return false;
		}

		string get_oppon() {
			if (playing && match) {
				if (match->player[0] == name)
					return match->player[0];
				else
					return match->player[1];
			}
			return "";
		}

		void clear_game(int won) {
			playing = false;
			vector<game *>::iterator g;
			for (g = watching.begin(); g != watching.end(); ++g)
				if (*g == match) watching.erase(g);
			match = 0;
			if (won == 1) ++wins;
			else if (won == 0) ++loses;
			rating = (double) wins / (wins + loses);
		}


		//****************** BLOCKING **************************//
		bool is_blocked(string u) {
			//vector<user> iterator i;
			if (blocked.empty()) return false;
			for (unsigned int i = 0; i < blocked.size(); ++i)
				if (u == blocked[i])
					return true;
			return false;
		}

		int block(user u) {
			for (unsigned int i = 0; i < users.size(); ++i) {
				if (u.name == users[i].name) {
					if (is_blocked(u.name))
						return 0;
					else {
						blocked.push_back(u.name);
						return 1;
					}
				}
			}
			return -1;
		}



		//****************** MAILING ***************************//
		string list_mail() {
			//vector<mail> iterator m;
			string msg = "";
			for (unsigned int m = 0; m < inbox.size(); ++m)
				msg += inbox[m].show_meta();
			return msg;
		}

		string read_mail(int id) {
			stringstream ss;
			for (unsigned int m = 0; m < inbox.size(); ++m)
				if (inbox[m].id == id) {
					ss << inbox[m].read();
					return ss.str();
				}
			ss << "Message number invalid\n";
			return ss.str();
		}

		void add_mail(mail &m) {
			m.id = (int)inbox.size();
			inbox.push_back(m);
		}

		bool del_mail(int id) {
			vector<mail>::iterator m, n;
			for (m = inbox.begin(); m != inbox.end(); ++m)
				if ((*m).id == id) {
					n = m--;
					inbox.erase(n);
					for (; n != inbox.end(); ++n)
						(*n).id--;
					return true;
				}
			return false;
		}


};


string game_matcher(user &u, vector<string> v, user *other) {
	stringstream ss;
	if (v.size() < 2)
		ss << "match <user> <b|w> <t>\n";
	else if (u.playing)
		ss << "Please finish a game before starting a new one.\n";
	else if (v[1] == u.name)
		ss << "You can't match yourself.\n";
	else {
		vector<user>::iterator usr;
		for (usr = users.begin(); usr != users.end(); ++usr)
			if (v[1] == usr->name) {
				if (usr->online == false)
					ss << "User " << usr->name << " is not online.\n";
				else if (u.req_match(v) == -1)
					ss << "sent game request to " << v[1] << "\n";
				else { // initiate game for second player
					usr->accept_match(u.match);
					ss << (u.match)->print_board();
					other = &(*usr);
				}
				break;
			}
		if (v[1] != usr->name)
			ss << v[1] << " is not a user.\n";
	}
	return ss.str();
}


string game_resign(user &u, user **oth) {
	stringstream ss;
	int valid = 0;
	*oth = 0;
	string opp = u.get_oppon();
	ss << u.quit_match(&valid);
	vector<user>::iterator usr;
	if (valid) {
		for (usr = users.begin(); usr != users.end(); ++usr)
			if (opp == usr->name) {
				*oth = &(*usr);
				(*oth)->clear_game(1);
				break;
			}
	}
	return ss.str();
}

#endif
