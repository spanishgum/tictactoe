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
	ofstream ofs(USER_DAT, "a");

	ofs.close();
}


void load_user(user &u) {
	ifstream ifs(USER_DAT, "r");
	char form_feed;


	ifs.close();
}


int save() {


}

int load() {

	
}
