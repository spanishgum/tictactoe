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
#include "user.h"

#define USER_DAT "users.dat"

void save_user(user &u) {
	ofstream ofs;
	ofs.open(USER_DAT, ios_base::out);
	ofs << u;
	ofs.close();
}


void load_user(user &u) {
	ifstream ifs;
	ifs.open(USER_DAT, ios_base::in);
	ifs >> u;
	ifs.close();
}


int save() {

	return 0;
}

int load() {

	return 0;
}
