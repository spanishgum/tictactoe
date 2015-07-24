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
		vector<mail> inbox;
		game *match;
		bool playing;

		int clientcounter;
		int cli_sockfd;

		friend bool operator ==(user &l, user &r) {
			if ((l.name).compare(r.name) == 0)
				return true;
			else return false;
		}

		user(string u_name = "", string u_passwd = "") {
			name = u_name;
			passwd = u_passwd;
			info = "";
			rating = wins = loses = quiet =
			clientcounter = 0;
			online = false;
			match = 0;
		}

		string stats() {
			stringstream ss;
			ss << "\nUser: " << name
				<< "\nInfo: " << info
				<< "\nRating: " << rating
				<< "\nWins: " << wins
				<< ", Loses: " << loses
				<< "\nQuiet: " << (quiet ? "Yes" : "No")
				<< "\nBlocked users:";
			if (blocked.empty())
				ss << " <none>";
			else {
				for (unsigned int i = 0; i < blocked.size(); ++i)
					ss << " " << blocked[i];
			}
			ss << "\n\n" << name
				<< " is currently "
				<< (online ? "online.\n" : "offline.\n");

			return ss.str();
		}


		void accept_match(game &g) {
			match = &g;
			playing = true;
		}


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



#endif
