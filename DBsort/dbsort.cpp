#include <iostream>
#include <cstdlib>
#include <queue>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sstream>
#include <fstream>

#include <omp.h> //Parallel timer.
#include <pthread.h>

#include "sqlite3.h"

using namespace std;

const int NTHREADS = 4;
const int MAXQSIZE = 100000; //Prevent overflowing.
int parentMachine = 2;

long long _l;
int size = sizeof(_l);

queue<long> inq [NTHREADS]; 		//Each stores 1/NTHREADS total ints, paritioned by range.
queue<string> outq [NTHREADS]; 		//Each stores 1/NTHREADS total ints, paritioned by range.

pthread_mutex_t qlock[NTHREADS];

bool readyToPrint[NTHREADS];
bool doneReading = false;
bool DEBUG = false;
bool done = false;
bool donesending[NTHREADS];

long long MIN, MAX;


static int callback(void *data, int argc, char **argv, char **azColName){   
   //Get id from data
   int id = *((int *) argv);
   while (outq[id].size() > MAXQSIZE){
   	usleep(250);
   }
   
   pthread_mutex_lock(&qlock[id]);
   outq[id].push(argv[0]);
   pthread_mutex_unlock(&qlock[id]);
   
   return 0;
}


//Or "Read()". Honestly we're basically doing producer consumer problem.
void Producer(string path){
	int fd = open(path.c_str(), O_RDONLY);
	long long e;
	while (fd){ //Is this going to be a valid to ensure file okay? Check.
		if (read(fd, (void*)&e, sizeof(e)) == sizeof(e)){
			if (NTHREADS > 1){
				for (int i = 0; i < NTHREADS; i++){
					if (e <= ((MAX - MIN)/i)){
						if (DEBUG) printf("Inserted %lld to queue %d\n", e, i);

						while (inq[i].size() > MAXQSIZE) usleep(250);

						pthread_mutex_lock(&qlock[i]);
						inq[i].push(e);
						pthread_mutex_unlock(&qlock[i]);

						//Element is to be inserted. Onto next.
						break;
					}
				}
			}
			else{
				while (inq[0].size() > MAXQSIZE) usleep(250);

				pthread_mutex_lock(&qlock[0]);
				inq[0].push(e);
				pthread_mutex_unlock(&qlock[0]);
			}
		}
		else{
			printf("Err: Read unexpexted number of bytes.\n");
		}
	
	}

	doneReading = true;
}


//Pthread function which pushes data into SQL files.
void* Consumer(void* arg){
	int id = *((int *) arg);
	long l;
	char *zErrMsg = 0;
	int rc;
	int machineID = id +  3; //3, 4, 5, 6. Reader is 2 or 7.
	string cmd, query;
	stringstream ss;

	pthread_detach(pthread_self());
	string dbname;
	ss << "db" << id << ".db";
	dbname = ss.str();
	ss.str(""); ss.clear();

	readyToPrint[id] = false;
	
	ss << "ssh c5570-" << machineID;
	cmd = ss.str();
	ss.str(""); ss.clear();
	cout << "Thread " << id << " now executing: " << cmd << endl;
	system(cmd.c_str());

	//Make db file
	sqlite3 * db;
	sqlite3_open(dbname.c_str(), &db);

	//One table in the whole database. Hah. It'll work though!
	query = "CREATE TABLE IF NOT EXISTS numbers (value BIGINT, t2 timestamp(9));";
	sqlite3_stmt *createStmt;
	sqlite3_prepare_v2(db, query.c_str(), query.size(), &createStmt, NULL);
	if (sqlite3_step(createStmt) != SQLITE_DONE) cout << "ERROR: Could not create table!" << endl;

	while(!doneReading && !inq[id].empty()){
		if (inq[id].empty()) usleep(250);
		while (!inq[id].empty()){
			pthread_mutex_lock(&qlock[id]);
			l = inq[id].front(); //Get next element.
			inq[id].pop(); //Pop it. We will insert this.
			pthread_mutex_unlock(&qlock[id]);

		   ss << "INSERT INTO numbers VALUES (" << l << ", CURRENT_TIMESTAMP);"; 
		   query = ss.str();
		   ss.str(""); ss.clear();

		   sqlite3_stmt *insertStmt;
		   sqlite3_prepare(db, query.c_str(), query.size(), &insertStmt, NULL);
		   if (sqlite3_step(insertStmt) != SQLITE_DONE) cout << "ERROR: Could not enter data into table!" << endl;

		}
	}

	donesending[id] = false;
	readyToPrint[id] = true;
	query = "SELECT ALL value FROM numbers ORDER BY value ASC;";
	int *arg2 = (int*)malloc(sizeof(*arg2));
    *arg2 = id;
	rc = sqlite3_exec(db, query.c_str(), callback, arg2, &zErrMsg);

	donesending[id] = true;
	while(!done) usleep(2500); //Sleep til main is done.
	//Clear DB
	cmd = "rm *.db";
	system(cmd.c_str());
	exit(0);
} 

int main(int narg, char ** argc){	
	int fd;
	//FILE * fd;
	long long e;
	long long counter = 0;
	string path, file, cmd, ofname;
	ofstream ofile;
	bool firstIteration = true;
	int code;

	double t0, t1, t2, t3, t4, t5, t6;

	if (narg != 2){
		cout << "Error: Usage: ./dbsort <file to sort> " << endl;
		exit(1);
	}

	path = argc[1];

	t0 = omp_get_wtime();

	if (NTHREADS > 1){
		cout << "Preliminary scan to determine MIN and MAX values...\n";
		fd = open(path.c_str(), O_RDONLY);

		while (fd){
			//Consider skipping ~1000 numbers each iteration. Should result in same ballpark min/max
			if (read(fd, (void*)&e, sizeof(e)) == sizeof(e)){				
				
				counter++;
				if (counter%10000 == 0){
					cout << "Read " << counter << " numbers! MIN = " << MIN << ", MAX = " << MAX << "\n";
				}//*/

				{
					if (firstIteration){
						MIN = MAX = e;
						firstIteration = false;
					}
					else if (e < MIN) MIN = e;
					else if (e > MAX) MAX = e;

					//Skip 10,000 iterations for speed
					lseek(fd, 10000*sizeof(e), SEEK_CUR);
				}
			}
		}

		close(fd);
		t2 = omp_get_wtime();
		cout << "Done. MIN = " << MIN << ", MAX = " << MAX << ". Scan time = " << t1 - t0 << " s.\n"; 
	}
	else t2 = omp_get_wtime();

	
	cout << "Beginning sort routine: Creating processes and dumping data into databases.\n";
	pthread_t pthreads[NTHREADS];
	for(int i = 0; i < NTHREADS; i++){
        int *arg2 = (int*)malloc(sizeof(*arg2));
        *arg2 = i;
        if ((code = pthread_create(&pthreads[i], NULL, Consumer, arg2)) != 0){
            printf("\tError creating thread #%d (Error code = %d).\n", i, code);
            printf("\tPrematurely returning.\n");
            return 0;
        }
    }

	Producer(path);

	t3 = omp_get_wtime();
	cout << "Done dumping data. (Insert Time = " << t3 - t2 << ", Total time = " 
		<< t3 - t0 << ")\nMain is now reading back data and printing it to file.";

	ofname = "outfile.txt";
	ofile.open(ofname.c_str());
	int tid = 0;
	long long itr = 0;
	cout << "Sorted data:\n";
	while (tid != NTHREADS){
		while (!donesending[tid] && !outq[tid].empty()){
			if (outq[tid].empty()) usleep(250);
			while(!outq[tid].empty()){
				if (itr%1000 == 0){
					string s;
					pthread_mutex_lock(&qlock[tid]);
					s = outq[tid].front();
					outq[tid].pop();
					pthread_mutex_unlock(&qlock[tid]); 

					cout << s << endl;
					ofile << s << endl;

				}
				else{
					pthread_mutex_lock(&qlock[tid]);
					outq[tid].pop();
					pthread_mutex_unlock(&qlock[tid]); 
				}


				itr++;
			}
		}
		tid++;
	}
	t4 = omp_get_wtime();
	cout << "Done. (Print Time = " << t4 - t3 << ", Total time = " 
		<< t4 - t0 << ")\nGoodbye World.";

	done = true;
	return 0;

}