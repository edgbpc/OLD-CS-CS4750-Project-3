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
	int mesg_terminatedOnSeconds;
	int mesg_terminatedOnNanoSeconds;
	int mesg_childSecondsTime;
	int mesg_childNanoSecondsTime;
	int mesg_childPid; //to tell parent which child terminated
} message;


//Globals

//message queue key
key_t messageQueueParentKey;
key_t messageQueueChildKey;

//Shared Memory Keys
key_t keySimClock;

int messageBoxParentID;
int messageBoxChildID;

//file creation for log
char fileName[50] = "data.log";  //default if not selected by command line
FILE * fp;

int childpid; //pid id for child processes


//taken from book
static int setperiodic(double sec);


int main (int argc, char *argv[]) {
	printf("at top of parent\n");

	//initalize variables
	//keys
	messageQueueParentKey = 59568;
	messageQueueChildKey = 59569;
	keySimClock = 59566;

	int maxProcess = 5; //default max processes if not specified 
	int processCount = 0; //current number of processes
	int processLimit = 100; //max number of processes allowed by assignment parameters
	int totalProcessesCreated; //keeps track of all processes created	
	int *simClock; // simulated system clock  simClock [0] = seconds, simClock[1] = nanoseconds

	int shmidSimClock;

//	int shmidSecondsClock;
//	int shmidNanoSecondsClock;

	double terminateTime = 20;


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

//set timer. from book
	if (setperiodic(terminateTime) == -1){
		perror("oss: failed to set run timer");
		return 1;
		}
printf("after setperiodic\n");

//Create Shared Memory
	if ((shmidSimClock = shmget(keySimClock, SHM_SIZE, IPC_CREAT | 0666)) == -1){
		perror("oss: could not create shared memory");
		return 1;
	}
//	if ((shmidNanoSecondsClock = shmget(keyNanoSecondsClock, SHM_SIZE, IPC_CREAT | 0666)) == -1){
//		perror("oss: could not create shared memory");
//		return 1;
//	}
printf("after create shared memory\n");


//Attach to shared memory and initalize clock to 0
	simClock = shmat(shmidSimClock, NULL, 0);
	simClock[0] = 0; //nanoseconds
	simClock[1] = 0; //seconds


	fp = fopen(fileName, "a");
printf("after file open\n");

printf("after assigning clock values\n");


//create mail boxes
	if ((messageBoxParentID = msgget(messageQueueParentKey, IPC_CREAT | 0666)) == -1){
		perror("oss: Could not create parent mail box");
		return 1;
	}

	if ((messageBoxChildID = msgget(messageQueueChildKey, IPC_CREAT | 0666)) == -1){
		perror("oss: Could not create child mail box");
		return 1;
	}
printf("after creating mail boxes\n");
	message.mesg_type = 1; //set message type
printf("after assining 1 to message type\n");
printf("%d\n", message.mesg_type);
//send message to child box

printf("messageBoxParentID is %d\n", messageBoxParentID);
printf("messageBoxChildID is %d\n", messageBoxChildID);

	if(msgsnd(messageBoxChildID, &message, sizeof(message), 0) == -1){
		perror("oss: Failed to send message to child");
	}
printf("In parent: Address of message is %d\n", &message);
//printf("ChildboxId is %d\n", messageBoxChildID);
	printf("before do loop in parent\n");


//MAIN PROCESS
printf("OSS: before do loop\n");

while ( simClock[1] <= 2) {  //runs for 2 seconds.  secondsCLock incremented in user
printf("OSS: Inside Do loop\n");
		while (processCount < maxProcess){ //spawn children to max.  each child will be blocked until there is a message in the box for it
	
			if ((childpid = fork()) == -1){
				perror("oss: failed to fork child");
			}

			if ((childpid == 0)){
				fprintf(fp, "Creating new child pid %d at my time %d.%d \n", getpid(), simClock[1], simClock[0]);
				fflush(fp);
				printf("Forking child\n");
				execv("./user", NULL);
				return 1;
			}
			printf("after the fork\n");
			//increment process count
			processCount++;
			totalProcessesCreated++; 
			if (totalProcessesCreated == processLimit){ //break the loop if number of processes exceed the limit
				break;

			}

			//CRITICAL SECTION

			//receive a message		
			msgrcv(messageBoxParentID, &message, sizeof(message), 1, 0);
			printf("In Parent - received message terminate on seconds :  %d\n", message.mesg_terminatedOnSeconds);

			if (simClock[1] >=2){ //break the loop of seconds exceed 2
				break;
			}

			processCount--; //decrement process count when message received from child.  message receied indicates the child has terminated


			simClock[1] += 100;
		
			
			//send message to child box
			message.mesg_type = 1;	
			if (msgsnd(messageBoxChildID, &message, sizeof(message), 0) == -1){
				perror("oss: failed to send message to child box\n");
			}
		}

}


wait(NULL); // wait for all child processes to end

shmdt(simClock);

shmctl(shmidSimClock, IPC_RMID, NULL);
msgctl(messageBoxParentID, IPC_RMID, NULL);
msgctl(messageBoxChildID, IPC_RMID, NULL);



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
