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
	int mesg_terminatedOn[2]; //[0] for nanoseconds, [1] for seconds
	float mesg_ranFor;
	int mesg_childPid; //to tell parent which child terminated
} message;


int main (int argc, char *argv[]) {

	//message queue key
//	key_t messageQueueParentKey = 59568;
	key_t messageQueueKey = 59569;

	//Shared Memory Keys
	key_t keySimClock = 59566;

//	int messageBoxParentID;
	int messageBoxID;
	int shmidSimClock;

	int totalTimeRunning = 0; //time current process has been running
	int workTime = 15000; //amount of workTime for 1 run
	float allotatedChildRunTime; //how long the child process is allowed to run.  determined randomly in child


	int childProcessStartTime[2];

//TESTING ONLY
//char sampleMessage[20] = "A test message";
//strcpy(message.mesg_text, sampleMessage);
//msgsnd(msgid, &message, sizeof(message), 0);
//
//printf("Data sent from child was %s\n", message.mesg_text);
//END TESTING


	//Get Shared Memory
	shmidSimClock = shmget(keySimClock, SHM_SIZE, 0666 );

	//Attach to shared memory and get simClock time
	int * simClock= (int *)(shmat(shmidSimClock, 0, 0));
	childProcessStartTime[0] = simClock[0];
	childProcessStartTime[1] = simClock[1];


	//get current times from shared memory clock
	//load as start times for child process for seconds and nanoseconds

	//message queue
	if ((messageBoxID = msgget(messageQueueKey, 0666)) == -1){
		perror("user: failed to acceess parent message box");
		return 1;
		}

//	if ((messageBoxChildID = msgget(messageQueueChildKey, 0666)) == -1){
//		perror("user: failed to access child messsage box");
//		return 1;
//	}

	//Seed random number generator
	srand(time(NULL));

	//set child run time
	allotatedChildRunTime = (rand() % 1000000) + 1; //random run time between .001 and .01 seconds
printf("Child %d alloatedRunTime is %f\n", getpid(), allotatedChildRunTime);
	//CRITICAL SECTION


	while (totalTimeRunning < allotatedChildRunTime){		

printf("In Child - Sim Clock is %d.%d\n", simClock[1], simClock[0]);
								//printf("in Child - before message received\n");	
								//printf("address of message is %d\n", &message);
		msgrcv(messageBoxID, &message, sizeof(message), 1, 0); //retrieve message from max box.  child is blocked unless there is a message to take from box
								//printf("In child - after receive message\n");
		
		
		message.mesg_type = 1;
		//maybe not needed

		//check if current run exceeds total allotated time
		if ((totalTimeRunning + workTime) > allotatedChildRunTime){
			workTime = allotatedChildRunTime - totalTimeRunning; //reduce work to the amount of time remaining
			}

								//printf("before incrementing clocks\n");
		//convert nanoseconds to seconds
		if (simClock[0] > 1000000000){ //if nanoSeconds > 1000000000
			simClock[0] += simClock[0]/1000000000; //add number of fully divisible by nanoseconds to seconds clock.  will truncate
			simClock[1] = simClock[0]%1000000000; //assign remainder of nanoseconds to nanosecond clock
		}
		
		//increment the system clocks in shared memory
		simClock[0] += workTime;
		totalTimeRunning += workTime;

								//printf("after time conversion\n");		

		//if  time exceeds alloated time for entire process, terminate and send message to parent
		if (simClock[1] >= 2){
printf("simClock has passed 2 seconds\n");
			message.mesg_type; //always 1
			message.mesg_terminatedOn[0] = simClock[0] ; //[0] for nanoseconds, [1] for seconds
			message.mesg_terminatedOn[1] = simClock[1] ;
			message.mesg_ranFor = totalTimeRunning;
			message.mesg_childPid = getpid(); //to tell parent which child terminated
			if(msgsnd(messageBoxID, &message, sizeof(message), 0) == -1){
				perror("user: could not send message to parent box");
			
			}
printf("before exit command\n");
			printf("not been 2 seconds yet\n");
		}

		//if duration has not passed, cede critical section to another child process
		if(totalTimeRunning < allotatedChildRunTime){
								//printf("total time running is %d and exceeded allotatedchildrun time of %f\n", totalTimeRunning, allotatedChildRunTime);
								//printf("inside totaltimeRunning < allotatedChildRunTime\n");

								//printf("messageBoxChildID is %d\n", messageBoxChildID);
			if(msgsnd(messageBoxID, &message, sizeof(message), 0) == -1){
				perror("user: could not send message to child box");
			}
								//printf("before message prep\n");
		
		}
	}	
//when while loop ends, prepare message for parent
	
	message.mesg_type; //always 1
	message.mesg_terminatedOn[0] = simClock[0] ; //[0] for nanoseconds, [1] for seconds
	message.mesg_terminatedOn[1] = simClock[1] ;
	message.mesg_ranFor = totalTimeRunning;
	message.mesg_childPid = getpid(); //to tell parent which child terminated

		
		//send message to parent
	if (msgsnd(messageBoxID, &message, sizeof(message), 0) == -1){
		perror("user: failed to send to parent box");
	}	


		



return 0;

}


	

