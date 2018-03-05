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
#include <sys/msg.h>
#include <signal.h>
#include <errno.h>

#define SHM_SIZE 100
#define BILLION 1000000000L //taken from book

void print_usage();

struct mesg_buffer {
	long mesg_type;
	char mesg_text[100];
	int willTerminate; // 0 or 1
	long timeTerminatedOn;
	long clockIncrementedBy;
	
} message;


//Globals

//message queue key
key_t messageQueueParentKey;
key_t messageQueueChildKey;

//Shared Memory Keys
key_t keySecondsClock;
key_t keyNanoSecondsClock;

//file creation for log
char fileName[50] = "data.log";
FILE * fp;

pid_t childpid;


//taken from book
static int setperiodic(double sec);


int main (int argc, char *argv[]) {
	printf("at top of parent\n");

	//initalize variables
	
	messageQueueParentKey = 59568;
	messageQueueChildKey = 59569;
	keySecondsClock = 59566;
	keyNanoSecondsClock = 59567;	

	int maxProcess = 5; //default max processes if not specified 
	int processCount = 0; //current number of processes
	int processLimit = 100; //max number of processes allowed by assignment parameters
	int totalProcessesCreated; //keeps track of all processes created	

	int shmidSecondsClock;
	int shmidNanoSecondsClock;

	int messageBoxParentID;
	int messageBoxChildID;

//	int secondsClock;
//	int nanoSecondsCLock;
	double terminateTime = 20;

	
	//mssget creates a message queue and returns identifier
	//create mail boxes for parent and child processes

//	if (int msgParentID = msgget(messageQueueKey, IPC_CREAT | 0777);


														printf("after message queue creation\n");
//getopt
	char option;
	while ((option = getopt(argc, argv, "s:hl:t:")) != -1){
		switch (option){
			case 's' : maxProcess = atoi(optarg);
				break;
			case 'h': print_usage();
				break;
			case 'l': 
				strcpy(fileName, optarg);
				break;
			case 't':
				terminateTime = atof(optarg);
				break;
			default : print_usage(); 
				exit(EXIT_FAILURE);
		}
	}

//set timer 
	if (setperiodic(terminateTime) == -1){
		perror("oss: failed to set run timer");
		return 1;
		}

//Create Shared Memory
	if ((shmidSecondsClock = shmget(keySecondsClock, SHM_SIZE, IPC_CREAT | 0777)) == -1){
		perror("oss: could not create shared memory");
		return 1;
	}
	if ((shmidNanoSecondsClock = shmget(keyNanoSecondsClock, SHM_SIZE, IPC_CREAT | 0777)) == -1){
		perror("oss: could not create shared memory");
		return 1;
	}

//Attach to shared memory
	int * secondsClock = (int *)shmat(shmidSecondsClock, NULL, 0);
	int * nanoSecondsClock = (int *)shmat(shmidNanoSecondsClock, NULL, 0);

//start the clock at 0
	*secondsClock = 0;
	*nanoSecondsClock = 0;

//create mail box for parent
	if ((messageBoxParentID = msgget(messageQueueParentKey, IPC_CREAT | 0777)) == -1){
		perror("oss: Could not create parent mail box");
		return 1;
	}

	if ((messageBoxChildID = msgget(messageQueueChildKey, IPC_CREAT | 0777)) == -1){
		perror("oss: Could not create child mail box");
		return 1;
	}

	message.mesg_type = 1; //set message type

//send message to child box
	if(msgsnd(messageBoxChildID, &message, sizeof(message), 0) == -1){
		perror("oss: Failed to send message to child");
	}


	printf("before do loop in parent\n");


//open file for log writing
	fp = fopen(fileName, "a");

//MAIN PROCESS


	do {

		while (processCount < maxProcess){
	
			if ((childpid = fork()) == -1){
				perror("oss: failed to fork child");
			}

			if ((childpid == 0)){
				fprintf(fp, "Creating new child pid %d at my time %d.%d \n", getpid(), *secondsClock, *nanoSecondsClock);
//				printf("Forking child\n");
				execv("./user", NULL);
				return 1;
			}

			//increment process count
			processCount++;
			totalProcessesCreated++;
			if (totalProcessesCreated == processLimit){ //break the loop if number of processes exceed the limit
				break;

			}

			//CRITICAL SECTION
			//receive a message		
	
			msgrcv(messageBoxParentID, &message, sizeof(message), 1, 0);
			printf("In Parent - received message Clock incremented by  %ld\n", message.timeTerminatedOn);

			if (*secondsClock >=2){ //break the loop of seconds exceed 2
				break;
			}

			processCount--;
		}

	} while ( *secondsClock < 2);  //runs for 2 seconds.  secondsCLock incremented in user






shmdt(secondsClock);
shmdt(nanoSecondsClock);

shmctl(shmidSecondsClock, IPC_RMID, NULL);
shmctl(shmidNanoSecondsClock, IPC_RMID, NULL);

return 0;

}



//TAKEN FROM BOOK
static int setperiodic(double sec) {
   timer_t timerid;
   struct itimerspec value;
   if (timer_create(CLOCK_REALTIME, NULL, &timerid) == -1)
      return -1;
   value.it_interval.tv_sec = (long)sec;
   value.it_interval.tv_nsec = (sec - value.it_interval.tv_sec)*BILLION;
   if (value.it_interval.tv_nsec >= BILLION) {
      value.it_interval.tv_sec++;
      value.it_interval.tv_nsec -= BILLION;
   }
   value.it_value = value.it_interval;
   return timer_settime(timerid, 0, &value, NULL);
}


void print_usage(){
	printf("future help message\n");
}
