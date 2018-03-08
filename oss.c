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
	long mesg_type; //always 1
	int mesg_terminatedOn[2]; //[0] for nanoseconds, [1] for seconds
	int mesg_ranFor;
	int mesg_childPid; //to tell parent which child terminated
} message;


//Globals

//message queue key
//key_t messageQueueParentKey;
key_t messageQueueKey;

//Shared Memory Keys
key_t keySimClock;

//int messageBoxParentID;
int messageBoxID;

//file creation for log
char fileName[50] = "data.log";  //default if not selected by command line
FILE * fp;

int childpid; //pid id for child processes


int shmidSimClock;

//taken from book
static int setperiodic(double sec);

void handle(int signo);

int main (int argc, char *argv[]) {
	printf("at top of parent\n");

	//initalize variables
	//keys
//	messageQueueParentKey = 59568;
	messageQueueKey = 59569;
	keySimClock = 59566;

	int maxProcess = 5; //default max processes if not specified 
	int processCount = 0; //current number of processes
	int processLimit = 100; //max number of processes allowed by assignment parameters
	int totalProcessesCreated = 0; //keeps track of all processes created	
	int *simClock; // simulated system clock  simClock [0] = seconds, simClock[1] = nanoseconds


//	int shmidSecondsClock;
//	int shmidNanoSecondsClock;

	double terminateTime = 20;


//	printf("after message queue creation\n");

	if(signal(SIGINT, handle) == SIG_ERR){
		perror("Signal Failed");
	}

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

//set timer. from book
	if (setperiodic(terminateTime) == -1){
		perror("oss: failed to set run timer");
		return 1;
		}
//printf("after setperiodic\n");

//Create Shared Memory
	if ((shmidSimClock = shmget(keySimClock, SHM_SIZE, IPC_CREAT | 0666)) == -1){
		perror("oss: could not create shared memory");
		return 1;
	}

//Attach to shared memory and initalize clock to 0
	simClock = shmat(shmidSimClock, NULL, 0);
	simClock[0] = 0; //nanoseconds
	simClock[1] = 0; //seconds


	fp = fopen(fileName, "w");
//printf("after file open\n");

//printf("after assigning clock values\n");


//create mail boxes
//	if ((messageBoxParentID = msgget(messageQueueParentKey, IPC_CREAT | 0666)) == -1){
//		perror("oss: Could not create parent mail box");
//		return 1;
//	}

	if ((messageBoxID = msgget(messageQueueKey, IPC_CREAT | 0666)) == -1){
		perror("oss: Could not create child mail box");
		return 1;
	}
	message.mesg_type = 1; //set message type

	if(msgsnd(messageBoxID, &message, sizeof(message), 0) == -1){
		perror("oss: Failed to send message to child");
	}


//MAIN PROCESS
	while ( simClock[1] < 2 && totalProcessesCreated < 100) {  //runs for 2 seconds.  simClock [1] incremented in user
		//create children
		while (processCount < maxProcess){ //spawn children to max.  each child will be blocked until there is a message in the box for it
	
			childpid = fork();
		
			if ((childpid == 0)){
					fprintf(fp, "Creating new child pid %d at my time %d.%d \n", getpid(), simClock[1], simClock[0]);
					fflush(fp);
					execl("./user", NULL);
					return 1;
				}
			//increment process count
			processCount++;
			totalProcessesCreated++;  //used to decided if overall too many processes have generated
			
			if (totalProcessesCreated >= processLimit){ //break inner loop
				break;
			}	
		}
		
		if (totalProcessesCreated >= processLimit){ // break outer loop
				break;
		}
			
			//wait for child process to send a message to it
		wait(NULL);	
		//CRITICAL SECTION		
		//receive a message		
		msgrcv(messageBoxID, &message, sizeof(message), 1, 0);
		if (simClock[1] >= 2){ // break outer loop if child processes increased simClock seconds to 2
			break;
		}
		processCount--; //receiving a message means a child terminated, reduce processCount.  will allow outer inner loop to continue and spawn new child to replace it		
		
		//configure output 
		//convert message.mesg_ranFor into format seconds.nanoseconds
		int convertedRanFor[2];
		convertedRanFor[1] = message.mesg_ranFor/1000000000; //assign fully divisible by nanoseconds to seconds clock
		convertedRanFor[0] = message.mesg_ranFor%1000000000;
	
		fprintf(fp, "Child pid is %d is terminating at my time %d.%d because it reached %d.%d which lived for %d.%d\n", message.mesg_childPid, simClock[1], simClock[0], message.mesg_terminatedOn[1], message.mesg_terminatedOn[0], convertedRanFor[1], convertedRanFor[0]);
		fflush(fp);

		//increment clock by 100 per assignment guidelines
		simClock[0] += 100;
		//convert nanoseconds to seconds	
		if (simClock[0] > 1000000000){ //if nanoSeconds > 1000000000
			simClock[1] += simClock[0]/1000000000; //add number of fully divisible by nanoseconds to seconds clock.  will truncate
			simClock[0] = simClock[0]%1000000000; //assign remainder of nanoseconds to nanosecond clock
		}

			
			//send message to child box to signal allow another child to be able to become unblocked
			message.mesg_type = 1;	
			if (msgsnd(messageBoxID, &message, sizeof(message), 0) == -1){
				perror("oss: failed to send message to child box\n");
			}
	}	


wait(NULL); // wait for all child processes to end

printf("simClock ended at %d.%d\n", simClock[1],simClock[0]);
printf("totalProcesses reached was %d\n", totalProcessesCreated);

//free  up memory queue and shared memory
shmdt(simClock);

shmctl(shmidSimClock, IPC_RMID, NULL);
//msgctl(messageBoxParentID, IPC_RMID, NULL);
msgctl(messageBoxID, IPC_RMID, NULL);



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

void handle(int signo){
	if (signo == SIGINT){
				
		shmctl(shmidSimClock, IPC_RMID, NULL);
		msgctl(messageBoxID, IPC_RMID, NULL);
		wait(NULL);
		exit(0);
	}
}
