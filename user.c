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


struct mesg_buffer {
	long mesg_type;
	char mesg_text[100];
	int willTerminate; // 0 or 1
	long timeTerminatedOn;
	long clockIncrementedBy;
	
} message;

int main (int argc, char *argv[]) {

	//message queue key
	key_t messageQueueParentKey = 59568;
	key_t messageQueueChildKey = 59569;

	int messageBoxParentID;
	int messageBoxChildID;

	//Shared Memory Keys
	key_t keySecondsClock = 59566;
	key_t keyNanoSecondsClock = 59567;

	int shmidSecondsClock;
	int shmidNanoSecondsClock;

	int runTime;
	int currentTime = 0; //time current process has been running
	int workTime = 15000; //amount of workTime for 1 run

	int startTimeSeconds;
	int startTimeNanoSeconds;

//TESTING ONLY
//char sampleMessage[20] = "A test message";
//strcpy(message.mesg_text, sampleMessage);
//msgsnd(msgid, &message, sizeof(message), 0);
//
//printf("Data sent from child was %s\n", message.mesg_text);
//END TESTING


	//Get Shared Memory
	shmidSecondsClock = shmget(keySecondsClock, SHM_SIZE, 0777 );
	shmidNanoSecondsClock = shmget(keyNanoSecondsClock, SHM_SIZE, 0777);

	//Attach to shared memory
	int * secondsClock= (int *)(shmat(shmidSecondsClock, 0, 0));
	int * nanoSecondsClock = (int *)(shmat(shmidNanoSecondsClock, 0, 0));


	//get current times from shared memory clock
	startTimeSeconds = *secondsClock;
	startTimeNanoSeconds = *nanoSecondsClock;

	//message queue
	if ((messageBoxParentID = msgget(messageQueueParentKey, 0777)) == -1){
		perror("user: failed to acceess parent message box");
		return 1;
		}

	if ((messageBoxChildID = msgget(messageQueueChildKey, 0777)) == -1){
		perror("user: failed to access child messsage box");
		return 1;
	}


	//Seed random number generator
	srand(time(NULL));

	//set child run time
	runTime = (rand() % 1000000) + 1; //random run time between 1 and 1M

	//CRITICAL SECTION

	do{
		msgrcv(messageBoxChildID, &message, sizeof(message), 1, 0);

		//maybe not needed
		message.mesg_type = 1;

		//check if current run exceeds total allotated time
		if ((currentTime + workTime) > runTime){
			workTime = runTime - currentTime; //reduce work to the amount of time remaining
			}

		//increment clock
		*nanoSecondsClock += workTime; //shared memory time
		currentTime += workTime; //time in tihe process

		//load data into message to send to parent
	
		//send message to parent
		if (msgsnd(messageBoxParentID, &message, sizeof(message), 0) == -1){
			perror("user: failed to send to parent box");
		}	

	} while (currentTime < runTime);

return 0;

}


