 // socket program stuff

#define CLIENT_PORT   1986
#define APPEND_PORT   1987
#define VERIFY_PORT	  1988
#define BUFLEN 2076

int hostname_to_ip(char * hostname , char* ip);

 in main 
/**** get ip addresses of severs ****/
hostname_to_ip(hostname1, ipAppend);
hostname_to_ip(hostname2, ipVerify);
printf ("IP addresses are:\tappend=%s\n", ipAppend);

	/**** set up sockets ****/
	// client socket
	struct sockaddr_in socket_client, socket_append, socket_verify;
	int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock == -1)
	{
		exit(0);
	}
	else
	{
		printf("UDP Socket created successfully.\n");
	}
	memset((char *) &socket_client, 0, sizeof(socket_client));
	socket_client.sin_family = AF_INET;
	socket_client.sin_port = htons(CLIENT_PORT);
	socket_client.sin_addr.s_addr = htonl(INADDR_ANY);
	// append server socket
	memset((char *) &socket_append, 0, sizeof(socket_append));
	socket_append.sin_family = AF_INET;
	socket_append.sin_port = htons(APPEND_PORT);
	inet_aton(ipAppend, &socket_append.sin_addr);    

	int retVal = bind(sock, (struct sockaddr*)&socket_client, sizeof(socket_client));
	if (retVal == -1)
	{
		exit(0);
	}
	else
		printf("Socket bound successfully.\n");

int hostname_to_ip(char * hostname , char* ip)
{
    struct hostent *he;
    struct in_addr **addr_list;
    int i;
         
    printf("pt 1\n");

    if ( (he = gethostbyname( hostname ) ) == NULL) 
    {
         get the host info
        herror("gethostbyname");
        return 1;
    }

    printf("pt 2\n");
 
    addr_list = (struct in_addr **) he->h_addr_list;
     
    printf("pt 3\n");
     
    for(i = 0; addr_list[i] != NULL; i++) 
    {
    	printf("FOR LOOP: %d\n", i);
        Return the first one;
        strcpy(ip , inet_ntoa(*addr_list[i]) );
        return 0;
    }
     
    printf("pt 4\n");

    return 1;
}