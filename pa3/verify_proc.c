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

static struct verify_arg *ver;
// static unsigned int verM; 	// num of segments
// static unsigned int verL; 	// length of each segment
// static unsigned int verN; 	// num threads
static char* verS;
static int sock;

#define APPEND_PORT   1987
#define VERIFY_PORT	  1988
#define PORT 1986
#define BUFSIZE 2076

// int hostname_to_ip(char * hostname , char* ip);
void error(char *msg);


int* rpc_initverifyserver_1_svc(struct verify_arg *arg, struct svc_req * req)
// int* rpc_initverifyserver_1(struct verify_arg *arg, CLIENT* clnt)
{
	static int result = 0;

	// verN = arg->n;
	// verL = arg->l;
	// verM = arg->m;
	// verS = "abcdefghijklmno";

	ver = (struct verify_arg*)calloc(1, sizeof(struct verify_arg));
	ver->n = arg->n;
	ver->l = arg->l;
	ver->m = arg->m;
	verS = "abcdefghijklmno";

	/**** set up sockets ****/
	int s; /* socket */
  int portno = 1240; /* port to listen on */
  int clientlen; /* byte size of client's address */
  struct sockaddr_in serveraddr; /* server's addr */
  struct sockaddr_in clientaddr; /* client addr */
  struct hostent *hostp; /* client host info */
  char buf[BUFSIZE]; /* message buf */
  char *hostaddrp; /* dotted decimal host addr string */
  int optval; /* flag value for setsockopt */
  int n; /* message byte size */


  /* 
   * socket: create the parent socket 
   */
  s = socket(AF_INET, SOCK_DGRAM, 0);
  if (s < 0) 
    error("ERROR opening socket");

  /* setsockopt: Handy debugging trick that lets 
   * us rerun the server immediately after we kill it; 
   * otherwise we have to wait about 20 secs. 
   * Eliminates "ERROR on binding: Address already in use" error. 
   */
  optval = 1;
  setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof(int));

  /*
   * build the server's Internet address
   */
  memset((char *) &serveraddr, 0, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
  serveraddr.sin_port = htons((unsigned short)portno);

  /* 
   * bind: associate the parent socket with a port 
   */
  if (bind(s, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0){
    error("ERROR on binding");
  }
  

  /* 
   * main loop: wait for a datagram, then echo it
   */
  clientlen = sizeof(clientaddr);
  while (1) {

    /*
     * recvfrom: receive a UDP datagram from a client
     */
    memset(buf, 0, BUFSIZE);
    n = recvfrom(s, buf, BUFSIZE, 0,(struct sockaddr *) &clientaddr, &clientlen);
    if (n < 0)
      error("ERROR in recvfrom");

    /* 
     * gethostbyaddr: determine who sent the datagram
     */
    hostp = gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr, sizeof(clientaddr.sin_addr.s_addr), AF_INET);
    if (hostp == NULL)
      error("ERROR on gethostbyaddr");
    hostaddrp = inet_ntoa(clientaddr.sin_addr);
    if (hostaddrp == NULL)
      error("ERROR on inet_ntoa\n");
    printf("server received datagram from %s (%s)\n", hostp->h_name, hostaddrp);
    printf("server received %d/%d bytes: %s\n", strlen(buf), n, buf);
    
  }

	return &result;
}

char** rpc_getseg_1_svc(long* rank, struct svc_req *req)
// char** rpc_getseg_1(long* rank, CLIENT* clnt)
{
	static char* seg;
	unsigned int i = 0;
	seg = (char*)calloc(ver->l, sizeof(char));
	unsigned int start = (*rank) * (ver->m/ver->n) * ver->l;
	// unsigned int end = start + L - 1;

	for (i=0 ; i<ver->l; i++)
	{
		seg[i] = verS[start + i];
	}

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