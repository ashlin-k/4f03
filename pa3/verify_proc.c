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

#define PORT_VERIFY 8888
#define BUFLEN 2076

// int hostname_to_ip(char * hostname , char* ip);
void error(char *msg);
void *setupUdp();

int* rpc_initverifyserver_1_svc(struct verify_arg *arg, struct svc_req * req)
// int* rpc_initverifyserver_1(struct verify_arg *arg, CLIENT* clnt)
{
	static int result = 0;

	ver = (struct verify_arg*)calloc(1, sizeof(struct verify_arg));
	ver->n = arg->n;
	ver->l = arg->l;
	ver->m = arg->m;
	verS = (char*) calloc(BUFLEN, sizeof(char));
	memset(verS, '\0', BUFLEN);
	segCounter = 0;

	/**** set up sockets ****/
	// #pragma omp parallel num_threads(1)
	// setupUdp();
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
    // fflush(stdout);
    recv_len = recvfrom(s, buf, BUFLEN, 0, (struct sockaddr *) &si_other, (socklen_t*)&slen);
    printf("VER: Data received...\n");
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
// char** rpc_getseg_1(long* rank, CLIENT* clnt)
{
	static char* seg;
	unsigned int i = 0;
	seg = (char*)calloc(ver->l + 1, sizeof(char));
	memset(seg, '\0', ver->l + 1);
	unsigned int start = segCounter * ver->l;
	char* sentinel = "-";
	// seg = "a" + segCounter;
	// unsigned int start = (*rank) * (ver->m/ver->n) * ver->l;
	// unsigned int end = start + L - 1;

	if (segCounter >= ver->m)
	{
		strcpy(seg, sentinel);
	}
	else
	{
		strncpy(seg, verS+start, ver->l);		
		segCounter++;
	}
	
	printf("segCounter = %lu, seg = %s\n", segCounter, seg);
	return &seg;
}

char** rpc_getstring_1_svc(void * ptr, struct svc_req * req)
{
	return &verS;
}


void error(char *msg) {
  perror(msg);
  exit(1);
}

// int hostname_to_ip(char * hostname , char* ip)
// {
//     struct hostent *he;
//     struct in_addr **addr_list;
//     int i;
         
//     if ( (he = gethostbyname( hostname ) ) == NULL) 
//     {
//         // get the host info
//         herror("gethostbyname");
//         return 1;
//     }

//     addr_list = (struct in_addr **) he->h_addr_list;
     
//     for(i = 0; addr_list[i] != NULL; i++) 
//     {
//         //Return the first one;
//         strcpy(ip , inet_ntoa(*addr_list[i]) );
//         return 0;
//     }
     
//     return 1;
// }

	// int s; 
 //  int portno = 1240; /* port to listen on */
 //  int clientlen; /* byte size of client's address */
 //  struct sockaddr_in serveraddr; /* server's addr */
 //  struct sockaddr_in clientaddr; /* client addr */
 //  struct hostent *hostp; /* client host info */
  // char buf[BUFSIZE]; /* message buf */
  // char *hostaddrp;  dotted decimal host addr string 
  // int optval; /* flag value for setsockopt */
  // int n; /* message byte size */


  /* 
   * socket: create the parent socket 
  //  */
  // s = socket(AF_INET, SOCK_DGRAM, 0);
  // if (s < 0) 
  //   error("ERROR opening socket");

  /* setsockopt: Handy debugging trick that lets 
   * us rerun the server immediately after we kill it; 
   * otherwise we have to wait about 20 secs. 
   * Eliminates "ERROR on binding: Address already in use" error. 
   */
  // optval = 1;
  // setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof(int));

  /*
   * build the server's Internet address
   */
  // memset((char *) &serveraddr, 0, sizeof(serveraddr));
  // serveraddr.sin_family = AF_INET;
  // serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
  // serveraddr.sin_port = htons((unsigned short)portno);

  /* 
   * bind: associate the parent socket with a port 
   */
  // if (bind(s, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0){
  //   error("ERROR on binding");
  // }
  

  /* 
   * main loop: wait for a datagram, then echo it
   */
  // clientlen = sizeof(clientaddr);
  // while (1) {

  // 	printf("Waiting...\n");
  //   /*
  //    * recvfrom: receive a UDP datagram from a client
  //    */
  //   memset(buf, 0, BUFSIZE);
  //   n = recvfrom(s, buf, BUFSIZE, 0,(struct sockaddr *) &clientaddr, &clientlen);
  //   if (n < 0)
  //     error("ERROR in recvfrom");

  //   /* 
  //    * gethostbyaddr: determine who sent the datagram
  //    */
  //   // hostp = gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr, sizeof(clientaddr.sin_addr.s_addr), AF_INET);
  //   // if (hostp == NULL)
  //   //   error("ERROR on gethostbyaddr");
  //   // hostaddrp = inet_ntoa(clientaddr.sin_addr);
  //   // if (hostaddrp == NULL)
  //   //   error("ERROR on inet_ntoa\n");
  //   // printf("server received datagram from %s (%s)\n", hostp->h_name, hostaddrp);
  //   // printf("server received %d/%d bytes: %s\n", strlen(buf), n, buf);
    
  // }