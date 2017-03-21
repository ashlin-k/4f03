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

int sentS;
struct append_arg *app;
struct sstr *appS;
char *ipVerify;
struct hostent *he;

#define PORT_APPEND 1987
#define PORT_VERIFY 8889
#define BUFLEN 1024
#define SERVER_NAME "moore.mcmaster.ca"

void error(char *msg);
int hostname_to_ip(char* hostname, char* ip);
int sendSToVerify();

struct sstr
{
	char *s;
	size_t max_length;
	size_t index;
};

int* rpc_initappendserver_1_svc(struct append_arg *arg, struct svc_req * req)
{
	static int result = 0;

	app = (struct append_arg*) calloc(1, sizeof(struct append_arg));
	app->f = arg->f;
	app->l = arg->l;
	app->m = arg->m;
	app->c0 = arg->c0;
	app->c1 = arg->c1;
	app->c2 = arg->c2;
	app->hostname2 = arg->hostname2;	
	sentS = 0;

	ipVerify = (char*)calloc(100, sizeof(char));
	hostname_to_ip(app->hostname2, ipVerify);

	// printf("init: IP: %s\n", ipVerify);

	// allocate space for S
	appS = (struct sstr*)calloc(1, sizeof(struct sstr));
	appS->max_length = app->m*app->l;
	appS->index = 0;
	appS->s = (char*)calloc(appS->max_length, sizeof(char));

	if (appS == NULL) result = -1;
	return &result;
}

int* rpc_append_1_svc(char* c, struct svc_req * req)
{
	// printf("char recevied: %c\n", c[0]);
	static int result = 0;

	// printf("index: %d, M: %lu, L: %lu\n", appS->index, app->m, app->l);

	if (appS->index < (app->m*app->l))
	{
		appS->s[appS->index] = c[0];
		appS->index++;	
		appS->s[appS->index] = '\0';
		// printf("S: %s\n", appS->s);	
	}
	else
	{
		result = -1;

		if (sentS == 0)
		{	
			// printf("APP: S is complete\n");
			sendSToVerify(app->hostname2);
			// printf("APP: UDP socket created.\n");
			sentS = 1;
		}
	}

	return &result;

}

int sendSToVerify()
{
	// printf("sendSToVerify: IP: %s\n", ipVerify);

	struct sockaddr_in si_other;
    int s, i; 
 
    if ( (s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        error("socket");
    }
    int option = 1;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

    memset((char*) &si_other, 0, sizeof(si_other));
	si_other.sin_family = AF_INET;
	si_other.sin_port = htons(PORT_VERIFY);
    si_other.sin_addr.s_addr = htonl(INADDR_ANY);
    bcopy((char *)he->h_addr, (char *)&si_other.sin_addr.s_addr, he->h_length);
    int slen=sizeof(si_other);
     
    if (inet_aton(ipVerify , &si_other.sin_addr) == 0) 
    {
        fprintf(stderr, "inet_aton() failed\n");
        exit(1);
    }

    if (sendto(s, appS->s, strlen(appS->s) , 0 , (struct sockaddr *) &si_other, slen)==-1)
    {
        error("sendto()");
    }
    // printf("APP: message: %s\n", appS->s);

	close(s);	

    return 0;
}

void error(char *msg) {
    perror(msg);
    exit(0);
}

int hostname_to_ip(char * hostname , char* ip)
{    
    struct in_addr **addr_list;
    int i;
    char* host;

    host = SERVER_NAME;

    // printf("hostname: %s\n", host);
    he = (struct hostent*)calloc(1, sizeof(struct hostent));
    he = gethostbyname( host );
    // printf("host gotten\n");   
    if ( he == NULL) 
    {
        // get the host info
        herror("gethostbyname");
        return 1;
    }    

    addr_list = (struct in_addr **) he->h_addr_list;
     
    for(i = 0; addr_list[i] != NULL; i++) 
    {
        //Return the first one;
        strcpy(ip , inet_ntoa(*addr_list[i]) );
        return 0;
    }
     
    return 1;
}











// int s, portno, n;
//     int serverlen;
//     struct sockaddr_in serveraddr;
//     struct hostent *server;
//     char *hostname;
//     char buf[BUFSIZE];

//     portno = SERVER_PORT_APPEND;

//     /* socket: create the socket */
//     s = socket(AF_INET, SOCK_DGRAM, 0);
//     if (s < 0) 
//         error("ERROR opening socket");

//     /* build the server's Internet address */
//     memset((char *) &serveraddr, 0, sizeof(serveraddr));

//     /*gethostbyname: get the server's DNS entry*/
    
//     hostname = SERVER_NAME;
//     server = gethostbyname(hostname);
//     if (server == NULL) {
//         fprintf(stderr,"ERROR, no such host as %s\n", hostname);
//         exit(0);
//     }
//     bcopy((char *)server->h_addr, (char *)&serveraddr.sin_addr.s_addr, server->h_length);

//      Following line set server address same as the computer where this program runs
//         assuming that server and client are in same computer.
//         If not, coment following line, define SERVER_NAME and uncoment code block above
//     //serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);

//     serveraddr.sin_family = AF_INET;
//     serveraddr.sin_port = htons(portno);

//     /* get a message from the user */
//     memset(buf, 0, BUFSIZE);
//     memset(buf, 'a', 10);
//     // printf("Please enter msg: ");
//     // fgets(buf, BUFSIZE, stdin);

//     /* send the message to the server */
//     serverlen = sizeof(serveraddr);
//     n = sendto(s, buf, strlen(buf), 0, &serveraddr, serverlen);
//     while(n < 0){
//         n = sendto(s, buf, strlen(buf), 0, &serveraddr, serverlen);
//     }
//     //if (n < 0) 
//     //  error("ERROR in sendto");