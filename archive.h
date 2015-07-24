
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
	ofs << u.name.length()
		<< u.name
		<< u.name.passwd.length()
		<< u.name.passwd
		<< u.name.info.length()
		<< u.name.info
		<< u.rating
		<< u.wins
		<< u.loses
		<< u.quiet
//		<< u.online
		<< u.blocked.size()
		<< u.blocked // *** fix to print the string vector
		<< u.inbox.size()
		<< u.inbox // *** fix to print the mail vector (possibly in other file)
//		<< *(u.match)
		<< u.playing
		<< u.cli_sockfd
		
		<< "\f"; // form feed character
	ofs.close();
}

<template T>
void write_vector(ofstream &ofs, vector<T> v) {
	vector<T>::iterator i = v.begin();
	ofs << v.size();
	while (i != v.end())
		ofs << (*i).size() << (*i).data();
}


void load_user(user &u) {
	ifstream ifs(USER_DAT, "r");
	char form_feed;
	
	ifs << u.name.length()
		<< u.name
		<< u.name.passwd.length()
		<< u.name.passwd
		<< u.name.info.length()
		<< u.name.info
		<< u.rating
		<< u.wins
		<< u.loses
		<< u.quiet
//		<< u.online
		<< u.blocked.size()
		<< u.blocked // *** fix to print the string vector
		<< u.inbox.size()
		<< u.inbox // *** fix to print the mail vector (possibly in other file)
//		<< *(u.match)
		<< u.playing
		<< u.cli_sockfd;
		
		<< form_feed; // form feed character
		
	if (form_feed != '\f')
		cerr << "Something went wrong. . ."
			<< "file may be corrupted, or this program is broken\n";
	
	ifs.close();
}


<template T>
void read_vector(ifstream &ifs, vector<T> &v) {
	unsigned int v_sz, i_sz;
	T data;
	ifs >> v_sz;
	for (int i = 0; i < v_sz; ++i) {
		ifs >> i_sz;
		for (int j = 0; j < i_sz; ++j) {
			ifs >> data[j];
		}
	}
}

