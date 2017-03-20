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
#define BUFLEN 2076

// int hostname_to_ip(char * hostname , char* ip);


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
	// create a socket
	struct sockaddr_in socket_append, socket_verify;
	sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock == -1)
	{
		exit(0);
	}
	else
	{
		printf("UDP Socket created successfully.\n");
	}
	// verify socket (server side)
	memset((char *) &socket_verify, 0, sizeof(socket_verify));
	socket_verify.sin_family = AF_INET;
	socket_verify.sin_port = htons(PORT);
	socket_verify.sin_addr.s_addr = htonl(INADDR_ANY); 

	result = bind(sock, (struct sockaddr*)&socket_verify, sizeof(socket_verify));
	if (result == -1)
	{
		exit(0);
	}
	else
		printf("Socket bound successfully.\n");
	
	char* buf[BUFLEN];
	memset((char *) buf, '\0', BUFLEN);
	int recv_len = 0;
	while(buf == '\0')
    {
        printf("Waiting for data...");
        fflush(stdout);
         
        //try to receive some data, this is a blocking call
        if ((recv_len = recvfrom(sock, buf, BUFLEN, 0, (struct sockaddr *) &socket_append, sizeof(socket_append))) == -1)
        {
            exit(1);
        }
         
        //print details of the client/peer and the data received
        printf("Received packet from %s:%d\n", inet_ntoa(socket_append.sin_addr), ntohs(socket_append.sin_port));
        printf("Data: %s\n" , buf);

        sleep(1);
     }

     printf("Buffer: %s\n", buf);

    close(sock);	

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