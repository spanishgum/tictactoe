#include <vector>
#include <time.h>
#include <sstream>

using namespace std;

extern vector<user> users;

class mail;
class game;

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
		vector<user> blocked;
		vector<mail> inbox;

		sockfd_t cli_sockfd;
		bool &connected = online;

		friend bool operator ==(user &l, user &r) {
			if ((l.name).compare(r.name) == 0)
				return true;
			else return false;
		}

		user(string u_name, string u_passwd) {
			this.name = u_name;
			this.passwd = u_passwd;
			rating = wins = loses = quiet = 0;
		}
		~user(){}

		string stats() {
			sstream ss;
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
				vector<user> iterator i;
				for (i = blocked.begin(); i != blocked.end(); ++i)
					ss << " " << (*i).name;
			}
			ss << "\n\n" << name
				<< " is currently "
				<< (online ? "online.\n" : "offline.\n");

			return ss.str();
		}


		bool is_blocked(user u) {
			vector<user> iterator i;
			if (blocked.empty()) return false;
			for (i = blocked.begin(); i != blocked.end(); ++i)
				if (u == *i)
					return true;
			return false;
		}


		int block(user u) {
			vector<user> iterator i;
			for (i = users.begin(); i != users.end(); ++i) {
				if (u == *i) {
					if (is_blocked(u))
						return 0;
					else return 1;
				}
			}
			return -1;
		}


		void list_mail() {
			vector<mail> iterator m;
			for (m = inbox.begin(); m != inbox.end(); ++m)
				(*m).show_meta();
		}


		bool read_mail(int id) {
			vector<mail> iterator m;
			for (m = inbox.begin(); m != inbox.end(); ++m)
				if ((*m).id == id) {
					(*m).read();
					return true;
				}
			return false;
		}


		void add_mail(mail m) {
			m.id = inbox.size();
			inbox.push_back(m);
		}


		bool del_mail(int id) {
			vector<mail> iterator m, n;
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


class mail {
	public:
		int id;
		bool open;
		string title;
		user from;
		time_t time;
		string body;

	mail(string m_title, user m_from, string m_body) {
		this.title = m_title;
		this.from = m_from;
		this.body = m_body;
		this.time = time(NULL);
		this.open = false;
	}
	~mail() {}

	string tstamp() {
		buf[20];
		struct tm *now = localtime(&time);
		strftime(buf, 20, "%c", now);
		return str(buf);
	}

	string show_meta() {
		sstream ss;
		ss << id << " " << (open ? "  " : "N ")
			<< width(15) << from.name
			<< width(30) << title
			<< tstamp() << endl;
		return ss.str();
	}

	string read() {
		sstream ss;
		ss << "\nFrom: " << from.name
			<< "\nTitle: " << title
			<< "\nTime: " << tstamp()
			<< "\n\n" << body;
		return ss.str();
	}

};


class game {
	public:
		int id;
		char[9] board;
		bool fin;
		time_t time_A;
		time_t time_B;
		user user_A;
		user user_B;
		int moves;
		vector<user> observing;

};
