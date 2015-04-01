#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>


void listenForNeighbors();
void* announceToNeighbors(void* unusedParam);


int globalMyID = 0;
//last time you heard from each node. TODO: you will want to monitor this
//in order to realize when a neighbor has gotten cut off from you.
struct timeval globalLastHeartbeat[256];

//our all-purpose UDP socket, to be bound to 10.1.1.globalMyID, port 7777
int globalSocketUDP;
//pre-filled for sending to 10.1.1.0 - 255, port 7777
struct sockaddr_in globalNodeAddrs[256];  
//structures for handling internet addresses
/*
struct sockaddr_in {
    short            sin_family;   // e.g. AF_INET
    unsigned short   sin_port;     // e.g. htons(3490)
    struct in_addr   sin_addr;     // see struct in_addr, below
    char             sin_zero[8];  // zero this if you want to
};
*/
uint32_t costs[256][256];
uint32_t nexthops[256];
uint32_t buf[512];
uint32_t temp[512];
FILE* logfile;

int main(int argc, char** argv)
{
	if(argc != 4)
	{
		fprintf(stderr, "Usage: %s mynodeid initialcostsfile logfile\n\n", argv[0]);
		exit(1);
	}
	
	
	//initialization: get this process's node ID, record what time it is, 
	//and set up our sockaddr_in's for sending to the other nodes.
	globalMyID = atoi(argv[1]);
	int i;
	for(i=0;i<256;i++)
	{
		gettimeofday(&globalLastHeartbeat[i], 0);
		
		char tempaddr[100];    
		sprintf(tempaddr, "10.1.1.%d", i);  	//send formatted 10.1.1.x to tempaddr
		memset(&globalNodeAddrs[i], 0, sizeof(globalNodeAddrs[i]));
		globalNodeAddrs[i].sin_family = AF_INET;   //address family that has been used everywhere
		globalNodeAddrs[i].sin_port = htons(7777);  //from host byte order to network byte order
		inet_pton(AF_INET, tempaddr, &globalNodeAddrs[i].sin_addr);  //load sin_addr
	}
	
	
	//TODO: read and parse initial costs file. default to cost 1 if no entry for a node. file may be empty.
	for(i = 0; i < 256; i++) {
		costs[globalMyID][i] = 1;
		nexthops[i] = i;
	}
	costs[globalMyID][globalMyID] = 0;

	FILE* costfile = fopen(argv[2], "r+");

	
  	int sth;

  	while (!feof(costfile)){  
		fscanf(costfile, "%d", &i);   
		fscanf(costfile, "%d", &sth);  
		costs[globalMyID][i] = sth; 
	}
	
//	for(i=0;i<20;i++)
//	printf("%d\n",costs[i]);
//	printf("end of costs\n");

  	fclose(costfile);    


	//socket() and bind() our socket. We will do all sendto()ing and recvfrom()ing on this one.
	if((globalSocketUDP=socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		perror("socket");
		exit(1);
	}
	char myAddr[100];
	struct sockaddr_in bindAddr;
	sprintf(myAddr, "10.1.1.%d", globalMyID);	
	printf("myaddr is%s\n",myAddr);
	memset(&bindAddr, 0, sizeof(bindAddr));
	bindAddr.sin_family = AF_INET;
	bindAddr.sin_port = htons(7777);
	inet_pton(AF_INET, myAddr, &bindAddr.sin_addr); //This function converts the character string myaddr into a network address structure sin_addr
	if(bind(globalSocketUDP, (struct sockaddr*)&bindAddr, sizeof(struct sockaddr_in)) < 0)
	{
		perror("bindjhjhj");
		close(globalSocketUDP);
		exit(1);
	}
	
	
	for(i=0;i<256;i++){
		int no_ne = htonl(costs[globalMyID][i]);
		int hop = htonl(nexthops[i]);
		buf[i] = no_ne;
		buf[i+256] = hop;
	}

	

	

	printf("test buff first: %d\n",buf[0]);	
	printf("test buff second: %d\n",buf[1]);	
	printf("test buff second: %d\n",buf[2]);



	//start threads... feel free to add your own, and to remove the provided ones.
	pthread_t announcerThread;
	pthread_create(&announcerThread, 0, announceToNeighbors, (void*)0);
	
	
	
	//good luck, have fun!
	listenForNeighbors(argv[3]);
	
	
	
}
