#include <iostream>
#include <cstdlib>

using namespace std;

char board[9];
/*
* 1 2 3
* 4 5 6 
* 7 8 9
*/

int get_spot(char *);
int check_win(char);
void print_board();

int main() {
	
	char curr_pl = 'O';  // O or #
	char move[3] = { '\0'};  // A1, B2, etc
	int win = 0;   //   0 = cat | 1 = O | 2 = #
	int rounds = 0;
	int spot;
	
	/* init board */
	for (int i = 0; i < 9; ++i)
		board[i] = '.';
	
	/* game loop */
	do {
		/* update round metadata */
		rounds++;
		curr_pl = (curr_pl == 'O') ? '#' : 'O';
		
		/* force valid input */
		do {
			/* this will need to be changed to take from client stream */
			cin >> move[0] >> move[1];
			cout << move[0] << move[1] << endl;
			spot = get_spot(&move[0]);
		} while (spot < 0);
		
		/* update board */
		board[spot] = curr_pl;
		print_board();
		
		win = check_win(curr_pl);
	} while (rounds < 9 && win == 0);
	
	
	/* determine game outcome */
	switch (win) {
		case 0:
			cout << "The game is tied.\n";
			break;
		case 1:
			cout << "O won the game.\n";
			break;
		case 2:
			cout << "# won the game.\n";
			break;
		default:
			cout << "You shouldn't see this...\n";
			exit(-1);
	}
	
	
	return 0;
}


int get_spot(char *str) {
	int spot;
	
	if (str[0] < 'A' || str[0] > 'C' || str[1] < '1' || str[1] > '3'){ // double check range
		cout << "Command not supported.\n";
		return -1;
	}
	
	spot = (str[0] - 'A') * 3; // go to row
	spot += (str[1] - '1'); // go to col
	
	if (board[spot] != '.') { // check if empty
		cout << str[0] << str[1] << " is occupied.\n";
		return -1;
	}
	else return spot; // good value
}


void print_board() {
	
	/* metadata */
	cout << "Black:";
	cout.width(15); cout << right << "<userA>";
	cout << "       White:";
	cout.width(15); cout << right << "<userB>\n";
	cout << " Time:    577 seconds        Time:    582 seconds\n";
	
	/* actual board */
	cout << "   1  2  3\n";
	for (int i = 0; i < 9; ++i) {
		if (i % 3 == 0)
			cout << (char)('A' + (i / 3));
		cout << "  " << board[i];
		if (i % 3 == 2)
			cout << "\n";
	}
}



int check_win(char c) {
	int win = 0;
	if (board[0] == c) {
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
	else if (board[1] == c) {
		if (board[4] == c) {
			if (board[7] == c)
				win = 1;       /* 2 5 8 */
		}
	}
	else if (board[2] == c) {
		if (board[4] == c) {
			if (board[6] == c)
				win = 1;       /* 3 5 7 */
		}
		else if (board[5] == c) {
			if (board[8] == c)
				win = 1;       /* 3 6 9 */
		}
	}
	else if (board[3] == c) {
		if (board[4] == c) {
			if (board[5] == c)
				win = 1;       /* 4 5 6 */
		}		
	}
	else if (board[6] == c) {
		if (board[7] == c) {
			if (board[8] == c)
				win = 1;       /* 7 8 9 */
		}
	}
	if (win > 0)
		return ((c == 'O') ? 1 : 2);
	else return win;
}




