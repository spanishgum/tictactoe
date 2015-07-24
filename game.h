/*
*  COP5570    |    Parallel, Concurrent, Distributed Programming
*  Asg #4     |    Tic Tac Toe Game Server
*  Summer C   |    07/24/15
*
*     by Adam Stallard, Steven Rohr
*
*/

#ifndef _GAME_H
#define _GAME_H

#include <vector>
#include <ctime>
#include <sstream>
#include <string>
#include <cctype>

using namespace std;

static const char icon[2] = { '#', 'O' };

class game;
vector<game> games;

int g_request(string, vector<string>);
game* g_lookup(string[2]);


class game {
	public:
		int id;
		char board[9];

		bool pending;
		bool fin;
		time_t t_A, t_B, timer;
		string player[2];
		int moves;
		int turn;
		vector<string> observing;
		bool b_w; // player 1 is black if 0



		game(int gid, string users[2], bool color, time_t t) {
			id = gid;
			t_A = t_B = t;
			player[0] = users[0];
			player[1] = users[1];
			moves = 0;
			for (int i = 0; i < 9; ++i)
				board[i] = '.';
			turn = 0;
			observing.push_back(users[0]);
			observing.push_back(users[1]);
			b_w = color;
			pending = true;
		}

		string move(string u, string choice) {
			stringstream ss;
			int result = try_move(u, choice);
			switch (result) {
				case -4: // this func was called improperly
					ss << "Something went wrong.\n\n";
					break;
				case -3:
					ss << "Please wait for your turn.\n\n"
						<< print_board() << "\n";
					break;
				case -2:
					ss << "Range out of bounds. Try again.\n\n"
						<< print_board() << "\n";
					break;
				case -1:
					ss << "Position occupied. Try again.\n\n"
						<< print_board() << "\n";
					break;
				default:
					ss << print_board() << "\n";
			}
			if (moves > 4) check_win();
			return ss.str();
		}


		int try_move(string u, string choice) {
			char mark;
			int spot = get_spot(choice);
			if (spot < 0) return spot;
			else if (u != player[turn]) return -3;
			if (u == player[0])
				mark = icon[0];
			else if (u == player[1])
				mark = icon[1];
			else return -4;

			board[spot] = mark;

			++moves;

			if (moves == 8) {
				fin = true;
				return 0;
			}

			turn = !turn;
			/* stop timer for player, start timer for other ***********************/
		}

		int get_spot(string str) {
			int spot;
			if (str[0] < 'A' || str[0] > 'C' || str[1] < '1' || str[1] > '3') // double check range
				return -2;
			spot = (str[0] - 'A') * 3; // go to row
			spot += (str[1] - '1'); // go to col
			if (board[spot] != '.') // check if empty
				return -1;
			else return spot; // good value
		}

		string print_board() {
			stringstream ss;
			/* metadata */
			ss << "Black:";
			ss.width(15); ss << right << player[0];
			ss << "       White:";
			ss.width(15); ss << right << player[1] << "\n";
			ss << " Time:    " << t_A << " seconds"
				<< "        Time:    " << t_B << " seconds\n";
			/* actual board */
			ss << "   1  2  3\n";
			for (int i = 0; i < 9; ++i) {
				if (i % 3 == 0)
					ss << (char)('A' + (i / 3));
				ss << "  " << board[i];
				if (i % 3 == 2)
					ss << "\n";
			}
			return ss.str();
		}

		bool check_win() {
			bool win = 0;
			char c;
			if ((c = board[0]) != '.') {
				if (board[1] == c) {
					if (board[2] == c)
						win = 1;       /* 1 2 3 */
				}
				else if (board[4] == c) {
					if (board[8] == c)
						win = 1;       /* 1 5 9 */
				}
				else if (board[3] == c) {
					if (board[6] == c)
						win = 1;       /* 1 4 7 */
				}
			}
			else if ((c = board[1]) != '.') {
				if (board[4] == c) {
					if (board[7] == c)
						win = 1;       /* 2 5 8 */
				}
			}
			else if ((c = board[2]) != '.') {
				if (board[4] == c) {
					if (board[6] == c)
						win = 1;       /* 3 5 7 */
				}
				else if (board[5] == c) {
					if (board[8] == c)
						win = 1;       /* 3 6 9 */
				}
			}
			else if ((c = board[3]) != '.') {
				if (board[4] == c) {
					if (board[5] == c)
						win = 1;       /* 4 5 6 */
				}
			}
			else if ((c = board[6]) != '.') {
				if (board[7] == c) {
					if (board[8] == c)
						win = 1;       /* 7 8 9 */
				}
			}
			fin = win;
			return win;
		}


		bool is_observer(string u_name) {
			stringstream ss;
			vector<string>::iterator u;
			for (u = observing.begin(); u != observing.end(); ++u)
				if ((*u) == u_name)
					return true;
			return false;
		}


		string add_observer(string u_name) {
			stringstream ss;
			if (is_observer(u_name))
				ss << "You are already watching this game.\n\n";
			else {
				ss << "You are now watching game " << id << ".\n\n";
				observing.push_back(u_name);
			}
			ss << print_board();
			ss << "\n";
			return ss.str();
		}


		string rem_observer(string u_name) {
			stringstream ss;
			if (is_observer(u_name))
				ss << "You are not watching game " << id << ".\n\n";
			else {
				ss << "You are no longer watching game " << id << ".\n\n";
				vector<string>::iterator u;
				for (u = observing.begin(); u != observing.end(); ++u)
					if (*u == u_name) {
						observing.erase(u);
						break;
					}
			}
			return ss.str();
		}


};





int g_request(string u_name, vector<string> &argv) {
	string player[2];
	int color = -1;
	time_t timer = -1;
	game *g_new;
	bool accept = 0;

	player[0] = u_name; // sender

	// parse 'match <user> <b|w> <t>'
	switch(argv.size()) {
		case 1:
			return -1;
		case 2:
			player[1] = argv[1]; // reciever
			color = 0; // default black
			break;
		default:
			player[1] = argv[1]; // reciever
			for (unsigned int i = 2; i < argv.size(); ++i) {
				if (isdigit(argv[i][0]) && timer < 0)
					timer = stoi(argv[i]);
				else if (color < 0)
					color = (tolower(argv[i][0]) == 'w') ? 1 : 0;
			}
	}

	// enforce timer range
	if (timer < 0 || timer > 600) timer = 600;

	// check for existing game or init new one
	if((g_new = g_lookup(player)) != 0) {
		accept |= (g_new->b_w == (bool)color);
		accept |= (g_new->timer == timer);
		if (accept) // game begins
			return 1;
		else { // update pending request details
			g_new->timer = g_new->t_A = g_new->t_B = timer;
			g_new->b_w = (u_name == g_new->player[0]) ? (bool)color : (bool)!color;
		}
	}
	else {
		g_new = new game(games.size(), player, color, timer);
		games.push_back(*g_new);
		delete g_new;
	}

	// new pending request
	return 0;
}




game* g_lookup(string player[2]) {
	vector<game>::iterator g;
	for (g = games.begin(); g != games.end(); ++g) {
		if (((*g).player[0].compare(player[0])
			|| (*g).player[1].compare(player[1])) == 0)
			return &*g;
		else if (((*g).player[0].compare(player[1])
			|| (*g).player[1].compare(player[0])) == 0)
			return &*g;
	}
	return NULL;
}



#endif
