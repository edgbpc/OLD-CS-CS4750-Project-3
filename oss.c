
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

#define SHM_SIZE 100

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
char fileName[50];
FILE * fp;

pid_t childpid;

int main (int argc, char *argv[]) {
	printf("at top of parent\n");

	//initalize variables
	
	messageQueueParentKey = 59568;
	mesageQueueChildKey = 59569;
	keySecondsClock = 59566;
	keyNanoSecondsCLock = 59567;	

	int maxProcess = 5; //default max processes if not specified 
	int processCount = 0; //current number of processes

	int secondsClock;
	int nanoSecondsCLock;

	
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
//				strcpy(fileName, optarg);
	//			fp = fopen(fileName, "w");
				break;
			case 't':
				terminateTime = atoi(optarg);
				break;
			default : print_usage(); 
				exit(EXIT_FAILURE);
		}
	}


//Create Shared Memory
	if ((int shmidSecondsClock = shmget(keySecondsClock, SHM_SIZE, IPC_CREAT | 0777)) == -1){
		perror("oss: could not create shared memory");
		return 1;
	}
	if ((int shmidNanoSecondsClock = shmget(keyNanoSecondsClock, SHM_SIZE, IPC_CREAT | 0777)) == -1){
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
	if (( int messageBoxParentID = msgget(messageQueueParentKey, IPC_CREAT | 0777)) == -1){
		perror("oss: Could not create parent mail box");
		return 1;
	}

	if (( int messageBoxChildID = msgget(messageQueueChildKey, IPC_CREAT | 0777)) == -1){
		perror("oss: Could not create child mail box");
		return 1;
	}

	message.mesg_type = 1; //set message type

//send message to child box
	if(msgsnd(messageBoxChildID, &message, sizeof(message), 0) == -1){
		perror("oss: Failed to send message to child");
	}

//Critical section

printf("before do loop in parent\n");

//do {

	if (processCount != maxProcess){

		if ((childpid = fork()) == -1){
			perror(("%s: Error:", argv[0]));
		}

		processCount++;
//		printf("processCount is %d\n", processCount);

		if ((childpid == 0)){
			printf("Forking child\n");
			execv("./user", NULL);
		}

	}
			
			sleep(10);
			msgrcv(msgid, &message, sizeof(message), 1, 0);
			printf("In Parent - received message Clock incremented by  %ld\n", message.timeTerminatedOn);
//test receive message
//printf("In Parent - received message %s\n", message.mesg_text);

//msgrc to receive message
//msgrcv(msgid, &message, sizeof(message), 1, 0);
//} while (1);



shmdt(secondsClock);
shmdt(nanoSecondsClock);

shmctl(shmidSecondsClock, IPC_RMID, NULL);
shmctl(shmidNanoSecondsClock, IPC_RMID, NULL);

return 0;

}


void print_usage(){
	printf("future help message\n");
}
