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
#include <unistd.h>

#define ECHOMAX 300 /* Longest string to echo */

#define BOOL int   /* define boolean value*/
#define TRUE 1
#define FALSE 0


	struct Dmessage{
		char op;
		char htype;
		char hlen;
		char hops;
		int transation_ID;
		short seconds;
		short flags;
		struct in_addr ciaddr;
		struct in_addr yiaddr;
		struct in_addr siaddr;
		struct in_addr giaddr;
		char macaddr[6];
		uint16_t padding[5];
		uint32_t serHostName[16];
		uint16_t bootFileName[64];
		uint8_t magicCookie[4];
		char options[100];
		uint8_t end;
	}Dmessage;
	
	
	void setZero(uint16_t[],int);

	/*A funtion to set zero of a zone*/
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
	int sock; /* Socket descriptor */
	struct sockaddr_in ServAddr; /* Echo server address */
	struct sockaddr_in CltAddr; 
	unsigned short SvrPort = 67; /* Server port */
	unsigned short CltPort = 68; /* Client port */
	unsigned int fromSize; /* In-out of address size for recvfrom() */
	char *servIP; /* IP address of DNS server */ 
	char sendString[ECHOMAX] ; /* String to send to echo server */
	int sendStringLen;  /* Length of string to echo */
	char echoBuffer[ECHOMAX];
	char requireStr[ECHOMAX];
	struct sockaddr_in fromAddr;
	int respStringLen; 
	char ip[4];			//ip address you get
	char mask[4];		//mask you get
	int leaseTime = 20;
	
	/*flags for input mode*/
	BOOL normal;
	BOOL t1Ex;
	BOOL t2Ex;
	BOOL release;
	BOOL inform;
	
	if (argc != 3) /* Test for correct number of
		arguments */
	{
		printf("Usage:\n%s -n -1[for apply for a ip address and enter normally renew period(t1 expires and successful lease renew).]\n",
		argv[0]);
		printf("%s -n -2[for apply for a ip address and enter normally renew period(t2 expires).]\n",
		argv[0]);
		printf("%s -u <Lease Time>[for apply for a ip address and enter normally renew period(t2 expires).]\n",
		argv[0]);
		printf("%s -r <serverIP>[for release a ip addr.]\n",
		argv[0]);
		printf("%s -inform <serverIP>[for inform.]\n",
		argv[0]);
		exit(1);
	}
	
	if(strcmp(argv[1],"-n") == 0){
		normal = TRUE;
		if(strcmp(argv[2],"-1") == 0)
			t1Ex = TRUE;
		else if(strcmp(argv[2],"-2") == 0)
			t2Ex = TRUE;
		else exit(1);
	}
	else if(strcmp(argv[1],"-u") == 0){
			leaseTime = atoi(argv[2]);
			t2Ex = TRUE;
			normal = TRUE;
	}
	else if(strcmp(argv[1],"-r") == 0){
		servIP = argv[2];
		normal = TRUE;
		release = TRUE;
	}
	else if(strcmp(argv[1],"-inform") == 0){
		servIP = argv[2];
		normal = TRUE;
		inform = TRUE;
	}
	
	

	/* Create a datagram/UDP socket */
	if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
	printf("socket() failed.\n");


	int i=1;
	struct ifreq if_eth1;
	socklen_t len = sizeof(i);
	//get the info of eth1.
	strcpy(if_eth1.ifr_name,"eth1");
	 if (ioctl (sock, SIOCGIFHWADDR, &if_eth1) < 0)
    {
        perror ("ioctl");
        return -1;
    }
	
	if(normal){
		
		/*generate a discover message*/
		struct Dmessage discover;

		discover.op = 0x01;
		discover.htype = 0x01;
		discover.hlen = 0x06;
		discover.hops = 0x00;
		discover.transation_ID = 2;
		discover.seconds = 0;
		discover.flags = 128;
		discover.ciaddr.s_addr = inet_addr("0.0.0.0");
		discover.yiaddr.s_addr = inet_addr("0.0.0.0");
		discover.siaddr.s_addr = inet_addr("0.0.0.0");
		discover.giaddr.s_addr = inet_addr("0.0.0.0");
	
		int j=0;
		/*feed in the client mac addr field*/
		while(1){
			discover.macaddr[j] = (unsigned char)if_eth1.ifr_hwaddr.sa_data[j];
			if(j==5) break;
			j++;
		}

		setZero(discover.padding,5);
		setZero(discover.bootFileName,64);

		int k=0;
		while(1){
		
			discover.serHostName[k] = atoi("0.0.0.0");
			if(k==16) break;
			k++;
		}
	
		discover.magicCookie[0] = 0x63;
		discover.magicCookie[1] = 0x82;
		discover.magicCookie[2] = 0x53;
		discover.magicCookie[3] = 0x63;
	
	
		char a[] = {0x35,0x01,0x01};      // option 53
		char b[] = {0x37,0x02,0x01,0x06};  // option 55 reqired list asking for option 1 and option 6
		char  c = 0xff;		// option 255
		
		memset(&discover.options, 0, sizeof(discover.options));
		char *p= &discover.options[0];
		
		memcpy(p,&a,sizeof(a));
		p += sizeof(a);
		
		memcpy(p,&b,sizeof(b));
		p += sizeof(b);
		memcpy(p,&c,1);
	
		if(setsockopt(sock, SOL_SOCKET, SO_BINDTODEVICE,(char *) &if_eth1,sizeof(if_eth1)) <0)
			printf("bind to eth1 failed!");
	
		setsockopt(sock, SOL_SOCKET, SO_BROADCAST,&i,len);
		
		memset(&CltAddr, 0, sizeof(CltAddr));
		CltAddr.sin_family = AF_INET;
		CltAddr.sin_addr.s_addr = htonl(INADDR_ANY);
		CltAddr.sin_port =htons(CltPort);
	
		/* Construct the server address structure */	
		memset(&ServAddr, 0, sizeof(ServAddr));/*Zero out structure*/
		ServAddr.sin_family = AF_INET; /* Internet addr family */
		ServAddr.sin_addr.s_addr = inet_addr("255.255.255.255");/*Server IP address*/
		ServAddr.sin_port = htons(SvrPort); /* Server port */
	
	
		/* Bind to the local address */
		if ((bind(sock, (struct sockaddr *)&CltAddr,
			sizeof(CltAddr))) < 0)
			printf("bind() failed.\n");
			
		/*To send a release message*/
		if(release == TRUE||inform == TRUE){	
		
			/*get local ip addr*/
			if(ioctl(sock,SIOCGIFADDR,&if_eth1)<0) {  
				printf("Error");
			} else {
			ip[0] = (unsigned char)if_eth1.ifr_addr.sa_data[2];
			ip[1] = (unsigned char)if_eth1.ifr_addr.sa_data[3];
			ip[2] = (unsigned char)if_eth1.ifr_addr.sa_data[4];
			ip[3] = (unsigned char)if_eth1.ifr_addr.sa_data[5];
			}
	 
			discover.flags = 0;
			discover.transation_ID = 4;
			memcpy(&discover.ciaddr.s_addr,&ip,sizeof(ip));
			
			/*generate a release message*/
			if(release == TRUE){
				char a1[] = {0x35,0x01,0x07};
				char b1[] = {0x36,0x04,0xc0,0xa8,0x00,0x01};
				memset(&discover.options, 0, sizeof(discover.options));
				char *p2= &discover.options[0];
				memcpy(p2,&a1,sizeof(a1));
				p2 += sizeof(a1);
				memcpy(p2,&b1,sizeof(b1));
				p2 += sizeof(b1);
				memcpy(p2,&c,1);
			} 
			/*generate a inform message*/
			else{
				char a2[] = {0x35,0x01,0x08};
				char b2[] = {0x36,0x04,0xc0,0xa8,0x00,0x01};       // options you want to ask!
				memset(&discover.options, 0, sizeof(discover.options));
				char *p2= &discover.options[0];
				memcpy(p2,&a2,sizeof(a2));
				p2 += sizeof(a2);
				memcpy(p2,&b,sizeof(b));
				p2 += sizeof(b);
				memcpy(p2,&c,1);
			}

			memcpy(&sendString,&discover,sizeof(discover));
			ServAddr.sin_addr.s_addr = inet_addr(servIP);
			if ((sendto(sock,sendString,sizeof(sendString), 0,
				(struct sockaddr *)&ServAddr, sizeof(ServAddr))) != sizeof(sendString))
				printf("sendto() sent a different number of bytes than expected.\n");
				
			/*send a release message*/
			if(release == TRUE){	
				printf("Release sent!\n");
				system("ifconfig eth1 0.0.0.0");
			}	
			/*send a inform message*/
			else{
				printf("Inform sent!\n");
				
				fromSize = sizeof(fromAddr);
				if ((respStringLen = recvfrom(sock, echoBuffer, ECHOMAX, 0,
					(struct sockaddr *)&fromAddr, &fromSize)) != sizeof(echoBuffer))
					printf("recvfrom() failed\n");
					
				printf("ACK received!\n");
				memcpy(&mask,&echoBuffer[251],4);
				char mask_p[16];
				printf("Subnet Mask: %s\n",inet_ntop(AF_INET,&mask,mask_p,(socklen_t )sizeof(mask_p)));
				char dnsServer[2][16];
				memcpy(&dnsServer[0],&echoBuffer[292],4);
				memcpy(&dnsServer[1],&echoBuffer[296],4);
				char dns_p[2][16];
				printf("DNS Server: %s , %s\n",inet_ntop(AF_INET,&dnsServer[0],dns_p[0],(socklen_t )sizeof(dns_p[1])),inet_ntop(AF_INET,&dnsServer[1],dns_p[1],(socklen_t )sizeof(dns_p[1])));
				
			}
			
			exit(0);
		}
		

		// Send the string to the server   (char*)&discover
		if ((sendto(sock,sendString,sizeof(sendString), 0,
			(struct sockaddr *)&ServAddr, sizeof(ServAddr))) != sizeof(sendString))
			printf("sendto() sent a different number of bytes than expected.\n");
		
	
		NAK:  //goto
		memcpy(&sendString,&discover,sizeof(discover));
		// Send the string to the server   discover
		if ((sendto(sock,sendString,sizeof(sendString), 0,
			(struct sockaddr *)&ServAddr, sizeof(ServAddr))) != sizeof(sendString))
			printf("sendto() sent a different number of bytes than expected.\n");
		

		// Recv a response 
		fromSize = sizeof(fromAddr);
		if ((respStringLen = recvfrom(sock, echoBuffer, ECHOMAX, 0,
			(struct sockaddr *)&fromAddr, &fromSize)) != sizeof(echoBuffer))
			printf("recvfrom() failed\n");
	

		memset(&sendString, 0, sizeof(sendString));
		memcpy(&sendString,&discover,240);
		char z[] = {0x35,0x01,0x03};
		char y[] = {0x32,0x04};
		char *q= &sendString[240];
		memcpy(q,&z,sizeof(z));
		q += sizeof(z);
		memcpy(q,&b,sizeof(b));
		q += sizeof(b);
		memcpy(q,&y,sizeof(y));
		q += sizeof(y);
		memcpy(q,&echoBuffer[16],4);
		q += 4;
		memcpy(q,&c,1);
	
	
		if ((sendto(sock,sendString,sizeof(sendString), 0,
			(struct sockaddr *)&ServAddr, sizeof(ServAddr))) != sizeof(sendString))
			printf("sendto() sent a different number of bytes than expected.\n");
		
	
		if ((respStringLen = recvfrom(sock, echoBuffer, ECHOMAX, 0,
			(struct sockaddr *)&fromAddr, &fromSize)) != sizeof(echoBuffer))
			printf("recvfrom() failed\n");
			
		/* Whether the recierved packet is the offer of send discover*/
		if(echoBuffer[4] != 0x02 || echoBuffer[5] != 0x00 || echoBuffer[6] != 0x00 || echoBuffer[7] != 0x00)
			{	printf("Bad Received\n");		exit(0);}
		
		char addr[16];
		char mask_p[16];
		memcpy(&ip,&echoBuffer[16],4);
		memcpy(&mask,&echoBuffer[251],4);
		printf("Your ip address: %s\nMask: %s\n",inet_ntop(AF_INET,&ip,addr,(socklen_t )sizeof(addr)),inet_ntop(AF_INET,&mask,mask_p,(socklen_t )sizeof(mask_p)));
	
		/*configure the ip address into eth1*/
		char cmd[30] ={"ifconfig eth1 "};
		strcat(cmd,inet_ntop(AF_INET,&ip,addr,(socklen_t )sizeof(addr)));
		strcat(cmd," netmask 255.255.255.0");
		printf("%s\n",cmd);
		system(cmd);
		
		
			memcpy(&requireStr,&sendString,10);
			char *pp= &requireStr[10];
			short uniFlags = 0;
			memcpy(pp,&uniFlags,sizeof(uniFlags));
			pp+=sizeof(uniFlags);
			memcpy(pp,&ip,sizeof(ip));
			pp+=sizeof(ip);
			memcpy(pp,&sendString[16],229);
			pp+=229;
			char zero[]={0x00,0x00,0x00,0x00};
			memcpy(pp,&zero,4);
			pp+=4;
			memcpy(pp,&c,1);
			
			memcpy(&ServAddr.sin_addr.s_addr,&echoBuffer[257],4);/*Server IP address*/
			
		/*for T1 time release*/
		for(;t1Ex==TRUE;){
			sleep(10);  
			printf("T1 expires!\n");

			
			if ((sendto(sock,requireStr,sizeof(requireStr), 0,
				(struct sockaddr *)&ServAddr, sizeof(ServAddr))) != sizeof(requireStr))
				printf("sendto() sent a different number of bytes than expected.\n");
			
			fromSize = sizeof(fromAddr);
			if ((respStringLen = recvfrom(sock, echoBuffer, ECHOMAX, 0,
				(struct sockaddr *)&fromAddr, &fromSize)) != sizeof(echoBuffer))
				printf("recvfrom() failed\n");
			if(echoBuffer[4] != 0x02 || echoBuffer[5] != 0x00 || echoBuffer[6] != 0x00 || echoBuffer[7] != 0x00)
			{	printf("Bad Received\n");		exit(0);}
		
		
			if(echoBuffer[242]==0x05){
				printf("ACK Received!\n");
				continue;
			}
			else if(echoBuffer[242]==0x06){  
				ServAddr.sin_addr.s_addr = inet_addr("255.255.255.255");
				
			/*reset the ip addr*/
			system("ifconfig eth1 0.0.0.0");
				goto NAK;
			}
		}
		
		/*for T2 time release*/
		for(;t2Ex==TRUE;){
			//printf("%d",(int) (0.5*leaseTime));
			sleep((int) (0.5*leaseTime) );  
			printf("T1 expires!\n");
			
			memset(&ServAddr.sin_addr.s_addr,0,sizeof(ServAddr.sin_addr.s_addr));
			memcpy(&ServAddr.sin_addr.s_addr,&echoBuffer[257],4);/*Server IP address*/
			if ((sendto(sock,requireStr,sizeof(requireStr), 0,
				(struct sockaddr *)&ServAddr, sizeof(ServAddr))) != sizeof(requireStr))
				printf("sendto() sent a different number of bytes than expected.\n");
				
				
			sleep((int) (0.25*leaseTime) ); 
			printf("T2 expires!\n");
			
			ServAddr.sin_addr.s_addr = inet_addr("255.255.255.255");
			memset(&sendString,0,sizeof(sendString));
			memcpy(&sendString,&requireStr,sizeof(requireStr));
			if ((sendto(sock,sendString,sizeof(sendString), 0,
				(struct sockaddr *)&ServAddr, sizeof(ServAddr))) != sizeof(requireStr))
				printf("sendto() sent a different number of bytes than expected.\n");
				
			sleep(5); 
			printf("Lease expires!\n");	
			
			/*reset the ip addr*/
			system("ifconfig eth1 0.0.0.0");
			goto NAK;
			break;
		}
		
	}
	
	/* null-terminate the received data */
	close(sock);
	exit(0);
}
