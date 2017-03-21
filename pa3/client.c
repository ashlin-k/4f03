// to run: ./client 0 3 6 6 a b c "moore.mcmaster.ca" "moore.mcmaster.ca"

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <omp.h>
#include <rpc/rpc.h>
#include "append.h"
#include "verify.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>
#include <pthread.h>

// set up global variables
pthread_t *thread_handles;
unsigned int N;		// num threads
unsigned int M; 	// num of segments
unsigned int L; 	// length of each segment
unsigned int F; 	// criteria num to check
char c0; 
char c1; 
char c2;

char *hostname1 = "";				// append server name
char *hostname2 = "";				// verify server name
CLIENT *clnt_append, *clnt_verify;

#define BUFLEN 2076

// declare functions
void* addToS(void* rank);
unsigned int count_chars(char* s, char ch);

int main(int argc, char **argv)
{
	// check num of args
	if (argc !=  10)
	{
		printf("You must input the correct number of arguments, which is 9.\n");
		return -1;
	}

	// get args
	F = strtol(argv[1], NULL, 10);
	N = strtol(argv[2], NULL, 10);
	L = strtol(argv[3], NULL, 10);
	M = strtol(argv[4], NULL, 10);
	c0 = *argv[5];
	c1 = *argv[6];
	c2 = *argv[7];
	hostname1 = argv[8];
	hostname2 = argv[9];
	char *ipAppend, *ipVerify;	

	// check input values
	if ((N < 3) || (N > 8))
	{
		printf("N must be between 3 and 8.\n");
		return -1;
	}
	if ((F < 0) || (F > 3))
	{
		printf("F must be between 0 and 3.\n");
		return -1;
	}
	if ((M <= 0) | (L <= 0))
	{
		printf("M and L must be greater than 0.\n");
		return -1;
	}
	char testC0 = c0 - 'a';
	char testC1 = c1 - 'a';
	char testC2 = c2 - 'a';
	if ((testC0 > N) | (testC1 > N) | (testC2 > N))
	{
		printf("c0, c1 and c2 must be between a and %c.\n", 'a'+N);
		return -1;
	}

	// check if the given parameters can possibly satify criteria F
	// for F0, can satisfy for N=3 if L!=3,5
	//			if N>3, can satisfy for L>=2
	if (F == 0)
	{
		if ( (N == 3 && ((L <= 3) | (L == 5))) | (N > 3 && L < 2) )
		{
			printf("These inputs will not satisfy F0. Please try different inputs.\n");
			return -1;
		}
	}
	// for F1, can satisfy if for N=3, L >= 5
	//			can satisfy any N>3 for L>=2
	else if (F == 1)
	{
		if ( (N == 3 && L < 5) | (N > 3 && L < 2) )
		{
			printf("These inputs will not satisfy F1. Please try different inputs.\n");
			return -1;
		}
	}	
	// for F2, can satisfy if N>=3 for any L
	// for F3, can satisfy for N=3 if (L>=2 & L%2==0)			
	// 			can satisfy for N>3 for L>=2
	else if (F == 3)
	{
		if ( ((N == 3) && ((L < 2) || (L%2 != 0))) | (N > 3 && L < 2) )
		{
			printf("These inputs will not satisfy F3. Please try different inputs.\n");
			return -1;
		}
	}

	// call one of these for allocating memory for the thread thread_handles
	thread_handles = (pthread_t*)calloc(N, sizeof(pthread_t));
	if (thread_handles == NULL)
	{
		printf("Cannot allocate memory for pthread.\n");
		return -1;
	}

	// create clients
	// printf("Creating append client\n");
	clnt_append = clnt_create(hostname1, APPENDPROG, APPENDVERS, "tcp");
	clnt_verify = clnt_create(hostname2, VERIFYPROG, VERIFYVERS, "tcp");
	if (clnt_append == (CLIENT*)NULL)
	{
		printf("AppendServer is null\n");
		clnt_pcreateerror(hostname1);
		exit(1);
	}
	// printf("Append client created sucessfully\n");
	if (clnt_verify == (CLIENT*)NULL)
	{
		printf("VerifyServer is null\n");
		clnt_pcreateerror(hostname2);
		exit(1);
	}
	// printf("Verify client created sucessfully\n");

	// initialize servers and check results are valid
	struct append_arg app_arg;
	app_arg.f = F;
	app_arg.l = L;
	app_arg.m = M;
	app_arg.c0 = c0;
	app_arg.c1 = c1;
	app_arg.c2 = c2;
	app_arg.hostname2 = hostname2;
	int *res1 = rpc_initappendserver_1(&app_arg, clnt_append);
	if (res1 == (int*)NULL)
	{
		printf("Could not locate AppendServer\n");
		clnt_perror(clnt_append, hostname1);
		exit(1);
	}
	if (*res1 == -1)
	{
		printf("Could not initialize AppendServer\n");
		exit(1);
	}
	
	struct verify_arg ver_arg;
	ver_arg.n = N;
	ver_arg.l = L;
	ver_arg.m = M;
	int *res2 = rpc_initverifyserver_1(&ver_arg, clnt_verify);
	if (res2 == (int*)NULL)
	{
		printf("Could not locate VerifyServer\n");
		clnt_perror(clnt_verify, hostname2);
		exit(1);
	}
	if (*res2 == -1)
	{
		printf("Could not initialize VerifyServer\n");
		exit(1);
	}

    // create variable to store return values from threads
    unsigned int passed = 0;
    unsigned long threadCount = 0;

    // printf("Creating threads...\n");

	unsigned int* threadPassedPtr =(unsigned int*)calloc(1, sizeof(unsigned int));
	memset(threadPassedPtr, 0, sizeof(unsigned int));
	for (threadCount = 0; threadCount < N;  threadCount++)
	{
		pthread_create(&thread_handles[threadCount], NULL, addToS, (void*) threadCount);
	}
	for (threadCount = 0; threadCount < N;  threadCount++)
	{
		pthread_join(thread_handles[threadCount], (void*)threadPassedPtr);
		passed+= *threadPassedPtr;		
		// printf("T%lu: returned = %lu, passed = %lu\n", threadCount, *threadPassedPtr, passed);
	}
	free(thread_handles);		

	// get S from verify_server
	char** str_ptr = rpc_getstring_1(NULL, clnt_verify);
	char* str = *str_ptr;
	
	//set up print to file ----------------------------------------------

	FILE *fp;
	fp = fopen("out.txt", "w");
	if (fp == NULL) 
	{
		printf("unable to write to out.txt\n");
		exit(0);
    }

	// results
	printf("%s\n", str);
	fprintf(fp, "%s\n", str);
	printf("%lu\n", passed);
	fprintf(fp, "%lu\n", passed);

	// close the file
	fclose(fp);

	// destroy clients
	if (clnt_append != NULL) clnt_destroy(clnt_append);
	if (clnt_verify != NULL) clnt_destroy(clnt_verify);

	return 0;
}

void* addToS(void* rank)
{	
	long my_rank = (long)rank;
	char my_char = 'a' + my_rank;
	unsigned int timeMicrosec = 0;
	int* appendRet = (int*)calloc(1, sizeof(int));
	*appendRet = 0;
	unsigned int counter = 0;
	unsigned int occC0=0, occC1=0, occC2=0;

	/** append char to S **/
	// printf("--> T%lu: my_char: %c\n", my_rank, my_char);
	while (*appendRet != -1)
	{
		timeMicrosec = (rand() % 401 + 100) * 1000;		// time in us
		usleep(timeMicrosec);

		appendRet = rpc_append_1(&my_char, clnt_append);
		if (appendRet == (int*)NULL)
		{
			clnt_perror(clnt_append, hostname1);
			exit(1);
		}		
		// if (*appendRet != -1) printf("--> T%lu: Added another letter\n", my_rank);		
	}

	// printf("--> T%lu: S is complete\n", my_rank);

	/** verify segments **/
	char** seg;
	seg = (char*)calloc(L, sizeof(char));
	memset(seg, 0, L);

	seg = rpc_getseg_1(&my_rank, clnt_verify);	
	if (seg == (char**)NULL)
	{
		clnt_perror(clnt_verify, hostname2);
		exit(1);
	}	

	while ( ((*seg)[0] != '-') )
	{		
	 	// printf("--> T%lu: seg = |%s|, count = %lu\n", my_rank, *seg, counter);

		// count occurrences of c0, c1 and c2 in S
		occC0 = count_chars(*seg, c0);
		occC1 = count_chars(*seg, c1);
		occC2 = count_chars(*seg, c2);

		switch(F) 
		{
			case 0: 
			counter += (occC0 + occC1 == occC2);
			break;

			case 1: 
			counter += (occC0 + 2*occC1 == occC2);
			break;

			case 2: 
			counter += (occC0 * occC1 == occC2);
			break;

			case 3: 
			counter += (occC0 - occC1 == occC2);
			break;
		}

		seg = rpc_getseg_1(&my_rank, clnt_verify);
		if (seg == (char**)NULL)
		{
			clnt_perror(clnt_verify, hostname2);
			exit(1);
		}
	}

	// printf("--> T%lu: count = %lu\n", my_rank, counter);	

	return (void*) counter;
}


//function to count occurrences of character in S within a segment
unsigned int count_chars(char* s, char ch) 
{  
	int count = 0;
	unsigned int i = 0;

  	for (i = 0; i < L; i++)
    	if (s[i] == ch) count++;

  	return count;
}

