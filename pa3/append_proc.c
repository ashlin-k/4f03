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
#define BUFLEN 2076

int hostname_to_ip(char* hostname, struct sockaddr_in *socket_addr);
int sendSToVerify(char* hostname);

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

int sendSToVerify(char* hostname)
{
	printf("Setting up udp\n");
	struct sockaddr_in si_append;
	int sock;
	char buf[BUFLEN];
	char message[BUFLEN];

	// get ip addr of verify_server
	// mills: 130.113.68.9
	// moore: 130.113.68.130
	memset((char *) &si_append, 0, sizeof(si_append));
    si_append.sin_family = AF_INET;
    si_append.sin_port = htons(PORT);
    char *strIP = "130.113.68.130";
    inet_aton(strIP, &si_append.sin_addr);
	// hostname_to_ip(hostname, &si_append);
	printf("Got IP\n");

		// create socket
    if ( (sock=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        exit(1);
    }
 
 	printf("Socket created\n");	    

    // put string and buffer and send packet
    sprintf(buf, appS->s);
	if (sendto(sock, buf, BUFLEN, 0, (struct sockaddr *)&si_append, (socklen_t)sizeof(si_append))==-1)
	{
		exit(1);
	}
	printf("Send completed\n");
	close(sock);

	return 0;
}

int hostname_to_ip(char *hostname, struct sockaddr_in *socket_addr)
{
    struct addrinfo hints, *res, *p;
    int status;
    char ipstr[INET6_ADDRSTRLEN];

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; // AF_INET or AF_INET6 to force version
    hints.ai_socktype = SOCK_DGRAM;

    printf ("Hostname: %s\n", app->hostname2);

    char* host = "moore.cas.mcmaster.ca";

    if ((status = getaddrinfo(host, NULL, &hints, &res)) != 0) 
    {
        // fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return -1;
    }

    printf("IP addresses for %s:\n\n", host);

    struct sockaddr_in *ipv4 = calloc(1, sizeof(struct sockaddr_in));

    for(p = res;p != NULL; p = p->ai_next) {
        void *addr;
        char *ipver;

        // get the pointer to the address itself,
        // different fields in IPv4 and IPv6:
        if (p->ai_family == AF_INET) // IPv4
        { 
            struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
            addr = &(ipv4->sin_addr);
            ipver = "IPv4";
        } 
        else // IPv6
        { 
        	printf("IPv6 address!\n");
        //     struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
        //     addr = &(ipv6->sin6_addr);
        //     ipver = "IPv6";
        }

        // convert the IP to a string and print it:
        inet_ntop(p->ai_family, addr, ipstr, sizeof(ipstr));
        printf("  %s: %s\n", ipver, ipstr);

        socket_addr->sin_addr.s_addr = ipv4->sin_addr.s_addr;
    }

    freeaddrinfo(res); // free the linked list

    return 0;
}