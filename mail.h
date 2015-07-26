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
		time_t timestamp;
		string title;
		string from;
		string body;

	friend ostream& operator <<(ostream &ofs, mail m) {
		ofs << m.id << "\n" << m.open << "\n" << m.title << "\n"
			<< m.from << "\n" << (int)m.timestamp << "\n" << m.body;
			// '.' on one line denotes end of mail
		return ofs;
	}

	friend istream& operator >>(istream &ifs, mail &m) {
		stringstream form, tmp;
		string data, sub_data;
		string line;
		int num_lines = 0;
		do {
			getline(ifs, line);
			form << line << "\n"; // \n removed, add it back
			++num_lines;
		} while (line.compare(".") != 0); // end of mail marker

		// double check mail meta data quality
		if (num_lines < 6) {
			if (num_lines > 1)
				cerr << "Back up data file may be corrupted. . .\n"
					<< "\tAborting load for curr mail object\n";
			else
				cerr << "Ivalid meta data for user mail content. . .\n"
					<< "\tAborting load for curr mail object\n";
			return ifs;
		}

		// proceed to grab mail data
		getline(form, data); m.id = _stoi(data);
		getline(form, data); m.open = (bool) _stoi(data);
    getline(form, data); m.title = data;
		getline(form, data); m.from = data;
		getline(form, data); m.timestamp = (time_t) _stoi(data);
		m.body = form.str().substr(form.tellg()); // remainder will be the body

		return ifs;
	}

	// wrapper for stoi to prevent aborts in >> overload
	static int _stoi(string data) {
		string sub = "";
		for(char c : data) {
			if (isdigit(c)) {
				sub += c;
			}
			else if (c == '-') {
				if (sub == "") {
					sub += c;
				}
				else break;
			}
			else break; // only grab leading digits in string
		}
		return stoi(sub);
	}


	mail(string m_title = "", string m_from = "", string m_body = ".\n") {
		id = -1;
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
