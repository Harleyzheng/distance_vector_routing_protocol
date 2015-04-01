#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>

extern int globalMyID;
//last time you heard from each node. TODO: you will want to monitor this
//in order to realize when a neighbor has gotten cut off from you.
extern struct timeval globalLastHeartbeat[256];

//our all-purpose UDP socket, to be bound to 10.1.1.globalMyID, port 7777
extern int globalSocketUDP;
//pre-filled for sending to 10.1.1.0 - 255, port 7777
extern struct sockaddr_in globalNodeAddrs[256];
extern uint32_t costs[256][256];
extern uint32_t nexthops[256];
extern uint32_t buf[512];
extern uint32_t temp[512];

extern FILE* logfile;

//Yes, this is terrible. It's also terrible that, in Linux, a socket
//can't receive broadcast packets unless it's bound to INADDR_ANY,
//which we can't do in this assignment.
void hackyBroadcast(uint32_t* buf, int length)
{
	int i;
	for(i=0;i<256;i++)
		if(i != globalMyID) //(although with a real broadcast you would also get the packet yourself)
			sendto(globalSocketUDP, buf, length, 0,
				  (struct sockaddr*)&globalNodeAddrs[i], sizeof(globalNodeAddrs[i]));
}

void* announceToNeighbors(void* unusedParam)
{
	struct timespec sleepFor;
	sleepFor.tv_sec = 0;
	sleepFor.tv_nsec = 300 * 1000 * 1000; //300 ms



	while(1)
	{
	//printf("before send buff first: %d\n",buf[0]);	
	//printf("before send buff second: %d\n",buf[1]);	
	//printf("before send buff second: %d\n",buf[2]);
		hackyBroadcast(buf, 512*sizeof(int));
		nanosleep(&sleepFor, 0);
	}
}

int copy_string(char destination[], char source[]) {
	int i = 0;

	while (source[i] != '\0') {
	  destination[i] = source[i];
	  i++;
	}
	destination[i] = '\0';

	return i;
}


void listenForNeighbors(char* logfilename)
{
	char fromAddr[100];
	struct sockaddr_in theirAddr;
	socklen_t theirAddrLen;
	unsigned char recvBuf[4*256*2];

	int bytesRecvd;
	while(1)
	{
		logfile = fopen(logfilename, "w+");
		memset(&recvBuf[0], 0, sizeof(recvBuf));
		theirAddrLen = sizeof(theirAddr);
		if ((bytesRecvd = recvfrom(globalSocketUDP, recvBuf, 1000 , 0, 
					(struct sockaddr*)&theirAddr, &theirAddrLen)) == -1)
		{
			perror("connectivity listener: recvfrom failed");
			exit(1);
		}
		
		inet_ntop(AF_INET, &theirAddr.sin_addr, fromAddr, 100);  //This function converts the network address structure sin_addr into a character string fromaddr
		
		short int heardFrom = -1;
		if(strstr(fromAddr, "10.1.1."))  //Returns a pointer to the first occurrence of str2 in str1, or a null pointer if str2 is not part of str1.
		{
			heardFrom = atoi(strchr(strchr(strchr(fromAddr,'.')+1,'.')+1,'.')+1); //Assign it to be node number
			
			//TODO: this node can consider heardFrom to be directly connected to it; do any such logic now.
					

			int i;
	/*		for(i=0;i<256;i++){

			memcpy(&buf[i],&recvBuf[4*i],4);
	
			temp[i] = ntohl(buf[i]);
			}
	*/		
			memcpy(&temp[0],&recvBuf[0],4);
			temp[0] = ntohl(temp[0]);
			memcpy(&temp[1],&recvBuf[4],4);
			temp[1] = ntohl(temp[1]);
			memcpy(&temp[2],&recvBuf[8],4);
			temp[2] = ntohl(temp[2]);

			printf("heard from: %d\n", heardFrom);	
			printf("reached TT first: %d\n",temp[0]);
			printf("reached TT second: %d\n",temp[1]);
			printf("reached TT third: %d\n",temp[2]);
			
			//record that we heard from heardFrom just now.
			gettimeofday(&globalLastHeartbeat[heardFrom], 0);
		}
		

		//Is it a packet from the manager? (see mp2 specification for more details)
		//send format: 'send'<4 ASCII bytes>, destID<net order 2 byte signed>, <some ASCII message>
		else if(!strncmp(recvBuf, "send", 4))
		{
			//TODO send the requested message to the requested destination node
			

			printf("Send msg received\n");

			short int destID = recvBuf[5];
			char message[106];	
			char recvmsg[101];	
			message[0] = 's';	
			message[1] = 'd';
			message[2] = 'i';
			message[3] = 'n';	
			message[4] = 'g';	
			int length = copy_string(&message[5],&recvBuf[6]);   //formatted to be sent
			copy_string(recvmsg,&recvBuf[6]);					 //just for logfile

			printf("Cost msg received, destID is : %hd\n", destID);
			printf("Cost msg received, msg is : %s\n", message);

			//fprintf(logfile, "sending packet dest %hd nexthop %d message %s", destID, nexthop, recvmsg);
			
			sendto(globalSocketUDP, message, length+5, 0,
				  (struct sockaddr*)&globalNodeAddrs[destID], sizeof(globalNodeAddrs[destID]));



		}
		//'cost'<4 ASCII bytes>, destID<net order 2 byte signed> newCost<net order 4 byte signed>
		else if(!strncmp(recvBuf, "cost", 4))
		{

			int recvcost;
			memcpy(&recvcost,&recvBuf[6],4);

			short int nid = recvBuf[5];
			int newcost = ntohl(recvcost);
		
			//printf("Cost msg received, recvbuf is : %s\n", recvBuf);
			printf("Cost msg received, neighborid is : %hd\n", nid);
			printf("Cost msg received, newcost is : %d\n", newcost);

			costs[globalMyID][nid] = newcost;

			//TODO record the cost change (remember, the link might currently be down! in that case,
			//this is the new cost you should treat it as having once it comes back up.)
			// ...
		}
		


		//TODO now check for the various types of packets you use in your own protocol
		else if(!strncmp(recvBuf, "sding", 5))
		{
			char recvmsg[101];	
			copy_string(recvmsg,&recvBuf[5]);

			printf("received msg is: %s\n",recvmsg);
			
			
			fprintf(logfile, "receive packet message %s", recvmsg);
			
		}

		else{
		printf("reached TT\n");

		}

	fclose(logfile);
		  
	}
	//(should never reach here)
	close(globalSocketUDP);
}

