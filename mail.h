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

// used in >> overload
#define grab_data() \
	getline(form, data); \
	data.pop_back()

using namespace std;

class mail {
	public:
		int id;
		bool open;
		string title;
		string from;
		time_t timestamp;
		string body;

	friend ostream& operator <<(ostream &ofs, mail m) {

		ofs << m.id << "\n" << m.open << "\n" << m.title << "\n"
			<< m.from << "\n" << (int)m.timestamp << "\n" << m.body
			<< "\v"; // denotes end of mail
		return ofs;
	}
	friend istream& operator >>(istream &ifs, mail &m) {
		stringstream form, tmp;
		string data, sub_data;
		char c;
		do {
			c = ifs.get();
			form << c;
		} while (c != '\v'); // end of mail marker

		if (form.str().size() < 2) return ifs; // no mail


		grab_data(); m.id = stoi(data);
		grab_data(); m.open = (bool) stoi(data);
        grab_data(); m.title = data;
		grab_data(); m.from = data;
		grab_data(); m.timestamp = (time_t) stoi(data);

		/* rest of form is the body */
		m.body = form.str();
		(m.body).pop_back();
		return ifs;
	}

	mail();
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
