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
	long mesg_type; //always 1
	int mesg_terminatedOnSeconds;
	int mesg_terminatedOnNanoSeconds;
	int mesg_childSecondsTime;
	int mesg_childNanoSecondsTime;
	int mesg_childPid; //to tell parent which child terminated
} message;

int main (int argc, char *argv[]) {

	//message queue key
	key_t messageQueueParentKey = 59568;
	key_t messageQueueChildKey = 59569;

	//Shared Memory Keys
	key_t keySecondsClock = 59566;
	key_t keyNanoSecondsClock = 59567;
	
	int messageBoxParentID;
	int messageBoxChildID;
	int shmidSecondsClock;
	int shmidNanoSecondsClock;

	int totalTimeRunning = 0; //time current process has been running
	int workTime = 15000; //amount of workTime for 1 run
	float allotatedChildRunTime; //how long the child process is allowed to run.  determined randomly in child

	int childProcessTimeSeconds; //seconds clock for child process
	int childProcessTimeNanoSeconds; //nanoseconds clock for child process

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
	//load as start times for child process for seconds and nanoseconds
	childProcessTimeSeconds = *secondsClock;
	childProcessTimeNanoSeconds = *nanoSecondsClock;

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
	allotatedChildRunTime = (rand() % 1000000) + 1; //random run time between .001 and .01 seconds

	//CRITICAL SECTION

	do{
		msgrcv(messageBoxChildID, &message, sizeof(message), 1, 0); //retrieve message from max box.  child is blocked unless there is a message to take from box

		//maybe not needed
		message.mesg_type = 1;

		//check if current run exceeds total allotated time
		if ((totalTimeRunning + workTime) > allotatedChildRunTime){
			workTime = allotatedChildRunTime - totalTimeRunning; //reduce work to the amount of time remaining
			}

		//increment child clocks
		childProcessTimeNanoSeconds += workTime; //shared memory time
		totalTimeRunning += childProcessTimeNanoSeconds; //accumlating total amount of nanoseconds child process as ran

		//increment the system clocks in shared memory
		*nanoSecondsClock += childProcessTimeNanoSeconds;
		*secondsClock += childProcessTimeSeconds;

		//convert nanoseconds to seconds
		if (childProcessTimeNanoSeconds > 1000000000){ //if nanoSeconds > 1000000000
			childProcessTimeSeconds += childProcessTimeNanoSeconds/1000000000; //add number of fully divisible by nanoseconds to seconds clock.  will truncate
			childProcessTimeNanoSeconds = childProcessTimeNanoSeconds%1000000000; //assign remainder of nanoseconds to nanosecond clock
		}

		

		//if  time exceeds alloated time for entire process, terminate and send message to parent
		if (*secondsClock >= 2){
			message.mesg_type = 1;
			message.mesg_terminatedOnSeconds = childProcessTimeSeconds;
			message.mesg_terminatedOnNanoSeconds = childProcessTimeNanoSeconds; 
			message.mesg_childSecondsTime = childProcessTimeSeconds;
			message.mesg_childNanoSecondsTime = childProcessTimeNanoSeconds;
			message.mesg_childPid = getpid();
	
			if(msgsnd(messageBoxParentID, &message, sizeof(message), 0) == -1){
				perror("user: could not send message to parent box");
			}
		}

		//if duration has not passed, cede critical section to another child process
		if(totalTimeRunning < allotatedChildRunTime){
			message.mesg_type = 1;
			message.mesg_terminatedOnSeconds = childProcessTimeSeconds;
			message.mesg_terminatedOnNanoSeconds = childProcessTimeNanoSeconds; 
			message.mesg_childSecondsTime = childProcessTimeSeconds;
			message.mesg_childNanoSecondsTime = childProcessTimeNanoSeconds;
			message.mesg_childPid = getpid();
	
			if(msgsnd(messageBoxChildID, &message, sizeof(message), 0) == -1){
				perror("user: could not send message to child box");

			}
		}

		
		//prepare message 
		message.mesg_type = 1;
		message.mesg_terminatedOnSeconds = childProcessTimeSeconds;
		message.mesg_terminatedOnNanoSeconds = childProcessTimeNanoSeconds; 
		message.mesg_childSecondsTime = childProcessTimeSeconds;
		message.mesg_childNanoSecondsTime = childProcessTimeNanoSeconds;
		message.mesg_childPid = getpid();
		

		
		//send message to parent
		if (msgsnd(messageBoxParentID, &message, sizeof(message), 0) == -1){
			perror("user: failed to send to parent box");
		}	



	} while (totalTimeRunning < allotatedChildRunTime);



return 0;

}



