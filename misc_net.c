#include <stdio.h>
#include <stdlib.h>             /* malloc() */
#include <string.h>             /* strncpy(), memcpy() ... */
#include <unistd.h>             /* getopt() : command line parsing */
#include <sys/socket.h>         /* Network */
#include <netinet/in.h>         /* inet_ntoa */
#include <sys/ioctl.h>          /* get infos about net device */
#include <net/if.h>             /* idem */
#include <linux/if_packet.h>    /* struct sockaddr_ll */

#include "misc_net.h"

char *misc_getIpAddress(char *ifname)
{
	struct ifreq ifr;
	struct sockaddr_in *saddr;
	int fd;
	char *address = "0.0.0.0";

	fd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
	if(fd == -1)
	{
		perror("socket creating failed");
		return NULL;
	}

    memset(&ifr, sizeof(ifr), 0);
    
	strcpy(ifr.ifr_name, ifname);
	if (ioctl(fd, SIOCGIFADDR, &ifr) == 0)
	{
		saddr = (struct sockaddr_in *)&ifr.ifr_addr;
		address = strdup((char *)inet_ntoa(saddr->sin_addr));
    }
    
    close(fd);
        
    return address;
}

char *misc_getMacAddress(char *ifname)
{
    struct ifreq ifr;
    struct sockaddr_in *saddr;
    int fd;
    char *address = NULL;
    static char pMac[18] = "";
    fd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
    if(fd == -1)
    {
        perror("socket creating failed");
        return NULL;
    }

    memset(&ifr, sizeof(ifr), 0);
    
    strcpy(ifr.ifr_name, ifname);
    if(ioctl(fd, SIOCGIFHWADDR, &ifr)==0)
    {
        address = ifr.ifr_hwaddr.sa_data;
        sprintf(pMac, "%02hhX:%02hhX:%02hhX:%02hhX:%02hhX:%02hhX" ,*address, *(address+1), *(address+2), *(address+3), *(address+4), *(address+5));
    }

    close(fd);

    return pMac;
}
