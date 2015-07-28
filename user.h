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

#define BLOCK_LIM 10

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
		mail *outgoing; // could have done full outbox - kept simple
		game *match;
		bool playing;
		bool writing_mail;

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
			u.online = 0;
			u.outgoing = 0;
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
			if ((u.wins + u.loses) == 0) u.rating = 0.00;
			else u.rating = (double)u.wins / (u.wins + u.loses);
			getline(form, data); u.quiet = (bool) _stoi(data);

			// input blocked list
			getline(form, data); // line contains blocked users
			tmp << data; // put line into stream
			tmp >> sub_data;
			while (tmp) { // tokenize
				u.blocked.push_back(data);
				tmp >> sub_data;
			}

			// input mail data
			getline(form, data); num_msgs = _stoi(data);
			for (unsigned int i = 0; i < num_msgs; ++i) {
				form >> m;
				m.id = u.inbox.size();
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



		//****************** CONSTRUCTOR ***********************//
		user(string u_name = "", string u_passwd = "") {
			name = u_name;
			passwd = u_passwd;
			info = "";
			rating = wins = loses = clientcounter = 0;
			match = 0;
			outgoing = 0;
			quiet = online = playing = writing_mail = false;
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
			accept_match(&games[req]);
			if (match->pending) return -1;
			else return req;
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
			if (match && playing)
				if (match->id == gid) {
					ss << "You can not unobserve your own game.\n";
					return ss.str();
				}
			if (gid == -1) {
				if (watching.empty())
					ss << "You are not observing anything\n";
				else {
					for(g = watching.begin(); g != watching.end(); ++g) {
						if (match) {
							if (match->id == (*g)->id) continue;
						}
						else {
							ss << (*g)->rem_observer(name);
							watching.erase(g);
							break;
						}
					}
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
					return match->player[1];
				else
					return match->player[0];
			}
			return "";
		}

		void clear_game(int won) {
			playing = false;
			vector<game *>::iterator g;
			for (g = watching.begin(); g != watching.end(); ++g)
				if (*g == match) {
					watching.erase(g);
					break;
				}
			match = 0;
			if (won == 1) ++wins;
			else if (won == 0) ++loses;
			rating = (double) wins / (wins + loses);
		}

		string move(string input) {
			stringstream ss;
			if (!playing)
				ss << "You are not currently playing a game.\n";
			else if (!match) {
				cerr << "Warning, client game match may be out of sync.\n";
				ss << "You are not currently playing a game.\n";
			}
			else
				ss << match->move(name, input);
			return ss.str();
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

		int block(string u) {
			for (unsigned int i = 0; i < users.size(); ++i) {
				if (u == users[i].name) {
					if (is_blocked(u))
						return 0;
					else {
						if (blocked.size() > BLOCK_LIM)
							return -2;
						blocked.push_back(u);
						return 1;
					}
				}
			}
			return -1;
		}

		int unblock(string u) {
			for (unsigned int i = 0; i < users.size(); ++i)
				if (users[i].name == u) {
					for (unsigned int j = 0; j < blocked.size(); ++j) {
						if (u == blocked[j]) {
							blocked.erase(blocked.begin() + j);
							return 1;
						}
					}
					return 0;
				}
			return -1;
		}



		//****************** MAILING ***************************//
		string mail_meta() {
			stringstream ss;
			int new_mail = 0;
			for (unsigned int i = 0; i < inbox.size(); ++i)
				if (!inbox[i].open) ++new_mail;
			if (new_mail)
				ss << "You have " << new_mail << " new messages in your inbox.\n";
			else ss << "Inbox empty.\n";
			return ss.str();
		}

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
					inbox[m].open = true;
					return ss.str();
				}
			ss << "Message number invalid.\n";
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


//************** GLOBAL USER UTILITIES *******************//
void clr_observers(int gid) {
	vector<user>::iterator u;
	for (u = users.begin(); u != users.end(); ++u)
		if (u->is_watching(gid))
			u->unobserve(gid);
}

string game_matcher(user &u, vector<string> v, user **other) {
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
				else if (u.req_match(v) == -1) {
					ss << "sent game request to " << v[1] << "\n";
					*other = &(*usr);
				}
				else { // initiate game for second player
					usr->accept_match(u.match);
					ss << "You have accepted the match.\n"
						<< "You are now playing in game " << (u.match)->id << "\n\n";
					ss << (u.match)->print_board();
					*other = &(*usr);
				}
				return ss.str();
			}

		ss << v[1] << " is not a user.\n";
	}
	return ss.str();
}

string game_resign(user &u, user **other) {
	stringstream ss;
	int valid = 0;
	int gid = -1;
	if (u.match)
		gid = (u.match)->id;
	string opp = u.get_oppon();
	ss << u.quit_match(&valid);
	vector<user>::iterator usr;
	if (valid) {
		for (usr = users.begin(); usr != users.end(); ++usr)
			if (opp == usr->name) {
				*other = &(*usr);
				(*other)->clear_game(1);
				break;
			}
	}
	clr_observers(gid);
	rem_game(gid);
	return ss.str();
}

void game_fin(user &u) {
	bool won;
	int gid;
	if (!u.match) return;
	gid = (u.match)->id;
	string opp = u.get_oppon();

	if ((u.match)->winner == -1) won = -1;
	else if ((u.match)->player[(u.match)->winner] == u.name) won = 0;
	else won = 1;

	vector<user>::iterator usr;
	for (usr = users.begin(); usr != users.end(); ++usr)
		if (opp == usr->name) {
			usr->clear_game(!won);
			break;
		}
	if (opp != usr->name)
		cerr << "Warning: game data may be out of sync.\n";
	u.clear_game(won);
	clr_observers(gid);
	rem_game(gid);
}


void send_mail(user &u) {
	if (u.writing_mail || !u.outgoing) return;
	vector<user>::iterator usr;
	for (usr = users.begin(); usr != users.end(); ++usr)
		if (usr->name == (u.outgoing)->to)
			if (!usr->is_blocked(u.name))
				usr->add_mail(*u.outgoing);
}

#endif
