
#include <vector>
#include <time.h>
#include <sstream>

#pragma once
#include "user.h"

using namespace std;

class mail {
	public:
		int id;
		bool open;
		string title;
		string from;
		time_t time;
		string body;

	mail(string m_title, user m_from, string m_body) {
		title = m_title;
		from = m_from.name;
		body = m_body;
		time = time(NULL);
		open = false;
	}
	~mail() {}

	string tstamp() {
		buf[20];
		struct tm *now = localtime(&time);
		strftime(buf, 20, "%c", now);
		return str(buf);
	}

	string show_meta() {
		stringstream ss;
		ss << id << " " << (open ? "  " : "N ")
			<< width(15) << from
			<< width(30) << title
			<< tstamp() << endl;
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
