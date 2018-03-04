
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <sys/time.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <time.h>


#define SHM_SIZE 100

void print_usage();



int main (int argc, char *argv[]) {

//Shared Memory Keys
key_t keySecondsClock = 59566;
key_t keyNanoSecondsClock = 59567;

//Create Shared Memory
int shmidSecondsClock = shmget(keySecondsClock, SHM_SIZE, IPC_CREAT | 0777);
int shmidNanoSecondsClock = shmget(keyNanoSecondsClock, SHM_SIZE, IPC_CREAT | 0777);

//Attach to shared memory
int * secondsClock = (int *)shmat(shmidSecondsClock, NULL, 0);
int * nanoSecondsClock = (int *)shmat(shmidNanoSecondsClock, NULL, 0);


*secondsClock = 5;
*nanoSecondsClock = 10000;

int maxProcess = 5;
int processCount = 0;

FILE * fp;

int terminateTime;

pid_t childpid = 0;
//pid_t endID;

//char fileName[50];
//fp = fopen(fileName, "w");

char option;


while ((option = getopt(argc, argv, "s:hl:t:")) != -1){
	switch (option){
		case 's' : maxProcess = atoi(optarg);
		break;

		case 'h': print_usage();
		break;

		case 'l': 
//			strcpy(fileName, optarg);
		break;

		case 't':
			terminateTime = atoi(optarg);
		break;

		default : print_usage(); 
			exit(EXIT_FAILURE);
	}
}

printf("in Parent - clock is %d", *secondsClock);


do {

	if ((childpid = fork()) == -1){
		perror(("%s: Error:", argv[0]));
	}

	processCount++;
//	printf("processCount is %d\n", processCount);

	if ((childpid == 0)){
		execv("./user", NULL);
	}

	if (processCount == maxProcess){
		exit(0);
	}	

} while (processCount !=  maxProcess);

return 0;

}


void print_usage(){
	printf("future help message\n");
}
