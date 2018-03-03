
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

void print_usage();

int main (int argc, char *argv[]) {

int maxProcess;
FILE * fp;
int terminateTime;


char fileName[50];
fp = fopen(fileName, "w");

char option;

while ((option = getopt(argc, argv, "s:hl:t:")) != -1){
	switch (option){
		case 's' : maxProcess = atoi(optarg);
//			printf("maxProcess is %d\n", maxProcess);
//			printf("s was chosen\n");
		break;

		case 'h': print_usage();
//			printf("h was chosen\n");
//		return(0);	
		break;

		case 'l': 
			strcpy(fileName, optarg);
//			printf("file name is %s\n", fileName);
//			printf("l was chosen\n");
		break;

		case 't':
			terminateTime = atoi(optarg);
//			printf("terminate in %d seconds\n", terminateTime);
//			printf("t was chosen\n");
		break;

		default : print_usage(); 
			exit(EXIT_FAILURE);
	}
}

}

void print_usage(){
	printf("future help message\n");
}
