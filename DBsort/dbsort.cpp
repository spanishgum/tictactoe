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

ofstream oftmp[NTHREADS];
ifstream iftmp[NTHREADS];

pthread_mutex_t qlock[NTHREADS];

pthread_mutex_t mymutex[NTHREADS];

bool doneReading = false;
bool DEBUG = false;


long long MIN, MAX;


static int callback(void *data, int argc, char **argv, char **azColName){   
	//Get id from data

	int id = *((int *) data);
	
	oftmp[id] << argv[0] << "\n";
	//cout << "My id = " << id << " Element = " << argv[0] << endl;

	return 0;
}


//Or "Read()". Honestly we're basically doing producer consumer problem.
void Producer(string path){
	cout << "Producer launched!" << endl;
	int fd = open(path.c_str(), O_RDONLY);
	long long e;
	long nNums = 0;
	while (fd){ //Is this going to be a valid to ensure file okay? Check.
		int rd = read(fd, (void*)&e, sizeof(e));
		if (rd == sizeof(e)){
			for (int i = 0; i < NTHREADS; i++){
				if ((i == NTHREADS - 1) || (e <= ((i + 1)*(MAX/(NTHREADS))) + MIN)){
					nNums++;
					/*if (nNums%1000 == 0){
						cout << nNums << ": " << e << endl;
					}//*/

					if (DEBUG) 
						printf("Inserted %lld to queue %d\n", e, i);

					while (inq[i].size() > MAXQSIZE) usleep(250);

					pthread_mutex_lock(&qlock[i]);
					inq[i].push(e);
					pthread_mutex_unlock(&qlock[i]);

					//Element is to be inserted. Onto next.
					break;
				}
			}
		}
		else if (rd == -1){
			//EoF
			break;
		}
		else{
			printf("Err: Read unexpexted number of bytes.\n");
		}

		//if (nNums > 10000) break;
	
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
	string cmd, query, fname;
	stringstream ss;
	pthread_mutex_lock(&mymutex[id]);
	ss << "file" << id << ".tmp";
	fname = ss.str();
	ss.str(""); ss.clear();

	oftmp[id].open(fname.c_str());

	long nInserted = 0;
	cout << "Thread " << id  << " launched!" << endl;
	
	/*ss << "ssh -q c5570-" << machineID;
	cmd = ss.str();
	ss.str(""); ss.clear();
	cout << "Thread " << id << " now executing: " << cmd << endl;
	system(cmd.c_str());//*/


	//pthread_detach(pthread_self());
	string dbname;
	ss << "db" << id << ".db";
	dbname = ss.str();
	ss.str(""); ss.clear();

	//Make db file
	sqlite3 * db;
	sqlite3_open(dbname.c_str(), &db);
	long batchCount = 0;
	bool hasbatch = false;

	//One table in the whole database. Hah. It'll work though!
	query = "CREATE TABLE IF NOT EXISTS numbers (value BIGINT);";//, t2 timestamp(9));";
	sqlite3_stmt *createStmt;
	sqlite3_prepare_v2(db, query.c_str(), query.size(), &createStmt, NULL);
	if (sqlite3_step(createStmt) != SQLITE_DONE) cout << "ERROR: Could not create table!" << endl;

	ss.str(""); ss.clear();
	sqlite3_exec(db, "BEGIN TRANSACTION;", NULL, NULL, NULL);
	//ss << "INSERT INTO numbers VALUES (";
	hasbatch = false;
	while(!(doneReading && inq[id].empty())){
		if (inq[id].empty()) usleep(250);
		//if (inq[id].size() < 10000) usleep(25000);
		while (!inq[id].empty()){

			
			pthread_mutex_lock(&qlock[id]);
			l = inq[id].front(); //Get next element.
			inq[id].pop(); //Pop it. We will insert this.
			pthread_mutex_unlock(&qlock[id]);

			/*if (hasbatch){
				ss << ", (" << l << ")";
			}
			else{
				ss << "(" << l << ")";

			}*/
			
		   	ss << "INSERT INTO numbers VALUES (" << l << ");"; // ", CURRENT_TIMESTAMP);\n"; 
			query = ss.str();
			ss.str(""); ss.clear();

			hasbatch = true;
			batchCount++;

			//sqlite3_stmt *insertStmt;
			sqlite3_exec(db, query.c_str(), NULL, NULL, NULL);
			//sqlite3_prepare(db, query.c_str(), query.size(), &insertStmt, NULL);
			//if (sqlite3_step(insertStmt) != SQLITE_DONE) cout << "ERROR: Could not enter data into table!" << endl;
			
			
		   //query = ss.str();
		   //ss.str(""); ss.clear();

		   	if (batchCount % 1000000 == 0){
		   		sqlite3_exec(db, "END TRANSACTION;", NULL, NULL, NULL);
		   		//ss << ");";
				nInserted += 1000000;
				batchCount = 0;
				hasbatch = false;
				//ss << "INSERT INTO numbers VALUES (";
				sqlite3_exec(db, "BEGIN TRANSACTION;", NULL, NULL, NULL);

				if (nInserted%1000000 == 0)
					cout << "Thread #" << id << " has inserted " << nInserted/1000000 << " million values." << endl;
		   
			}

		   //cout << "SQL: " << query << endl;
		   
		}
	}

	if (hasbatch){
   		//ss << ");";
   		sqlite3_exec(db, "END TRANSACTION;", NULL, NULL, NULL);
		
	}

	cout << "Thread #" << id << " executing SELECT statement." << endl;
	
	query = "SELECT ALL value FROM numbers ORDER BY value ASC;";
	int *arg2 = (int*)malloc(sizeof(*arg2));
    *arg2 = id;
	rc = sqlite3_exec(db, query.c_str(), callback, arg2, &zErrMsg);

	if (rc != SQLITE_OK){
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
	    sqlite3_free(zErrMsg);
	}
	cout << "Thread #" << id << " done." << endl;

	sqlite3_close(db);

	oftmp[id].close();
	pthread_mutex_unlock(&mymutex[id]);
	cout << "Thread #" << id << " signaled done." << endl;

	return NULL;
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
	cmd = "rm *.db";
	system(cmd.c_str());
	cmd = "rm *.db-journal";
	system(cmd.c_str());

	//double t0, t1, t2, t3, t4, t5, t6;

	if (narg != 2){
		cout << "Error: Usage: ./dbsort <file to sort> " << endl;
		exit(1);
	}

	/*for (int i = 0; i < NTHREADS; i++){
		pthread_cond_init(&readyToPrint[i], NULL);
	}*/

	path = argc[1];

	//t0 = omp_get_wtime();

	if (NTHREADS > 1){
		cout << "Preliminary scan to determine MIN and MAX values...\n";
		fd = open(path.c_str(), O_RDONLY);

		while (fd){
			//Consider skipping ~1000 numbers each iteration. Should result in same ballpark min/max
			int rd = read(fd, (void*)&e, sizeof(e));
			if (rd == sizeof(e)){				
				
				counter++;
				if (counter%1000 == 0){
					cout << "Read " << counter << " numbers!\tMIN = " << MIN << ", MAX = " << MAX << "\n";
				}//*/

				{
					if (firstIteration){
						MIN = MAX = e;
						firstIteration = false;
					}
					else if (e < MIN) MIN = e;
					else if (e > MAX) MAX = e;

					if ((float)((float)MIN/(float)MAX) < 0.005)
						break; //Small enough.

					//Skip a shitload of numbers for speed
					lseek(fd, 100000*sizeof(e), SEEK_CUR);
				}
			}
			else break;
		}

		close(fd);
		//t2 = omp_get_wtime();
		cout << "Done. MIN = " << MIN << ", MAX = " << MAX << endl; // ". Scan time = " << t1 - t0 << " s.\n"; 
	}
	//else t2 = omp_get_wtime();

	
	cout << "Beginning sort routine: Creating processes and dumping data into databases.\n";
	pthread_t pthreads[NTHREADS];
	for(int i = 0; i < NTHREADS; i++){
		cout << "Launching thread " << i << endl;
        int *arg2 = (int*)malloc(sizeof(*arg2));
        *arg2 = i;
        if ((code = pthread_create(&pthreads[i], NULL, Consumer, arg2)) != 0){
            printf("\tError creating thread #%d (Error code = %d).\n", i, code);
            printf("\tPrematurely returning.\n");
            return 0;
        }
    }

	Producer(path);

	//t3 = omp_get_wtime();
	cout << "Done dumping data. \nMain is now waiting, then reading back data and printing it to file." << endl;
	//(Insert Time = " << t3 - t2 << ", Total time = " << t3 - t0 << ")\nMain is now reading back data and printing it to file.";

	ofname = "outfile.txt";
	ofile.open(ofname.c_str());
	int tid = 0;
	long long itr = 0;
	long long ele;
	stringstream ss;

	for (int i = 0; i < NTHREADS; i++){
		cout << "Waiting on thread #" << i << endl;
		pthread_mutex_lock(&mymutex[i]);
		ss << "file" << i << ".tmp";
		iftmp[i].open(ss.str().c_str());
		ss.str(""); ss.clear();

		iftmp[i] >> ele;
		while (iftmp[i]){
			if (itr%10 == 0){
				ofile << ele << endl;
				//cout << ele << endl;
			}

			iftmp[i] >> ele;
			itr++;
		}
		iftmp[i].close();
	}


	
	//t4 = omp_get_wtime();
	cout << "Done. \nGoodbye World.\n";
	cmd = "rm *.db*";
	system(cmd.c_str());
	cmd = "rm *.tmp";
	system(cmd.c_str());

	return 0;
}