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

pthread_mutex_t games_lock = PTHREAD_MUTEX_INITIALIZER;
vector<game> games;

int g_request(string, vector<string>);
game* g_lookup(string[2]);


class game {
	public:
		int id;
		char board[9];

		bool pending;
		string requester;
		bool fin;
		int winner;
		time_t timer[3]; // [2] uses as stop watch
		string player[2]; // player 0 always black
		int moves;
		int turn;
		vector<string> observing;


		//****************** CONSTRUCTOR ***********************//
		game(int gid, string users[2], time_t t) {
			id = gid;
			timer[0] = timer[1] = t;
			player[0] = users[0];
			player[1] = users[1];
			moves = 0;
			for (int i = 0; i < 9; ++i)
				board[i] = '.';
			turn = 0;
			observing.push_back(users[0]);
			observing.push_back(users[1]);
			pending = true;
			fin = false;
		}



		//****************** MECHANICS *************************//
		string move(string u, string choice) {
			stringstream ss;
			int result = try_move(u, choice);
			ss << "\n" << print_board() << "\n";
			switch (result) {
				case -4: // this func was called improperly
					cerr << "Something went wrong.\n\n";
					break;
				case -3:
					ss << "Please wait for your turn.\n\n";
					break;
				case -2:
					ss << "Range out of bounds. Try again.\n\n";
					break;
				case -1:
					ss << "Position occupied. Try again.\n\n";
					break;
				case 10:
					ss << "Your time ran out. You lose.\n"
						<< player[turn] << " wins!!\n";
					winner = !turn;
					fin = true;
					/* initiate game remover */
					break;
				default:

					if (moves > 4) {
						if (check_win()) {
							ss << "Congratualtions, you have won the game!!\n\n";
							fin = true;
							winner = turn;
						}
						else if (moves == 8) {
							ss << "Cat game.\n\n";
							fin = true;
							winner = -1;
						}
					}
			}

			return ss.str();
		}

		int try_move(string u, string choice) {
			int spot;

			// not there turn or invalid request
			if (u != player[turn]) {
				if (u != player[!turn]) return -4;
				else return -3;
			}

			// update remaining user time and clock timer
			timer[turn] -= difftime(time(NULL), timer[2]);
			timer[2] = time(NULL);

			// check if time ran out
			if (timer[turn] < 1)
				return 10;

			// get spot or error code (-1 occupied, -2 bad range)
			if ((spot = get_spot(choice)) < 0)
				return spot;

			// update game data
			board[spot] = icon[turn];
			++moves;

			// change turns
			turn = !turn;
			return 0;
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
			if ((c = board[1]) != '.') {
				if (board[4] == c) {
					if (board[7] == c)
						win = 1;       /* 2 5 8 */
				}
			}
			if ((c = board[2]) != '.') {
				if (board[4] == c) {
					if (board[6] == c)
						win = 1;       /* 3 5 7 */
				}
				else if (board[5] == c) {
					if (board[8] == c)
						win = 1;       /* 3 6 9 */
				}
			}
			if ((c = board[3]) != '.') {
				if (board[4] == c) {
					if (board[5] == c)
						win = 1;       /* 4 5 6 */
				}
			}
			if ((c = board[6]) != '.') {
				if (board[7] == c) {
					if (board[8] == c)
						win = 1;       /* 7 8 9 */
				}
			}
			return win;
		}

		void quit(string u) {
			if (fin) return; // shouldn't happen
			fin = true;
			if (u == player[0])
				winner = 1;
			else if (u == player[1])
				winner = 0;
			else // shouldn't happen
				cerr << "Error: Game is out of sync. Players do not match.\n";
		}


		//****************** OBSERVERS *************************//
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

		string print_observers() {
			stringstream ss;
			vector<string>::iterator o;
			for (o = observing.begin(); o != observing.end(); ++o)
				ss << (*o) << " ";
			return ss.str();
		}



		//****************** STATUS ****************************//
		string print_board() {
			stringstream ss;
			/* metadata */
			ss << "Black:";
			ss.width(15); ss << right << player[0];
			ss << "       White:";
			ss.width(15); ss << right << player[1] << "\n";
			ss << " Time:    " << timer[0] << " seconds"
				<< "        Time:    " << timer[1] << " seconds\n";
			/* actual board */
			ss << "\n   1  2  3\n";
			for (int i = 0; i < 9; ++i) {
				if (i % 3 == 0)
					ss << (char)('A' + (i / 3));
				ss << "  " << board[i];
				if (i % 3 == 2)
					ss << "\n";
			}
			ss << "\n";
			return ss.str();
		}

		string stats() {
			stringstream ss;
			if (pending)
				ss << "Pending game. . .\n";
			ss << "id: " << id << "\n"
				<< print_board() << "\n"
				<< "moves: " << moves << "\n"
				<< "turn: ";
			if (turn) ss <<  "White\n";
			else ss << "Black\n";
			ss << "observing: " << print_observers() << "\n";
			if (fin)
				ss << "winner: " << player[winner] << "\n";
			return ss.str();
		}

		string metadata() {
			stringstream ss;
			ss << player[0] << " vs. " << player[1]
				<< ", " << moves << " moves\n";
			return ss.str();
		}


};



//****************** UTILITIES ***************************//
int _stoi(string data) {
	string sub = "";
	for(char c : data) {
		if (isdigit(c)) sub += c;
		else if (c == '-') {
			if (sub == "") sub += c;
			else break;
		}
		else break; // only grab leading digits in string
	}
	return stoi(sub);
}


int g_request(string u_name, vector<string> argv) {
	string player[2];
	int color = -1;
	time_t timer = -1;
	game *g_new;
	bool accept = 0;

	// parse 'match <user> <b|w> <t>'
	for (unsigned int i = 2; i < argv.size(); ++i) {
		if (argv[i].compare("") == 0)
			continue;
		else if (isdigit(argv[i][0]) && timer < 0) {
			timer = _stoi(argv[i]);
			if (timer < 0 || timer > 600)
				timer = 600; // enforce timer range
		}
		else if (color < 0) {
			if (argv[i].length() == 1) {
				if (tolower(argv[i][0]) == 'w')
					color = 1;
				else if (tolower(argv[i][0]) == 'b')
					color = 0;
			}
		}
	}


	player[0] = u_name; // sender
	player[1] = argv[1]; // reciever
	g_new = g_lookup(player); // find pending game


	// check for existing game or init new one
	if(g_new != NULL) {

		if (g_new->requester == u_name)
			return -1; // so user doesnt enter match <name> twice
		else
			g_new->requester = u_name;

		accept = true;

		if (color != -1) // check color only if parameter provided
			accept &= (g_new->player[0].compare(player[color]) == 0);

		if (timer != -1) // check timer only if parameter provided
			accept &= (g_new->timer[0] == timer);

		if (accept) { // game begins
			g_new->pending = false;
			g_new->timer[2] = time(NULL);
			return g_new->id; // returns game id
		}
		else { // update pending request details
			g_new->timer[0] = g_new->timer[1] = (timer < 0 || timer > 600) ? 600 : timer;
			g_new->player[0] = (color == 1) ? player[1] : player[0];
			g_new->player[1] = (color == 1) ? player[0] : player[1];
			g_new->requester = u_name;
		}
	}
	else {
		if (timer < 0 || timer > 600) timer = 600;
		if (color == 1) {
			player[0] = argv[1];
			player[1] = u_name;
		}
		g_new = new game(games.size(), player, timer);
		g_new->requester = u_name;
		games.push_back(*g_new);
		delete g_new;
	}

	// new pending request
	return g_new->id;
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


string show_games() {
	stringstream ss;
	unsigned int num_games = games.size();
	ss << "Total of " << num_games << " games(s):\n";
	vector<game>::iterator g;
	for (g = games.begin(); g != games.end(); ++g)
		ss << g->metadata();
	return ss.str();
}


void rem_game(int gid) {
	vector<game>::iterator g;
	for (g = games.begin(); g != games.end(); ++g)
		if (g->id == gid) {
			g = games.erase(g);
			for (; g != games.end(); ++g) {
				--(g->id);
			}
			break;
		}
}

#endif
