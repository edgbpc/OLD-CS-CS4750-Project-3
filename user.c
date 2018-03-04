#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <time.h>
#include <string.h>
#include <signal.h>



#define SHM_SIZE 100

int main (int argc, char *argv[]) {

//Shared Memory Keys
key_t keySecondsClock = 59566;
key_t keyNanoSecondsClock = 59567;

//Create Shared Memory

int shmidSecondsClock = shmget(keySecondsClock, SHM_SIZE, 0777 );
int shmidNanoSecondsClock = shmget(keyNanoSecondsClock, SHM_SIZE, 0777);

//Attach to shared memory

int * secondsClock= (int *)(shmat(shmidSecondsClock, 0, 0));
int * nanoSecondsClock = (int *)(shmat(shmidNanoSecondsClock, 0, 0));


printf("Sleeping for 1 second\n");

printf("In Child -  secondsClock is %d\n", *secondsClock);

sleep(1);


return 1;

}

