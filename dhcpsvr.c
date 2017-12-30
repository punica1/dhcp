#include <stdio.h> /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket(), sendto() and recvfrom() */
#include <arpa/inet.h> /* for sockaddr_in and inet_addr() */
#include <stdlib.h> /* for atoi() and exit() */
#include <string.h> /* for memset() */
#include <unistd.h> /* for close() */
#include <net/if.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <time.h>

#define ECHOMAX 320/* Longest string to echo */
#define IPnum 5

#define BOOL int   /* define boolean value*/
#define TRUE 1
#define FALSE 0
	
	struct message{
		char op;
		char htype;
		char hlen;
		char hops;
		int transation_ID;
		short seconds;
		short  flags;
		struct in_addr ciaddr;
		struct in_addr yiaddr;
		struct in_addr siaddr;
		struct in_addr giaddr;
		uint8_t macaddr[6];
		uint16_t padding[5];
		uint32_t serHostName[16];
		uint16_t bootFileName[64];
		uint8_t magicCookie[4];
		char options[100];
	}message;
	
	void setZero(uint16_t[],int);

	void setZero(uint16_t a[],int value){
	int w=0; 
	while(1){
		a[w] = 0x0000;
		if(w==(value-1)) break;
		w++;
	}
	return;
	}

int main(int argc, char *argv[])
{
	int sock; /* Socket */
	struct sockaddr_in ServAddr; /* Local address */
	struct sockaddr_in ClntAddr,fromAddr; /* Client address */
	unsigned int cliAddrLen; /* Length of client address */
	
	char sendBuffer[ECHOMAX]; /* Buffer for message */
	//char RcvBuffer[ECHOMAX];
	
	unsigned short ServPort = 67; /* Server port */
	unsigned short CltPort = 68; 
	
	char recvMsgSize; /* Size of received message */
	char assignedIP[IPnum][15];	
	char opValue[3][15];
	struct ifreq if_eth1;
	BOOL nak;
	BOOL ipGot;

	if (argc > 2) /* Test for correct number of
		arguments */
	{
		printf("Usage:\n%s [for normal worked server.]\n",
		argv[0]);
		printf("%s -n[server will give nak when t1 expires]\n",
		argv[0]);
		exit(1);
	}
	
	if( argc == 1 ) ;
	else if (strcmp(argv[1],"-n") == 0){
		nak = TRUE;
	}
	else {
		printf("Bad input!\n");
		exit(0);
	}
	
	/* Create socket for sending/receiving datagrams */
	if ((sock = socket(PF_INET, SOCK_DGRAM, 0)) < 0)
		printf("socket() failed.\n");
	
	strcpy(if_eth1.ifr_name,"eth1");
	if(setsockopt(sock, SOL_SOCKET, SO_BINDTODEVICE,(char *) &if_eth1,sizeof(if_eth1)) <0)
		printf("bind to eth1 failed!");
	
	int i=1;
	socklen_t len = sizeof(i);
	setsockopt(sock, SOL_SOCKET, SO_BROADCAST,&i,len);
	
	unsigned char RcvBuffer[ECHOMAX];
		
	/* Construct local address structure */
	memset(&ServAddr, 0, sizeof(ServAddr));
	ServAddr.sin_family = AF_INET;
	ServAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	ServAddr.sin_port =htons(ServPort);
	
	memset(&ClntAddr, 0, sizeof(ClntAddr));
	ClntAddr.sin_family = AF_INET;
	ClntAddr.sin_port =htons(CltPort);
	ClntAddr.sin_addr.s_addr = inet_addr("255.255.255.255");
	
		
	/* Bind to the local address */
	if ((bind(sock, (struct sockaddr *)&ServAddr,
		sizeof(ServAddr))) < 0)
		printf("bind() failed.\n");
	
	
	struct message receiveMes;
	for (;;)// Run forever 
	{	
		int i = sizeof(fromAddr);
		memset(&RcvBuffer,0,sizeof(RcvBuffer));
		// Block until receive message from a client 
		if ((recvMsgSize = recvfrom(sock,RcvBuffer, ECHOMAX,0,(struct sockaddr *)&fromAddr, &i)) < 0)
			printf("recvfrom() failed.\n");
		
		if(RcvBuffer[242] == 0x07){
			printf("Release received!\n");
			continue;
		}
		
		/*generate a dhcp message*/
		struct message sendMes;
			memset(&sendMes, 0, sizeof(sendMes));  //clear the buffer
			sendMes.op = 0x02;
			sendMes.htype=0x01;
			sendMes.hlen=0x06;
			sendMes.hops=0x00;
			memcpy(&sendMes.transation_ID,&RcvBuffer[4],4);
			sendMes.seconds=0x00;
			memcpy(&sendMes.flags,&RcvBuffer[10],2);
		
			//printf("%s",assignedIP[0]);
			sendMes.ciaddr.s_addr = inet_addr("0.0.0.0");//receiveMes.ciaddr.s_addr;
			sendMes.siaddr.s_addr = inet_addr("0.0.0.0") ;
			sendMes.giaddr.s_addr = inet_addr("0.0.0.0") ;
			
			int j = 0;
			while(1){
				memcpy(&sendMes.macaddr[j],&RcvBuffer[28+j],1);
				//sendMes.macaddr[j] = receiveMes.macaddr[j];
				if(j==5) break;
				j++;
			}
			
			setZero(sendMes.padding,5);
			setZero(sendMes.bootFileName,64);
		
			int k=0;
			while(1){
		
				sendMes.serHostName[k] = atoi("0.0.0.0");
					if(k==15) break;
				k++;
			}
		
		
			sendMes.magicCookie[0] = 0x63;
			sendMes.magicCookie[1] = 0x82;
			sendMes.magicCookie[2] = 0x53;
			sendMes.magicCookie[3] = 0x63;
			//if(receiveMes.options.type == 0x35) printf("1111111\n");
			
			/*generate options*/
			char b[] = {0x01,0x04,0xff,0xff,0xff,0x00};
			char c[] = {0x36,0x04,0xc0,0xa8,0x00,0x01};
			char d[] = {0x03,0x04,0xca,0xa8,0x00,0x00};
			char e[] = {0x0f,0x09,0xca,0xa8,0x00,0x00,0xca,0xa8,0x00,0x00,0x00};
			char f[] = {0x3a,0x04,0x00,0x00,0x00,0x0a};      //T1
			char g[] = {0x3b,0x04,0x00,0x00,0x01,0x0f};		//T2
			char h[] = {0x06,0x08,0xca,0x63,0xd8,0x71,0xca,0x63,0xc0,0x44};	     // DNS Server
			char end = 0xff;
		
		
		//printf("%2x\n",RcvBuffer[242]);	
		if(RcvBuffer[242] == 0x01){
			
			/*get a ip to assign to the client*/
			FILE *fp=fopen("dhcp.config","r");
			if(!fp)
			{
				printf("can't open file\n");
				return -1;
			}
			fscanf(fp,"%s %s %s\n",opValue[0],opValue[1],opValue[2]);
			int ij=0;   
			while(fscanf(fp,"%s\n",assignedIP[ij])!= EOF)
			{
				//printf("ipget:%s\n",assignedIP[ij]);
				ij++;
				continue;
			}			
			fclose(fp);
		
			
			/*rewrite the file*/
			FILE *fp2=fopen("dhcp.config","w");
			if(!fp2)
			{
				printf("can't open file\n");
				return -1;
			}
			fprintf(fp2,"%s %s %s\n",opValue[0],opValue[1],opValue[2]);
			int iw=1;
			while(iw<ij)
			{
				fprintf(fp2,"%s\n",assignedIP[iw]);
				iw++;
				continue;
			}
			fclose(fp2);
			
			sendMes.yiaddr.s_addr = inet_addr(assignedIP[0]);
			
			printf("DHCP DISCOVER received!\n");
		
			char a[] = {0x35,0x01,0x02};

			memset(&sendMes.options, 0, sizeof(sendMes.options));
			char *p= &sendMes.options[0];
			memcpy(p,&a,sizeof(a));
			p += sizeof(a);
			memcpy(p,&b,sizeof(b));
			p += sizeof(b);
			memcpy(p,&c,sizeof(c));
			p += sizeof(c);
			memcpy(p,&d,sizeof(d));
			p += sizeof(d);
			memcpy(p,&e,sizeof(e));
			p += sizeof(e);
			memcpy(p,&f,sizeof(f));
			p += sizeof(f);
			memcpy(p,&g,sizeof(g));
			p += sizeof(g);	
			memcpy(p,&h,sizeof(h));
			p += sizeof(h);	
			//char end = 0xff;
			memcpy(p,&end,sizeof(end));

			
			memcpy(&sendBuffer,&sendMes,sizeof(sendMes));
			if ((sendto(sock, sendBuffer, sizeof(sendBuffer), 0, (struct sockaddr *)&ClntAddr, sizeof(ClntAddr ))!= sizeof(sendBuffer)) )
				printf("sendto() sent a different number of bytes than expected.\n");
			printf("DHCP OFFER send!\n");
		}
		
		/*For received a DHCP request/inform messege*/
		else if(RcvBuffer[242] == 0x03|| RcvBuffer[242] == 0x08){
			
			if(RcvBuffer[242] == 0x03)
				printf("DHCP REQUEST received!\n");
			else 	printf("DHCP INFORM received!\n");
		
			sendMes.yiaddr.s_addr = inet_addr(assignedIP[0]);
			memset(&sendBuffer, 0, sizeof(sendBuffer));
			memcpy(&sendBuffer,&sendMes,240);
			
			char z[] = {0x35,0x01,0x05};  
			char x[] = {0x35,0x01,0x06};
			char y[] = {0x33,0x04,0x00,0x00,0x00,0x14};
			char *q= &sendBuffer[240];
			
			/*for NAK, generate a NAK message*/
			if(nak == TRUE && ipGot == TRUE){
				memcpy(q,&x,sizeof(x));
				q += sizeof(x);
				memcpy(q,&c,sizeof(c));
				q += sizeof(c);
				
				char zero1[] = {0x00,0x00,0x00,0x00};
				memcpy(&sendBuffer[12],&zero1,sizeof(zero1));
			}
			else {
				memcpy(q,&z,sizeof(z));
				q += sizeof(z);
				memcpy(q,&y,sizeof(y));
				q += sizeof(y);
				memcpy(q,&b,sizeof(b));
				q += sizeof(b);
				memcpy(q,&c,sizeof(c));
				q += sizeof(c);
				memcpy(q,&d,sizeof(d));
				q += sizeof(d);
				memcpy(q,&e,sizeof(e));
				q += sizeof(e);
				memcpy(q,&f,sizeof(f));
				q += sizeof(f);
				memcpy(q,&g,sizeof(g));
				q += sizeof(g);	
				memcpy(q,&h,sizeof(h));
				q += sizeof(h);	
			}
			memcpy(q,&end,sizeof(end));
			
			/*if received message is expirarion renewal -> unicast*/
			if(RcvBuffer[10] ==0x00 && RcvBuffer[11] ==0x00)
				memcpy(&ClntAddr.sin_addr.s_addr,&RcvBuffer[12],4);
		
			if ((sendto(sock, sendBuffer, sizeof(sendBuffer), 0, (struct sockaddr *)&ClntAddr, sizeof(ClntAddr ))!= sizeof(sendBuffer)) )
				printf("sendto() sent a different number of bytes than expected.\n");
			
			if(nak == TRUE && ipGot == TRUE){
				 printf("DHCP NACK send!\n");
				 ipGot =FALSE;
			}
			else if(RcvBuffer[242] == 0x03){
				printf("DHCP ACK send!\n");
		
				if(RcvBuffer[10] ==0x80 && RcvBuffer[11] ==0x00){
					
					/*recoed the assigned ip and info of client into file*/
					FILE *fp3=fopen("dhcp.lease","a");
					if(!fp3)
					{
						printf("can't open file\n");
						return -1;
					}
		
					time_t time1;
					time(&time1);

					fprintf(fp3,"%s %x:%x:%x:%x:%x:%x %s\n",assignedIP[0],sendMes.macaddr[0],sendMes.macaddr[1],
							sendMes.macaddr[2],sendMes.macaddr[3],sendMes.macaddr[4],sendMes.macaddr[5],ctime (&time1));
					fclose(fp3);
					printf("dhcp.lease renewed!\n");
				}
				ipGot =TRUE;
			}
			
			ClntAddr.sin_addr.s_addr = inet_addr("255.255.255.255");
		}
	}	
	
}

