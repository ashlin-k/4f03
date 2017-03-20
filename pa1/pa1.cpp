// Assignment by
// Ashlin Kanawaty, kanawaa, 1220399
// Prakhar Garg, gargp2, 1204351

#include <algorithm>
#include <math.h>
#include <stdlib.h>
#include <pthread.h> 
#include <stdio.h>
#include <string.h>
#include <unistd.h>

using namespace std;

// suggested by the TA
// struct for string S
struct sstr
{
	pthread_mutex_t lock;
	char *s;
	size_t max_length;
	size_t index;
};

// set up global variables
sstr *S;
pthread_t *handles;
unsigned int N;
unsigned int M; 
unsigned int L; 
unsigned int F; 
char c0; 
char c1; 
char c2;

// declare functions
void* addToS(void* args);
unsigned int count_chars(char* s, char ch, unsigned int start, 
	unsigned int end);
bool enforceF(int a, int b, int c, long rank);

int main(int argc, char **argv)
{
	// check num of args
	if (argc !=  8)
	{
		printf("You must input the correct number of arguments, which is 7.\n");
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
	

	// call one of these for allocating memory for the thread handles
	//handles = malloc(N * sizeof(pthread_t));
	handles = (pthread_t*)calloc(N, sizeof(pthread_t));
	if (handles == NULL)
	{
		printf("Cannot allocate memory for pthread.\n");
		return -1;
	}

	// allocate space for S
	S = (sstr*)calloc(1, sizeof(sstr));
	S->max_length = M*L;
	S->index = 0;
	S->s = (char*)calloc(S->max_length, sizeof(char));

	// initialize muetx
	if (pthread_mutex_init(&(S->lock), NULL) != 0)
    {
        printf("Mutex init failed\n");
        return -1;
    }

    // create variable to store return values from threads
    void* threadPassedPtr;
    int passed = 0;

	// create threads
	long thread;
	for (thread=0; thread<N ; thread++)
	{
		pthread_create(&handles[thread], NULL, addToS, (void*)thread);
	}

	// join threads
	for (unsigned int j=0; j<N ; j++)
	{
		pthread_join(handles[j], &threadPassedPtr);
		passed += *(int*)(&threadPassedPtr);
	}
	free(handles);

	pthread_mutex_destroy(&(S->lock));


//set up print to file ----------------------------------------------

	FILE *fp;
	int i;
   
	/* open the file */
	fp = fopen("out.txt", "w");
	if (fp == NULL) {
		printf("unable to write to out.txt\n");
		exit(0);
      }

//results
	printf("%s\n", S->s);
	fprintf(fp, "%s\n", S->s);
	printf("%d\n", passed);
	fprintf(fp, "%d\n", passed);

   
//close the file
	fclose(fp);

	return 0;
}


// thread function
void* addToS(void* rank)
{
	long my_rank = (long)rank;
	char my_char = 'a' + my_rank;

	//printf ("my rank: %ld,\tmy char: %c\n", my_rank, my_char);

	unsigned int timeMicrosec = 0;
	bool SisComplete = false;
	//bool SPassedCriteria = false;

	int occC0 = 0;
	int occC1 = 0;
	int occC2 = 0;
	bool okToWrite = false;

	// determine number of segments this thread must check
	unsigned int numSegToCheck = (unsigned int) M/N;		// assume M/N evenly for now
	unsigned int counter = 0;
	
	unsigned int startIndex = my_rank * numSegToCheck * L;		
	unsigned int endIndex = startIndex + L - 1;
	unsigned int writingToSeg = (S->index) / L + startIndex;
	unsigned int start=0, end=0;

	while (!SisComplete)
	{
		timeMicrosec = (rand() % 401 + 100) * 1000;		// time in us
		usleep(timeMicrosec);

		pthread_mutex_lock(&(S->lock));		
		if (S->index < M*L)
		{
			// count occurrences of c0, c1 and c2 in S
			start = startIndex + writingToSeg*L;
			end = start + L - 1;
			occC0 = count_chars(S->s, c0, start, end);
			occC1 = count_chars(S->s, c1, start, end);
			occC2 = count_chars(S->s, c2, start, end);
			okToWrite = enforceF(occC0, occC1, occC2, my_rank);
			if (okToWrite)
			{
				strcat(S->s, (const char*)&my_char);
				S->index++;
			}
		}
		else
		{
			SisComplete = true;
		}

		pthread_mutex_unlock(&(S->lock));
	}	

	// final check for passing F
	for (unsigned int i = 0; i < numSegToCheck; i++)
	{
		// start and end indices for each segment
		startIndex += i*L;		
		endIndex += i*L;

		// count occurrences of c0, c1 and c2 in S
		occC0 = count_chars(S->s, c0, startIndex, endIndex);
		occC1 = count_chars(S->s, c1, startIndex, endIndex);
		occC2 = count_chars(S->s, c2, startIndex, endIndex);

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
	}

//	SPassedCriteria = (counter == numSegToCheck);

//	printf("Thread: %ld,\tF has passed: %d\n", my_rank, SPassedCriteria);

//	return (void*)SPassedCriteria;

	return (void*) counter;

}

//function to count occurrences of character in S within a segment
unsigned int count_chars(char* s, char ch, unsigned int start, 
	unsigned int end) 
{  
	int count = 0;

  	for (unsigned int i = start; i <= end; i++)
    	if (s[i] == ch) count++;

  	return count;
}

bool enforceF(int a, int b, int c, long rank)
{
	double sum = 0;
	char ch = 'a' + rank;
	bool okToWrite = false;
	int idealSum = 0;

	switch(F)
	{
		case 0: 			
			sum = c - a - b;
			break;

		case 1: 
			sum = c - 2*b - a;
			break;

		case 2: {
			if ((a != 0) & (b != 0))
			{
				sum = (double)c / ((double)a * (double)b);
			}
			else
			{
				if (c == 0) sum = 1;
				else sum = 2;
			}
			idealSum = 1;
		} break;

		case 3: 
			sum = c + b - a;
			break;
	}	

	if ((sum > idealSum) && ((ch == c0) | (ch == c1)))	// need more a and b
		okToWrite = true;
	else if (sum < idealSum && (ch == c2))		// need more c
		okToWrite = true;
	else if ((sum == idealSum) && ((ch != c0) | (ch != c1) | (ch != c2))) // if good, add a non-a,b,c letter
		okToWrite = true;
	else if ((sum == idealSum) && (S->index%L != 0) && (N == 3))		// if good, but must add a letter to fill segment, N=3
		okToWrite = true;

	// printf("a = %d, b = %d, c = %d\tsum: %f\tokToWrite: %d\n", a,b,c,sum, okToWrite);

	return okToWrite;
}



// when using malloc for S
//malloc(sstr.max_length * sizeof(char));
// or use calloc because calloc will clear and reserve the memory, while malloc will only reserve the memory
//calloc(sstr.max_length * sizeof(char));



// enumerate all the cases
// write a,b,c first, then if you have extra space in the segment, add another a,b,c where necessary to satisfy F
// if F is already satisfied, then can add the other letter
