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

class game {
	public:
		int id;
		char board[9];
		
		bool fin;
		time_t time_A;
		time_t time_B;
		string player[2];
		int moves;
		int turn;
		vector<string> observing;
		bool b_w;

		game(int g_id, string u_A, string u_B, bool color, time_t t = 600) {
			id = g_id;
			time_A = time_B = t;
			player[0] = u_A;
			player[1] = u_B;
			moves = 0;
			for (int i = 0; i < 9; ++i)
				board[i] = '.';
			turn = 0;
			b_w = color;
		}

		
		int move(string u, string choice) {
			char mark;
			int spot = get_spot(choice);
			if (spot < 0) return spot;
			if (u == player[0])
				mark = icon[0];
			else if (u == player[1])
				mark = icon[1];
			else return -3;
			
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
				return -1;
			spot = (str[0] - 'A') * 3; // go to row
			spot += (str[1] - '1'); // go to col
			if (board[spot] != '.') // check if empty
				return 0;
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

		
		void add_observer(string u_name) {
			stringstream ss;
			observing.push_back(u_name);
		}
};




#endif
