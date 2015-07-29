FLAGS=-fopenmp -Ofast
LIBS=-lpthread -ldl
make: sqlite3.o
	g++ $(FLAGS) sqlite3.o dbsort.cpp -o dbsort $(LIBS)  

sqlite3.o:
	gcc $(FLAGS) sqlite3.c -c