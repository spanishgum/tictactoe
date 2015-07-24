/*
*  COP5570    |    Parallel, Concurrent, Distributed Programming
*  Asg #4     |    Tic Tac Toe Game Server
*  Summer C   |    07/24/15
*
*     by Adam Stallard, Steven Rohr
*
*/

#ifndef _MAIL_H
#define _MAIL_H

#include <vector>
#include <ctime>
#include <sstream>
#include <string>
#include <iomanip>

using namespace std;

class mail {
	public:
		int id;
		bool open;
		string title;
		string from;
		time_t timestamp;
		string body;

	mail(string m_title, string m_from, string m_body) {
		title = m_title;
		from = m_from;
		body = m_body;
		timestamp = time(NULL);
		open = false;
	}

	string tstamp() {
		char buf[31] = {'\0'};  stringstream ss;
		struct tm *now = localtime(&timestamp);

		strftime(buf, 30, "%c", now);
		ss << buf;
		return ss.str();
	}

	string show_meta() {
		stringstream ss;
		ss << id << " " << (open ? "  " : "N ")
			<< setfill(' ') << setw(15) << from
			<< setfill(' ') << setw(30) << title
			<< "  " << tstamp() << endl;
		return ss.str();
	}

	string read() {
		stringstream ss;
		ss << "\nFrom: " << from
			<< "\nTitle: " << title
			<< "\nTime: " << tstamp()
			<< "\n\n" << body;
		return ss.str();
	}

};




#endif
