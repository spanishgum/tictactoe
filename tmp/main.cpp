#include <string>
#include <iostream>

#include "../user.h"
#include "../game.h"
#include "../mail.h"
#include "../load.h"
#include "../save.h"

int main() {

	user A("adam", "pass"), B("steven", "pass");
	game G(A.name, B.name, 0, 100);
	game H("a", "b", 1, 200);

	cout << G.print_board();
	cout << "\n" << H.id << "";

	return 0;
}
