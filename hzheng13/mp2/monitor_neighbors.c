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
extern uint32_t costs[256];
extern uint32_t nexthops[256];
extern uint32_t buf[256];
extern uint32_t temp[256];
extern uint32_t hops[256];

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
		hackyBroadcast(buf, 256*4);
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
	unsigned char recvBuf[4*256];

	int bytesRecvd;
	while(1)
	{
		
		memset(&recvBuf[0], 0, sizeof(recvBuf));
		theirAddrLen = sizeof(theirAddr);
		if ((bytesRecvd = recvfrom(globalSocketUDP, recvBuf, 4*256 , 0, 
					(struct sockaddr*)&theirAddr, &theirAddrLen)) == -1)
		{
			perror("connectivity listener: recvfrom failed");
			exit(1);
		}
		
		inet_ntop(AF_INET, &theirAddr.sin_addr, fromAddr, 100);  //This function converts the network address structure sin_addr into a character string fromaddr
		
		short int heardFrom = -1;
		
		

		//Is it a packet from the manager? (see mp2 specification for more details)
		//send format: 'send'<4 ASCII bytes>, destID<net order 2 byte signed>, <some ASCII message>
		if(!strncmp(recvBuf, "send", 4))
		{
			logfile = fopen(logfilename, "w+");
			//TODO send the requested message to the requested destination node
			int length;
			char message[106];	
			char recvmsg[101];	

			

			short int destID = recvBuf[5];
			short int no_destID = htons(destID);

			if(costs[destID] == 1)  			//cost = 1 -> cannot reach destID
				fprintf(logfile, "unreachable dest %d\n", destID);
			else{		
				
				message[0] = 's';	
				message[1] = 'd';
				message[2] = 'i';
					

				memcpy(&message[3], &no_destID, sizeof(short int));	
				length = copy_string(&message[5],&recvBuf[6]);   //formatted to be sent
				copy_string(recvmsg,&recvBuf[6]);					 //just for logfile

				printf("Cost msg received, destID is : %hd\n", destID);
				printf("Cost msg received, msg is : %s\n", recvmsg);

				fprintf(logfile, "sending packet dest %d nexthop %d message %s\n", destID,nexthops[destID],recvmsg);

				if(sendto(globalSocketUDP, message, length+5, 0,
				  (struct sockaddr*)&globalNodeAddrs[nexthops[destID]], sizeof(globalNodeAddrs[nexthops[destID]])) < 0)
					perror("sendto()");
			}

			
			fclose(logfile);
			

			//fprintf(logfile, "sending packet dest %hd nexthop %d message %s", destID, nexthop, recvmsg);
			
			
			
		}
		//'cost'<4 ASCII bytes>, destID<net order 2 byte signed> newCost<net order 4 byte signed>
		else if(!strncmp(recvBuf, "cost", 4))
		{
			logfile = fopen(logfilename, "w+");
			int recvcost;
			memcpy(&recvcost,&recvBuf[6],4);

			short int nid = recvBuf[5];
			int newcost = ntohl(recvcost);
		
			//printf("Cost msg received, recvbuf is : %s\n", recvBuf);
			printf("Cost msg received, neighborid is : %hd\n", nid);
			printf("Cost msg received, newcost is : %d\n", newcost);

			costs[nid] = newcost;
			fclose(logfile);
			//TODO record the cost change (remember, the link might currently be down! in that case,
			//this is the new cost you should treat it as having once it comes back up.)
			// ...
		}
		


		//TODO now check for the various types of packets you use in your own protocol
		else if(!strncmp(recvBuf, "sdi", 3))
		{
			logfile = fopen(logfilename, "w+");
			char recvmsg[101];	

								//printf("received hop is: %d\n",recvBuf[4]);
			short int destID = recvBuf[4];

			int length = copy_string(recvmsg,&recvBuf[5]);
			printf("received msg is: %s\n",recvmsg);

			if(globalMyID == destID)
				fprintf(logfile, "receive packet message %s\n", recvmsg);
			else{
				fprintf(logfile, "forward packet dest %d nexthop %d message %s\n", destID, nexthops[destID], recvmsg);
				if(sendto(globalSocketUDP, recvBuf, length+5, 0,
				  (struct sockaddr*)&globalNodeAddrs[nexthops[destID]], sizeof(globalNodeAddrs[nexthops[destID]])) < 0)
					perror("sendto()");
			}
			fclose(logfile);
			
		}
		else  //Returns a pointer to the first occurrence of str2 in str1, or a null pointer if str2 is not part of str1.
		{
			heardFrom = atoi(strchr(strchr(strchr(fromAddr,'.')+1,'.')+1,'.')+1); //Assign it to be node number
			
			//TODO: this node can consider heardFrom to be directly connected to it; do any such logic now.
					
			printf("heardfrom msg is: %d\n",heardFrom);


			int i;
			for(i=0;i<256;i++){
			if(heardFrom == i) continue;	
			
			memcpy(&temp[i],&recvBuf[4*i],4);
			temp[i] = ntohl(temp[i]);

			//memcpy(&hops[i],&recvBuf[4*i+1024],4);
			//hops[i] = ntohl(hops[i]);


			if(costs[i] == 1 && temp[i] != 1){
			
				costs[i] = temp[i]+costs[heardFrom];
				if(heardFrom != nexthops[heardFrom])	
					nexthops[i] = nexthops[heardFrom];
				else nexthops[i] = heardFrom;
				int no_ne = htonl(costs[i]);
				
				buf[i] = no_ne;
				
			}
			
			if(temp[i] != 1)   //if neighbor has cost 1, skip it.
			if(costs[i] > temp[i]+costs[heardFrom]){
			
				costs[i] = temp[i]+costs[heardFrom];
				nexthops[i] = heardFrom;
				int no_ne = htonl(costs[i]);
				
				buf[i] = no_ne;
			}

			if(temp[i] != 1)   //breaking tie
			if(costs[i] == temp[i]+costs[heardFrom]){
		/*		FILE* temp = fopen("temp.txt","w+");
				fprintf(temp,"reached\n");
				fprintf(temp,"heardfrom is %d\n", heardFrom);				
				fprintf(temp,"my nexthop is %d\n", nexthops[globalMyID]);
				fclose(temp);*/
				if(nexthops[i] > heardFrom)			
				{	
					
					nexthops[i] = heardFrom;
				}
			}
			
	//	printf("HeardFrom: %d\n",heardFrom);
	/*	
			printf("costs1 msg is: %d\n",costs[1]);
			printf("costs5 msg is: %d\n",costs[5]);
			printf("costs2 msg is: %d\n",costs[2]);
			printf("costs3 msg is: %d\n",costs[3]);
			
			
			printf("nexthops1 msg is: %d\n",nexthops[1]);
			printf("nexthops5 msg is: %d\n",nexthops[5]);
			printf("nexthops2 msg is: %d\n",nexthops[2]);


			
			printf("nexthops3 msg is: %d\n",nexthops[3]);

*/

			//printf("costs[i] and temp[i] msg is: %d and %d\n",costs[i],temp[i]);




			}
			
			

			//record that we heard from heardFrom just now.
			gettimeofday(&globalLastHeartbeat[heardFrom], 0);
		}


	
		  
	}
	//(should never reach here)
	close(globalSocketUDP);
}

