#ifndef _GAME_H
#define _GAME_H

#include <vector>
#include <ctime>
#include <sstream>
#include <string>

using namespace std;

static const char icon[2] = { '#', 'O' };
class game;

vector<game> games;

int match(u_name, vector<string> argv) {
	string players[2], color;
	player[0] = u_name;
	player[1] = argv[1];
	color = (argv[2].size() == 0) ? 0 : 
		tolower(argv[2][0]) == 'w' ? 1 : 0;
	
	game new_game(games.size(), players, argv[2], argv[3]);
}

class game {
	public:
		int id;
		char board[9];
		
		bool pending;
		bool fin;
		time_t time_A;
		time_t time_B;
		string player[2];
		int moves;
		int turn;
		vector<string> observing;
		bool b_w;
		
		

		game(int gid, string users[2], bool color, time_t t = 600) {
			id = gid;
			time_A = time_B = t;
			player[0] = users[0];
			player[1] = users[1];
			moves = 0;
			for (int i = 0; i < 9; ++i)
				board[i] = '.';
			turn = 0;
			observing.push_back(u_A);
			observing.push_back(u_B);
			b_w = color;
			pending = true;
		}

		string move(string u, string choice) {
			stringstream ss;
			int result = try_move(u, choice);
			switch (result) {
				case -4:
					ss << "You are not a player of this game.\n Something went wrong.\n\n";
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
			ss << " Time:    " << time_A << " seconds"
				<< "        Time:    " << time_B << " seconds\n";
			/* actual board */
			ss << "   1  2  3\n";
			for (int i = 0; i < 9; ++i) {
				if (i % 3 == 0)
					ss << (char)('A' + (i / 3));
				ss << "  " << board[i];
				if (i % 3 == 2)
					ss << "\n";
			}	
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




#endif
