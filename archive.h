/*
*  COP5570    |    Parallel, Concurrent, Distributed Programming
*  Asg #4     |    Tic Tac Toe Game Server
*  Summer C   |    07/24/15
*
*     by Adam Stallard, Steven Rohr
*
*/

#include <fstream>
#include <iostream>
#include <time.h>
#include <cstdio>
#include "user.h"

#define USER_DAT "data/users.dat"
#define USER_SWP "data/users.dat.swap"
#define SERV_LOG "data/server.log"
#define SWP_BACK "data/backup.dat"

void create_swap() {
	time_t t;
	struct tm *now;
	char stamp[80];

	ofstream slog(SERV_LOG, ios_base::out | ios_base::app);
	t = time(NULL);
	now = localtime(&t);
	strftime(stamp, sizeof(stamp), "\n%Y-%m-%d.%X\n", now);
	slog << stamp << "Begin swap overwrite\n";

	ifstream orig(USER_DAT, ios_base::binary);
	ofstream swap(USER_SWP, ios_base::binary | ios_base::trunc);
	swap << orig.rdbuf();
	orig.close();
	swap.close();

	t = time(NULL);
	now = localtime(&t);
	strftime(stamp, sizeof(stamp), "\n%Y-%m-%d.%X\n", now);
	slog << stamp << "Swap overwrite complete\n";
	slog.close();
}

void create_udat() {
	time_t t;
	struct tm *now;
	char stamp[80];

	ofstream slog(SERV_LOG, ios_base::out | ios_base::app);
	t = time(NULL);
	now = localtime(&t);
	strftime(stamp, sizeof(stamp), "\n%Y-%m-%d.%X\n", now);
	slog << stamp << "Begin udat overwrite\n";

	ofstream udat(USER_DAT, ios_base::out | ios_base::trunc);
	vector<user>::iterator u;
	udat << users.size() << "\n"; // metadata
	for (u = users.begin(); u != users.end(); ++u)
		udat << (*u);
	udat.close();

	t = time(NULL);
	now = localtime(&t);
	strftime(stamp, sizeof(stamp), "\n%Y-%m-%d.%X\n", now);
	slog << stamp << "Udat overwrite complete\n";
	slog.close();
}

int save_server() {
	create_swap(); // save orig data in case of crash during overwrite
	create_udat(); // update curr user data file

	// create back up of swap just in case
	ifstream swap(USER_DAT, ios_base::binary);
	ofstream back(USER_SWP, ios_base::binary | ios_base::trunc);
	back << swap.rdbuf();
	swap.close();
	back.close();

	// remove swap
	if (remove(USER_SWP) != 0)
		cerr << "Error deleting swap file\n";

	return 0;
}

void load_users(ifstream &ifs) {
	user u;
	int num_users = 0;
	string line;

	// get metadata
	getline(ifs, line);
	for (char c : line) {
		if (isdigit(c)) {
			num_users *= 10;
			num_users += (c - '0');
		}
	}

	// clear curr user vector and get users from file
	users.clear();
	for (int i = 0; i < num_users; ++i) {
		ifs >> u;
		users.push_back(u);
	}

	ifs.close();
}

int load_server() {
	int res;
	ifstream udat(USER_DAT, ios_base::in);
	ifstream swap(USER_SWP, ios_base::in);
	ifstream back(SWP_BACK, ios_base::in);

	cout << "Initiating server data load\n" << flush;
	if (swap) {
		cerr << "Server may have crashed unexpectedly on last run\n"
			<< "\tLoading server from swap file\n";
		res = 1;
		load_users(swap);
	}
	else if (udat) {
		res = 0;
		load_users(udat);
	}
	else if (back) {
		cerr << "Both user data and swap files are missing\n"
			<< "\tLoading server from backup file\n";
		res = 2;
		load_users(back);
	}
	else {
		cerr << "All data and backup files are missing"
			<< "\tCould not load server\n";
		res = 3;
	}

	udat.close();
	swap.close();
	back.close();

	return res;
}
