/*
 * psp2link library for PSP2 to communicate and use host file system with psp2client host tool 
 * Copyright (C) 2003,2015 Antonio Jose Ramos Marquez (aka bigboss) @psxdev on twitter
 * Repository https://github.com/psxdev/psp2link
 * based on ps2vfs, ps2client, ps2link, ps2http tools. 
 * Credits goes for all people involved in ps2dev project https://github.com/ps2dev
 * This file is subject to the terms and conditions of the PSP2Link License.
 * See the file LICENSE in the main directory of this distribution for more
 * details.
 */

#include <stdlib.h>
#include <string.h>
#include <psp2/net/net.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/types.h>
#include <psp2/io/dirent.h>
#include <psp2/io/stat.h>
#include <debugnet.h>
#include "psp2link_internal.h"
#include "psp2link.h"

//int server_commands_sockfd = -1;
extern psp2LinkConfiguration *configuration;
#define BUF_SIZE 1024
static char recvbuf[BUF_SIZE] __attribute__((aligned(16)));



void psp2LinkCmdExecElf(psp2link_pkt_exec_cmd *pkg)
{
	char *buf;//buffer for elf file
	int fd; //descriptor to manage file from host0
	int filesize;//variable to control file size  
	int numread;//control read
	debugNetPrintf(DEBUG,"[PSP2LINK] Received command execelf argc=%x argv=%s\n",sceNetNtohl(pkg->argc),pkg->argv);

	//we open file in read only from host0 ps4sh include the full path with host0:/.......
	fd=psp2LinkIoOpen(pkg->argv,SCE_O_RDONLY,0);

	//If we can't open file from host0 print  the error and return
	if(fd<0)
	{
		debugNetPrintf(DEBUG,"[PSP2LINK] psp2LinkOpen returned error %d\n",fd);
		return;
	}
	//Seek to final to get file size
	filesize=psp2LinkIoLseek(fd,0,SCE_SEEK_END);
	//If we get an error print it and return
	if(filesize<0)
	{
		debugNetPrintf(DEBUG,"[PSP2LINK] psp2LinkSeek returned error %d\n",fd);
		psp2LinkIoClose(fd);
		return;
	}
	//Seek back to start
	psp2LinkIoLseek(fd,0,SCE_SEEK_SET);
	//Reserve  memory for read buffer
	buf=malloc(filesize);
	//Read filsesize bytes to buf
	numread=psp2LinkIoRead(fd,buf,filesize);
	//if we don't get filesize bytes we are in trouble
	if(numread!=filesize)
	{
		debugNetPrintf(DEBUG,"[PSP2LINK] ps4LinkRead returned error %d\n",numread);
		psp2LinkIoClose(fd);
		return;		
	}
	//Close file
	psp2LinkIoClose(fd);

	//buffer with elf it's ready time to call load elf stuff....


	return;
}
void psp2LinkCmdExecSprx(psp2link_pkt_exec_cmd *pkg)
{
	debugNetPrintf(DEBUG,"[PSP2LINK] Received command execsprx argc=%d argv=%s\n",sceNetNtohl(pkg->argc),pkg->argv);
	//TODO check psp2LinkCmdExecElf
}
void psp2LinkCmdExit(psp2link_pkt_exec_cmd *pkg)
{
	debugNetPrintf(DEBUG,"[PSP2LINK] Received command exit. Closing PSP2Link...\n");
	
	psp2LinkFinish();
	
}
int psp2link_commands_thread(SceSize args, void *argp)
{
	struct SceNetSockaddrIn serveraddr;
	struct SceNetSockaddrIn remote_addr;
	int ret;
	int len;
	unsigned int addrlen;
	unsigned int cmd;
	psp2link_pkt_hdr *header;
		
	debugNetPrintf(DEBUG,"[PSP2LINK] Command Thread Started.\n" );
	
	configuration->psp2link_commands_sock = sceNetSocket("commands_server_sock",SCE_NET_AF_INET, SCE_NET_SOCK_DGRAM, SCE_NET_IPPROTO_UDP);
	
	if (psp2LinkGetValue(COMMANDS_SOCK) >=0)
	{
		debugNetPrintf(DEBUG,"[PSP2LINK] Created psp2link_commands_sock: %d\n", psp2LinkGetValue(COMMANDS_SOCK));
	}
	else
	{
		debugNetPrintf(DEBUG,"[PSP2LINK] Error creating socket psp2link_commands_sock  0x%08X\n", psp2LinkGetValue(COMMANDS_SOCK));
		psp2LinkFinish();
		return -1;
	}
	/* Fill the server's address */
    memset(&serveraddr, 0, sizeof serveraddr);
	serveraddr.sin_family = SCE_NET_AF_INET;
	serveraddr.sin_addr.s_addr = sceNetHtonl(SCE_NET_INADDR_ANY);
	serveraddr.sin_port = sceNetHtons(psp2LinkGetValue(COMMANDS_PORT));	
	
	ret = sceNetBind(psp2LinkGetValue(COMMANDS_SOCK), (SceNetSockaddr *)&serveraddr, sizeof(serveraddr));
	
	
	
	
	if (ret < 0) 
	{
		debugNetPrintf(DEBUG,"[PSP2LINK] command listener sceNetBind error: 0x%08X\n", ret);
		sceNetSocketClose(psp2LinkGetValue(COMMANDS_SOCK));
		psp2LinkFinish();
		return -1;
	}
	// Do tha thing
	debugNetPrintf(DEBUG,"[PSP2LINK] Command listener waiting for commands...\n");
	
	while(psp2LinkGetValue(CMDSIO_ACTIVE)) {

		addrlen = sizeof(remote_addr);
		//wait for new command
		
		len = sceNetRecvfrom(psp2LinkGetValue(COMMANDS_SOCK), &recvbuf[0], BUF_SIZE, 0, (struct SceNetSockaddr *)&remote_addr,&addrlen);
		debugNetPrintf(DEBUG,"[PSP2LINK] commands listener received packet size (%d)\n", len);

		if (len < 0) {
			debugNetPrintf(DEBUG,"[PSP2LINK] commands listener recvfrom size error (%d)\n", len);
			continue;
		}
		if (len < sizeof(psp2link_pkt_hdr)) {
			debugNetPrintf(DEBUG,"[PSP2LINK] commands listener recvfrom header size error (%d)\n", len);
			continue;
		}

		header = (psp2link_pkt_hdr *)recvbuf;
		cmd = sceNetHtonl(header->cmd);
		
		switch (cmd) {

			case PSP2LINK_EXECELF_CMD:
				psp2LinkCmdExecElf((psp2link_pkt_exec_cmd *)recvbuf);
				break;
			case PSP2LINK_EXECSPRX_CMD:
				psp2LinkCmdExecSprx((psp2link_pkt_exec_cmd *)recvbuf);
				break;
			case PSP2LINK_EXIT_CMD:
				psp2LinkCmdExit((psp2link_pkt_exec_cmd *)recvbuf);
				break;
			default: 
				debugNetPrintf(DEBUG,"[PSP2LINK] Unknown command received\n");
				break;
		}
		debugNetPrintf(DEBUG,"[PSP2LINK] commands listener waiting for next command\n");
	}
	debugNetPrintf(DEBUG,"[PSP2LINK] exit commands listener thread\n");
	if(psp2LinkGetValue(COMMANDS_SOCK))
	{
		debugNetPrintf(DEBUG,"[PSP2LINK] closing server_commands_sock\n");
		sceNetSocketClose(psp2LinkGetValue(COMMANDS_SOCK));
		configuration->psp2link_commands_sock=-1;
	}
	
	sceKernelExitDeleteThread(0);
	return 0;
}
