FLAGS= -Ofast
LIBS=-lpthread -ldl
make: sqlite3.o
	g++ $(FLAGS) sqlite3.o dbsort.cpp -o dbsort $(LIBS)  

sqlite3.o:
	gcc -O3 sqlite3.c -c

demo0: make
	time ./dbsort /home/cop5570a/test0

demo1: make
	time ./dbsort /home/cop5570a/test1