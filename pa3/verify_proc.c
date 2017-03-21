#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "verify.h"
#include <rpc/rpc.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h> 
#include <pthread.h>
// #include <omp.h>

struct verify_arg *ver;
char* verS;
pthread_t *handle;
unsigned int segCounter;

#define PORT_VERIFY 8889
#define BUFLEN 1024

void error(char *msg);
void *setupUdp();

int* rpc_initverifyserver_1_svc(struct verify_arg *arg, struct svc_req * req)
{
	static int result = 0;

	ver = (struct verify_arg*)calloc(1, sizeof(struct verify_arg));
	ver->n = arg->n;
	ver->l = arg->l;
	ver->m = arg->m;
  unsigned int strLength = ver->m * ver->l + 1;
	verS = (char*) calloc(strLength, sizeof(char));
	memset(verS, 0, strLength);
	segCounter = 0;

	/**** set up sockets ****/
	handle = (pthread_t*)calloc(1, sizeof(pthread_t));
	if (handle == NULL)
	{
		printf("Cannot allocate memory for pthread.\n");
		return -1;
	}
	pthread_create(handle, NULL, setupUdp, NULL);

	return &result;
}

void* setupUdp()
{
	struct sockaddr_in si_me, si_other;
     
    int s, i, slen = sizeof(si_other) , recv_len;
    char buf[BUFLEN];

    // zero the buffer
    memset((char *) buf, 0, BUFLEN); 
     
    //create a UDP socket
    if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        error("socket");
    }
    int option = 1;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const void *)&option , sizeof(int));
     
    // zero out the structure
    memset((char *) &si_me, 0, sizeof(si_me));     
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(PORT_VERIFY);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);
     
    //bind socket to port
    if( bind(s , (struct sockaddr*)&si_me, sizeof(si_me) ) == -1)
    {
        error("bind");
    }
     
    //keep listening for data
    printf("Waiting for data...\n");
    recv_len = recvfrom(s, buf, BUFLEN, 0, (struct sockaddr *) &si_other, (socklen_t*)&slen);
    // printf("VER: Data received...\n");
    //try to receive some data, this is a blocking call
    if (recv_len == -1)
    {
        error("recvfrom()");
    }
    strcpy(verS, buf);
    //print details of the client/peer and the data received
    printf("Received packet from %s:%d\n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port));
    printf("Buffer: %s\t verS: %s\n" , buf, verS);  
 
    close(s);

}

char** rpc_getseg_1_svc(long* rank, struct svc_req *req)
{
	static char* seg;
	unsigned int i = 0;
	seg = (char*)calloc(ver->l + 1, sizeof(char));
	memset(seg, '\0', ver->l + 1);
	unsigned int start = segCounter * ver->l;
	char* sentinel = "-";

	if (segCounter >= ver->m)
	{
		strcpy(seg, sentinel);
	}
	else
	{
		strncpy(seg, verS+start, ver->l);		
		segCounter++;
	}
	
	// printf("segCounter = %lu, seg = |%s|\n", segCounter, seg);
	return &seg;
}

char** rpc_getstring_1_svc(void* str, struct svc_req * req)
{
  return &verS;
}


void error(char *msg) {
  perror(msg);
  exit(1);
}
