#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "append.h"
#include <rpc/rpc.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>

// static unsigned int appM; 	// num of segments
// static unsigned int appL; 	// length of each segment
// static unsigned int appF; 	// criteria num to check
// static char appc0; 
// static char appc1; 
// static char appc2;
// static char* app_hostname2 = "";
static int sentS;

static append_arg *app;

#define APPEND_PORT   1987
#define VERIFY_PORT	  1988
#define PORT 1986
#define BUFSIZE 2076

#define SERVER_NAME "moore.mcmaster.ca"
#define SERVER_PORT 1240

void error(char *msg);
int hostname_to_ip(char* hostname, struct sockaddr_in *socket_addr);
int sendSToVerify();

struct sstr
{
	char *s;
	size_t max_length;
	size_t index;
};

static struct sstr *appS;

int* rpc_initappendserver_1_svc(struct append_arg *arg, struct svc_req * req)
// int* rpc_initappendserver_1(struct append_arg *arg, CLIENT* clnt)
{
	printf("Init append server\n");

	static int result = 0;

	// appF = arg->f;
	// appL = arg->l;
	// appM = arg->m;
	// appc0 = arg->c0;
	// appc1 = arg->c1;
	// appc2 = arg->c2;
	// app_hostname2 = arg->hostname2;

	app = (struct append_arg*) calloc(1, sizeof(struct append_arg));
	app->f = arg->f;
	app->l = arg->l;
	app->m = arg->m;
	app->c0 = arg->c0;
	app->c1 = arg->c1;
	app->c2 = arg->c2;
	app->hostname2 = arg->hostname2;	
	sentS = 0;

	// allocate space for S
	appS = (struct sstr*)calloc(1, sizeof(struct sstr));
	appS->max_length = app->m*app->l;
	appS->index = 0;
	appS->s = (char*)calloc(appS->max_length, sizeof(char));

	if (appS == NULL) result = -1;
	return &result;
}

int* rpc_append_1_svc(char* c, struct svc_req * req)
// int* rpc_append_1(char* c, sCLIENT* clnt)
{
	char ch = c[0];
	static int result = 0;

	// printf("index: %d, M: %lu, L: %lu\n", appS->index, app->m, app->l);

	if (appS->index < (app->m*app->l - 1))
	{
		strcat(appS->s, (const char*)&ch);
		appS->index++;		
	}
	else
	{
		result = -1;

		if (sentS == 0)
		{	
			sendSToVerify(app->hostname2);
			sentS = 1;
		}
	}

	return &result;

}

int sendSToVerify()
{
	int s, portno, n;
    int serverlen;
    struct sockaddr_in serveraddr;
    struct hostent *server;
    char *hostname;
    char buf[BUFSIZE];

    portno = SERVER_PORT;

    /* socket: create the socket */
    s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0) 
        error("ERROR opening socket");

    /* build the server's Internet address */
    memset((char *) &serveraddr, 0, sizeof(serveraddr));

    /*gethostbyname: get the server's DNS entry*/
    
    hostname = SERVER_NAME;
    server = gethostbyname(hostname);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host as %s\n", hostname);
        exit(0);
    }
    bcopy((char *)server->h_addr, (char *)&serveraddr.sin_addr.s_addr, server->h_length);

    /* Following line set server address same as the computer where this program runs
        assuming that server and client are in same computer.
        If not, coment following line, define SERVER_NAME and uncoment code block above*/
    //serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);

    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(portno);

    /* get a message from the user */
    memset(buf, 0, BUFSIZE);
    memset(buf, 'a', 10);
    // printf("Please enter msg: ");
    // fgets(buf, BUFSIZE, stdin);

    /* send the message to the server */
    serverlen = sizeof(serveraddr);
    n = sendto(s, buf, strlen(buf), 0, &serveraddr, serverlen);
    while(n < 0){
        n = sendto(s, buf, strlen(buf), 0, &serveraddr, serverlen);
    }
    //if (n < 0) 
    //  error("ERROR in sendto");

    return 0;
}

void error(char *msg) {
    perror(msg);
    exit(0);
}