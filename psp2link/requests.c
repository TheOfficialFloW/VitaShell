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


#include <stdio.h>
#include <stdarg.h>
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


#define PACKET_MAXSIZE 4096

static char send_packet[PACKET_MAXSIZE] __attribute__((aligned(16)));
static char recv_packet[PACKET_MAXSIZE] __attribute__((aligned(16)));
unsigned int remote_pc_addr = 0xffffffff;

//int server_requests_sock = -1;
//int psp2link_fileio_sock = -1;
//extern int psp2link_fileio_active;
//extern int psp2link_requests_port;


extern psp2LinkConfiguration *configuration;
int psp2link_requests_connected;


int psp2LinkRequestsIsConnected()
{
 	if(psp2link_requests_connected)
 	{
		debugNetPrintf(INFO,"[PSP2LINK] psp2link connected  %d\n", psp2link_requests_connected);
 	}
	else
	{
       // debugNetPrintf(INFO,"psp2link is not connected   %d\n", psp2link_requests_connected);
	}
	return psp2link_requests_connected;
}
void psp2LinkRequestsAbort()
{
	int ret;
	if(psp2LinkGetValue(REQUESTS_SOCK))
	{
		debugNetPrintf(DEBUG,"[PSP2LINK] Aborting psp2link_requests_sock\n");
		
		ret = sceNetSocketAbort(psp2LinkGetValue(REQUESTS_SOCK),1);
		if (ret < 0) {
			debugNetPrintf(DEBUG,"[PSP2LINK] abort psp2link_requests_sock returned error 0x%08X\n", ret);
		}
	}
}
void psp2link_close_socket(void)
{
	int ret;

	ret = sceNetSocketClose(psp2LinkGetValue(FILEIO_SOCK));
	if (ret < 0) {
		debugNetPrintf(ERROR,"[PSP2LINK] disconnect returned error %d\n", ret);
	}
	configuration->psp2link_fileio_sock = -1;
	
}


/*void psp2link_close_fsys(void)
{
	if (psp2LinkGetValue(FILEIO_SOCK) > 0) {
		sceNetSocketClose(psp2LinkGetValue(FILEIO_SOCK));
	}
	configuration->psp2link_fileio_active = 0;
	return;
}*/

static inline int psp2link_send(int sock, void *buf, int len, int flag)
{
	int ret;
	ret = sceNetSend(sock, buf, len, flag);
	if (ret < 0) 
	{
		debugNetPrintf(ERROR,"[PSP2LINK] sceNetSend error %d\n", ret);
		psp2link_close_socket();
		return -1;
	}
	else 
	{
		return ret;
	}
}

 
int psp2link_recv_bytes(int sock, char *buf, int bytes)
{
	int left;
	int len;

	left = bytes;

	while (left > 0) 
	{
		len = sceNetRecv(sock, &buf[bytes - left], left, SCE_NET_MSG_WAITALL);
		if (len < 0) 
		{
			debugNetPrintf(ERROR,"[PSP2LINK] psp2link_recv_bytes error!! 0x%08X\n",len);
			return -1;
		}
		left -= len;
	}
	return bytes;
}


int psp2link_accept_pkt(int sock, char *buf, int len, int pkt_type)
{
	int length;
	psp2link_pkt_hdr *hdr;
	unsigned int hcmd;
	unsigned short hlen;


	length = psp2link_recv_bytes(sock, buf, sizeof(psp2link_pkt_hdr));
	if (length < 0) {
		debugNetPrintf(ERROR,"[PSP2LINK] psp2link_accept_pkt recv error\n");
		return -1;
	}

	if (length < sizeof(psp2link_pkt_hdr)) {
		debugNetPrintf(ERROR,"[PSP2LINK] did not receive a full header!!!! " "Fix this! (%d)\n", length);
	}
    
	hdr = (psp2link_pkt_hdr *)buf;
	hcmd = sceNetNtohl(hdr->cmd);
	hlen = sceNetNtohs(hdr->len);

	if (hcmd != pkt_type) {
		debugNetPrintf(ERROR,"[PSP2LINK] psp2link_accept_pkt: Expected %x, got %x\n",pkt_type, hcmd);
		return 0;
	}

	if (hlen > len) {
		debugNetPrintf(ERROR,"[PSP2LINK] psp2link_accept_pkt: hdr->len is too large!! " "(%d, can only receive %d)\n", hlen, len);
		return 0;
	}

	// get the actual packet data
	length = psp2link_recv_bytes(sock, buf + sizeof(psp2link_pkt_hdr),hlen - sizeof(psp2link_pkt_hdr));

	if (length < 0) {
		debugNetPrintf(ERROR,"[PSP2LINK] accept recv2 error!!\n");
		return 0;
	}

	if (length < (hlen - sizeof(psp2link_pkt_hdr))) {
		debugNetPrintf(ERROR,"[PSP2LINK] psp2link_accept_pkt: Did not receive full packet!!! " ,"Fix this! (%d)\n", length);
	}

	return 1;
}
int psp2LinkIoOpen(const char *file, int flags, SceMode mode)
{
	psp2link_pkt_open_req *openreq;
	psp2link_pkt_file_rly *openrly;

	if (psp2LinkGetValue(FILEIO_SOCK) < 0) {
		debugNetPrintf(DEBUG,"[PSP2LINK] file open req (%s, %x) but psp2link_fileio_sock is not active\n", file, flags);
		
		return -1;
	}

	debugNetPrintf(DEBUG,"[PSP2LINK] file open req (%s, %x %x)\n", file, flags, mode);

	openreq = (psp2link_pkt_open_req *)&send_packet[0];

	// Build packet
	openreq->cmd = sceNetHtonl(PSP2LINK_OPEN_CMD);
	openreq->len = sceNetHtons((unsigned short)sizeof(psp2link_pkt_open_req));
	openreq->flags = sceNetHtonl(flags);
	strncpy(openreq->path, file, PSP2LINK_MAX_PATH);
	openreq->path[PSP2LINK_MAX_PATH - 1] = 0; // Make sure it's null-terminated

	if (psp2link_send(psp2LinkGetValue(FILEIO_SOCK), openreq, sizeof(psp2link_pkt_open_req), SCE_NET_MSG_DONTWAIT) < 0) {
		return -1;
	}

	if (!psp2link_accept_pkt(psp2LinkGetValue(FILEIO_SOCK), recv_packet, sizeof(psp2link_pkt_file_rly), PSP2LINK_OPEN_RLY)) {
		debugNetPrintf(ERROR,"[PSP2LINK] psp2link_file: psp2link_open_file: did not receive OPEN_RLY\n");
		return -1;
	}

	openrly = (psp2link_pkt_file_rly *)recv_packet;
    
	debugNetPrintf(DEBUG,"[PSP2LINK] file open reply received (ret %d)\n", sceNetNtohl(openrly->retval));

	return sceNetNtohl(openrly->retval);	
	
	
}
int psp2LinkIoClose(SceUID fd)
{
	
	psp2link_pkt_close_req *closereq;
	psp2link_pkt_file_rly *closerly;


	if (psp2LinkGetValue(FILEIO_SOCK) < 0) {
		return -1;
	}

	debugNetPrintf(DEBUG,"[PSP2LINK] psp2link_file: file close req (fd: %d)\n", fd);

	closereq = (psp2link_pkt_close_req *)&send_packet[0];
	closerly = (psp2link_pkt_file_rly *)&recv_packet[0];

	closereq->cmd = sceNetHtonl(PSP2LINK_CLOSE_CMD);
	closereq->len = sceNetHtons((unsigned short)sizeof(psp2link_pkt_close_req));
	closereq->fd = sceNetHtonl(fd);

	if (psp2link_send(psp2LinkGetValue(FILEIO_SOCK), closereq, sizeof(psp2link_pkt_close_req), SCE_NET_MSG_DONTWAIT) < 0) {
		return -1;
	}

	if(!psp2link_accept_pkt(psp2LinkGetValue(FILEIO_SOCK), (char *)closerly, sizeof(psp2link_pkt_file_rly), PSP2LINK_CLOSE_RLY)) {
		debugNetPrintf(ERROR,"[PSP2LINK] psp2link_close_file: did not receive PSP2LINK_CLOSE_RLY\n");
		return -1;
	}

	debugNetPrintf(DEBUG,"[PSP2LINK] psp2link_close_file: close reply received (ret %d)\n", sceNetNtohl(closerly->retval));

	return sceNetNtohl(closerly->retval);
	
	
	
}
int psp2LinkIoRead(SceUID fd, void *data, SceSize size)
{
	int nbytes;
	int i;
	psp2link_pkt_read_req *readcmd;
	psp2link_pkt_read_rly *readrly;


	if (psp2LinkGetValue(FILEIO_SOCK) < 0) {
		return -1;
	}

	readcmd = (psp2link_pkt_read_req *)&send_packet[0];
	readrly = (psp2link_pkt_read_rly *)&recv_packet[0];

	readcmd->cmd = sceNetHtonl(PSP2LINK_READ_CMD);
	readcmd->len = sceNetHtons((unsigned short)sizeof(psp2link_pkt_read_req));
	readcmd->fd = sceNetHtonl(fd);


	if (size < 0) {
		debugNetPrintf(ERROR,"[PSP2LINK] psp2link_read_file: illegal req!! (whish to read < 0 bytes!)\n");
		return -1;
	}

	readcmd->nbytes = sceNetHtonl(size);

	i = psp2link_send(psp2LinkGetValue(FILEIO_SOCK), readcmd, sizeof(psp2link_pkt_read_req), SCE_NET_MSG_DONTWAIT);
	if (i<0)
	{
		debugNetPrintf(ERROR,"[PSP2LINK] psp2link_file: psp2link_read_file: send failed (%d)\n", i);
		return -1;
	}

	if(!psp2link_accept_pkt(psp2LinkGetValue(FILEIO_SOCK), (char *)readrly, sizeof(psp2link_pkt_read_rly), PSP2LINK_READ_RLY)) {
		debugNetPrintf(ERROR,"[PSP2LINK] psp2link_read_file: " "did not receive PSP2LINK_READ_RLY\n");
		return -1;
	}

	nbytes = sceNetNtohl(readrly->nbytes);
	debugNetPrintf(DEBUG,"[PSP2LINK] psp2link_read_file: Reply said there's %d bytes to read " "(wanted %d)\n", nbytes, size);

	// Now read the actual file data
	i = psp2link_recv_bytes(psp2LinkGetValue(FILEIO_SOCK), &data[0], nbytes);
	if (i < 0) {
    	debugNetPrintf(ERROR,"[PSP2LINK] psp2link_read_file, data read error\n");
    	return -1;
	}
	return nbytes;
}
int psp2LinkIoWrite(SceUID fd, const void *data, SceSize size)
{
	psp2link_pkt_write_req *writecmd;
	psp2link_pkt_file_rly *writerly;
	int hlen;
	int writtenbytes;
	int nbytes;
	int retval;

	if (psp2LinkGetValue(FILEIO_SOCK) < 0) {
		return -1;
	}

	debugNetPrintf(DEBUG,"[PSP2LINK] file write req (fd: %d)\n", fd);

	writecmd = (psp2link_pkt_write_req *)&send_packet[0];
	writerly = (psp2link_pkt_file_rly *)&recv_packet[0];

	hlen = (unsigned short)sizeof(psp2link_pkt_write_req);
	writecmd->cmd = sceNetHtonl(PSP2LINK_WRITE_CMD);
	writecmd->len = sceNetHtons(hlen);
	writecmd->fd = sceNetHtonl(fd);

	// Divide the write request
	writtenbytes = 0;
	while (writtenbytes < size) {

		if ((size - writtenbytes) > PSP2LINK_MAX_WRITE_SEGMENT) {
			// Need to split in several read reqs
			nbytes = PSP2LINK_MAX_READ_SEGMENT;
        }
		else 
		{
			nbytes = size - writtenbytes;
		}

		writecmd->nbytes = sceNetHtonl(nbytes);
#ifdef ZEROCOPY
		/* Send the packet header.  */
		if (psp2link_send(psp2LinkGetValue(FILEIO_SOCK), writecmd, hlen, SCE_NET_MSG_DONTWAIT) < 0)
			return -1;
		/* Send the write() data.  */
		if (psp2link_send(psp2LinkGetValue(FILEIO_SOCK), &data[writtenbytes], nbytes, SCE_NET_MSG_DONTWAIT) < 0)
			return -1;
#else
		// Copy data to the acutal packet
		memcpy(&send_packet[sizeof(psp2link_pkt_write_req)], &data[writtenbytes],nbytes);

		if (psp2link_send(psp2LinkGetValue(FILEIO_SOCK), writecmd, hlen + nbytes, SCE_NET_MSG_DONTWAIT) < 0)
			return -1;
#endif


		// Get reply
		if(!psp2link_accept_pkt(psp2LinkGetValue(FILEIO_SOCK), (char *)writerly, sizeof(psp2link_pkt_file_rly), PSP2LINK_WRITE_RLY)) {
			debugNetPrintf(ERROR,"[PSP2LINK] psp2link_write_file: " "did not receive PSP2LINK_WRITE_RLY\n");
			return -1;
		}
		retval = sceNetNtohl(writerly->retval);

		debugNetPrintf(DEBUG,"[PSP2LINK] wrote %d bytes (asked for %d)\n", retval, nbytes);

		if (retval < 0) {
			// Error
			debugNetPrintf(ERROR,"[PSP2LINK] psp2link_write_file: received error on write req (%d)\n",retval);
			return retval;
		}

		writtenbytes += retval;
		if (retval < nbytes) {
			// EOF?
			break;
		}
	}
	
	return writtenbytes;	

	
}
int psp2LinkIoLseek(SceUID fd, int offset, int whence)	
{
	psp2link_pkt_lseek_req *lseekreq;
	psp2link_pkt_file_rly *lseekrly;


	if (psp2LinkGetValue(REQUESTS_SOCK) < 0) {
		return -1;
	}

	debugNetPrintf(DEBUG,"[PSP2LINK] file lseek req (fd: %d)\n", fd);

	lseekreq = (psp2link_pkt_lseek_req *)&send_packet[0];
	lseekrly = (psp2link_pkt_file_rly *)&recv_packet[0];

	lseekreq->cmd = sceNetHtonl(PSP2LINK_LSEEK_CMD);
	lseekreq->len = sceNetHtons((unsigned short)sizeof(psp2link_pkt_lseek_req));
	lseekreq->fd = sceNetHtonl(fd);
	lseekreq->offset = sceNetHtonl(offset);
	lseekreq->whence = sceNetHtonl(whence);

	if(psp2link_send(psp2LinkGetValue(FILEIO_SOCK), lseekreq, sizeof(psp2link_pkt_lseek_req), SCE_NET_MSG_DONTWAIT) < 0) {
		return -1;
	}

	if(!psp2link_accept_pkt(psp2LinkGetValue(FILEIO_SOCK), (char *)lseekrly,sizeof(psp2link_pkt_file_rly), PSP2LINK_LSEEK_RLY)) {
		debugNetPrintf(ERROR,"[PSP2LINK] psp2link_lseek_file: did not receive PSP2LINK_LSEEK_RLY\n");
		return -1;
	}

	debugNetPrintf(DEBUG,"[PSP2LINK] psp2link_lseek_file: lseek reply received (ret %d)\n",sceNetNtohl(lseekrly->retval));

	return sceNetNtohl(lseekrly->retval);
	
}
int psp2LinkIoRemove(const char *file)
{
	psp2link_pkt_remove_req *removereq;
	psp2link_pkt_file_rly *removerly;

	if (psp2LinkGetValue(REQUESTS_SOCK) < 0) {
		return -1;
	}

	debugNetPrintf(DEBUG,"[PSP2LINK] file remove req (%s)\n", file);

	removereq = (psp2link_pkt_remove_req *)&send_packet[0];

	// Build packet
	removereq->cmd = sceNetHtonl(PSP2LINK_REMOVE_CMD);
	removereq->len = sceNetHtons((unsigned short)sizeof(psp2link_pkt_remove_req));
	strncpy(removereq->name, file, PSP2LINK_MAX_PATH);
	removereq->name[PSP2LINK_MAX_PATH - 1] = 0; // Make sure it's null-terminated

	if (psp2link_send(psp2LinkGetValue(FILEIO_SOCK), removereq, sizeof(psp2link_pkt_remove_req), SCE_NET_MSG_DONTWAIT) < 0) {
		return -1;
	}

	if (!psp2link_accept_pkt(psp2LinkGetValue(FILEIO_SOCK), recv_packet, sizeof(psp2link_pkt_file_rly), PSP2LINK_REMOVE_RLY)) {
		debugNetPrintf(ERROR,"[PSP2LINK] psp2link_remove: did not receive REMOVE_RLY\n");
		return -1;
	}

	removerly = (psp2link_pkt_file_rly *)recv_packet;
	debugNetPrintf(DEBUG,"[PSP2LINK] file remove reply received (ret %d)\n", sceNetNtohl(removerly->retval));
	return sceNetNtohl(removerly->retval);
}
int psp2LinkIoMkdir(const char *dirname, SceMode mode)
{
	psp2link_pkt_mkdir_req *mkdirreq;
	psp2link_pkt_file_rly *mkdirrly;

	if (psp2LinkGetValue(REQUESTS_SOCK) < 0) {
		return -1;
	}

	debugNetPrintf(DEBUG,"[PSP2LINK] make dir req (%s)\n", dirname);
	
	mkdirreq = (psp2link_pkt_mkdir_req *)&send_packet[0];

	// Build packet
	mkdirreq->cmd = sceNetHtonl(PSP2LINK_MKDIR_CMD);
	mkdirreq->len = sceNetHtons((unsigned short)sizeof(psp2link_pkt_mkdir_req));
	mkdirreq->mode = mode;
	strncpy(mkdirreq->name, dirname, PSP2LINK_MAX_PATH);
	mkdirreq->name[PSP2LINK_MAX_PATH - 1] = 0; // Make sure it's null-terminated

	if (psp2link_send(psp2LinkGetValue(FILEIO_SOCK), mkdirreq, sizeof(psp2link_pkt_mkdir_req), SCE_NET_MSG_DONTWAIT) < 0) {
		return -1;
	}

	if (!psp2link_accept_pkt(psp2LinkGetValue(FILEIO_SOCK), recv_packet, sizeof(psp2link_pkt_file_rly), PSP2LINK_MKDIR_RLY)) {
		debugNetPrintf(ERROR,"[PSP2LINK] psp2link_mkdir: did not receive MKDIR_RLY\n");
		return -1;
	}

	mkdirrly = (psp2link_pkt_file_rly *)recv_packet;
	debugNetPrintf(DEBUG,"[PSP2LINK] psp2link_file: make dir reply received (ret %d)\n", sceNetNtohl(mkdirrly->retval));
	return sceNetNtohl(mkdirrly->retval);
}
int psp2LinkIoRmdir(const char *dirname)
{
	psp2link_pkt_rmdir_req *rmdirreq;
	psp2link_pkt_file_rly *rmdirrly;

	if (psp2LinkGetValue(REQUESTS_SOCK) < 0) {
		return -1;
	}

	debugNetPrintf(DEBUG,"[PSP2LINK] psp2link_file: remove dir req (%s)\n", dirname);

	rmdirreq = (psp2link_pkt_rmdir_req *)&send_packet[0];

	// Build packet
	rmdirreq->cmd = sceNetHtonl(PSP2LINK_RMDIR_CMD);
	rmdirreq->len = sceNetHtons((unsigned short)sizeof(psp2link_pkt_rmdir_req));
	strncpy(rmdirreq->name, dirname, PSP2LINK_MAX_PATH);
	rmdirreq->name[PSP2LINK_MAX_PATH - 1] = 0; // Make sure it's null-terminated

	if (psp2link_send(psp2LinkGetValue(FILEIO_SOCK), rmdirreq, sizeof(psp2link_pkt_rmdir_req), SCE_NET_MSG_DONTWAIT) < 0) {
		return -1;
	}

	if (!psp2link_accept_pkt(psp2LinkGetValue(FILEIO_SOCK), recv_packet, sizeof(psp2link_pkt_file_rly), PSP2LINK_RMDIR_RLY)) {
		debugNetPrintf(ERROR,"[PSP2LINK] psp2link_file: psp2link_rmdir: did not receive RMDIR_RLY\n");
		return -1;
	}

    rmdirrly = (psp2link_pkt_file_rly *)recv_packet;
	debugNetPrintf(DEBUG,"[PSP2LINK] psp2link_file: remove dir reply received (ret %d)\n", sceNetNtohl(rmdirrly->retval));
	return sceNetNtohl(rmdirrly->retval);
}
int psp2LinkIoDopen(const char *dirname)
{
	psp2link_pkt_open_req *openreq;
	psp2link_pkt_file_rly *openrly;

	if (psp2LinkGetValue(REQUESTS_SOCK) < 0) {
		return -1;
	}

	debugNetPrintf(DEBUG,"[PSP2LINK] dir open req (%s)\n", dirname);

	openreq = (psp2link_pkt_open_req *)&send_packet[0];

	// Build packet
	openreq->cmd = sceNetHtonl(PSP2LINK_OPENDIR_CMD);
	openreq->len = sceNetHtons((unsigned short)sizeof(psp2link_pkt_open_req));
 	openreq->flags = sceNetHtonl(0);
	strncpy(openreq->path, dirname, PSP2LINK_MAX_PATH);
	openreq->path[PSP2LINK_MAX_PATH - 1] = 0; // Make sure it's null-terminated

	if (psp2link_send(psp2LinkGetValue(FILEIO_SOCK), openreq, sizeof(psp2link_pkt_open_req), SCE_NET_MSG_DONTWAIT) < 0) {
		return -1;
	}

	if (!psp2link_accept_pkt(psp2LinkGetValue(FILEIO_SOCK), recv_packet, sizeof(psp2link_pkt_file_rly), PSP2LINK_OPENDIR_RLY)) {
		debugNetPrintf(ERROR,"[PSP2LINK] psp2link_open_dir: did not receive OPENDIR_RLY\n");
		return -1;
	}

	openrly = (psp2link_pkt_file_rly *)recv_packet;
    
	debugNetPrintf(DEBUG,"[PSP2LINK] dir open reply received (ret %d)\n", sceNetNtohl(openrly->retval));

	return sceNetNtohl(openrly->retval);
}
int psp2LinkIoDread(SceUID fd, SceIoDirent *dir)
{
	psp2link_pkt_dread_req *dirreq;
	psp2link_pkt_dread_rly *dirrly;
	SceIoDirent *dirent;

	if (psp2LinkGetValue(REQUESTS_SOCK) < 0) {
		return -1;
	}

	debugNetPrintf(DEBUG,"[PSP2LINK] dir read req (%x)\n", fd);

	dirreq = (psp2link_pkt_dread_req *)&send_packet[0];

	// Build packet
	dirreq->cmd = sceNetHtonl(PSP2LINK_READDIR_CMD);
	dirreq->len = sceNetHtons((unsigned short)sizeof(psp2link_pkt_dread_req));
	dirreq->fd = sceNetHtonl(fd);

	if (psp2link_send(psp2LinkGetValue(FILEIO_SOCK), dirreq, sizeof(psp2link_pkt_dread_req), SCE_NET_MSG_DONTWAIT) < 0) {
		return -1;
	}

	if (!psp2link_accept_pkt(psp2LinkGetValue(FILEIO_SOCK), recv_packet, sizeof(psp2link_pkt_dread_rly), PSP2LINK_READDIR_RLY)) {
		debugNetPrintf(ERROR,"[PSP2LINK] psp2link_read_dir: did not receive OPENDIR_RLY\n");
		return -1;
	}

	dirrly = (psp2link_pkt_dread_rly *)recv_packet;
    
	debugNetPrintf(DEBUG,"[PSP2LINK] dir read reply received (ret %d)\n", sceNetNtohl(dirrly->retval));

	
	dirent = (SceIoDirent *) dir;
	// now handle the return buffer translation, to build reply bit
	dirent->d_stat.st_mode    = sceNetNtohl(dirrly->mode);
	dirent->d_stat.st_attr    = sceNetNtohl(dirrly->attr);
	dirent->d_stat.st_size    = sceNetNtohl(dirrly->size);
	dirent->d_stat.st_ctime.microsecond = 0;
	dirent->d_stat.st_ctime.second = sceNetNtohs(dirrly->ctime[1]);
	dirent->d_stat.st_ctime.minute = sceNetNtohs(dirrly->ctime[2]);
	dirent->d_stat.st_ctime.hour = sceNetNtohs(dirrly->ctime[3]);
	dirent->d_stat.st_ctime.day = sceNetNtohs(dirrly->ctime[4]);
	dirent->d_stat.st_ctime.month = sceNetNtohs(dirrly->ctime[5]);
	dirent->d_stat.st_ctime.year = sceNetNtohs(dirrly->ctime[6]);
	
	dirent->d_stat.st_atime.microsecond = 0;
	dirent->d_stat.st_atime.second = sceNetNtohs(dirrly->atime[1]);
	dirent->d_stat.st_atime.minute = sceNetNtohs(dirrly->atime[2]);
	dirent->d_stat.st_atime.hour = sceNetNtohs(dirrly->atime[3]);
	dirent->d_stat.st_atime.day = sceNetNtohs(dirrly->atime[4]);
	dirent->d_stat.st_atime.month = sceNetNtohs(dirrly->atime[5]);
	dirent->d_stat.st_atime.year = sceNetNtohs(dirrly->atime[6]);
	dirent->d_stat.st_mtime.microsecond = 0;
	dirent->d_stat.st_mtime.second = sceNetNtohs(dirrly->mtime[1]);
	dirent->d_stat.st_mtime.minute = sceNetNtohs(dirrly->mtime[2]);
	dirent->d_stat.st_mtime.hour = sceNetNtohs(dirrly->mtime[3]);
	dirent->d_stat.st_mtime.day = sceNetNtohs(dirrly->mtime[4]);
	dirent->d_stat.st_mtime.month = sceNetNtohs(dirrly->mtime[5]);
	dirent->d_stat.st_mtime.year = sceNetNtohs(dirrly->mtime[6]);
	
	
	
	//dirent->stat.hisize  = ntohl(dirrly->hisize);
   
	
	
	strncpy(dirent->d_name,dirrly->name,256);
	dirent->d_private = 0;

	return sceNetNtohl(dirrly->retval);
}
int psp2LinkIoDclose(SceUID fd)	
{
 	psp2link_pkt_close_req *closereq;
	psp2link_pkt_file_rly *closerly;


	if (psp2LinkGetValue(REQUESTS_SOCK) < 0) {
		return -1;
	}

	debugNetPrintf(DEBUG,"[PSP2LINK] psp2link_file: dir close req (fd: %d)\n", fd);

	closereq = (psp2link_pkt_close_req *)&send_packet[0];
	closerly = (psp2link_pkt_file_rly *)&recv_packet[0];

	closereq->cmd = sceNetHtonl(PSP2LINK_CLOSEDIR_CMD);
	closereq->len = sceNetHtons((unsigned short)sizeof(psp2link_pkt_close_req));
	closereq->fd = sceNetHtonl(fd);

	if (psp2link_send(psp2LinkGetValue(FILEIO_SOCK), closereq, sizeof(psp2link_pkt_close_req), SCE_NET_MSG_DONTWAIT) < 0) {
		return -1;
	}

	if(!psp2link_accept_pkt(psp2LinkGetValue(FILEIO_SOCK), (char *)closerly, sizeof(psp2link_pkt_file_rly), PSP2LINK_CLOSEDIR_RLY)) {
		debugNetPrintf(ERROR,"[PSP2LINK] psp2link_close_dir: did not receive PSP2LINK_CLOSEDIR_RLY\n");
		return -1;
	}

	debugNetPrintf(DEBUG,"[PSP2LINK] dir close reply received (ret %d)\n", sceNetNtohl(closerly->retval));

	return sceNetNtohl(closerly->retval);
}
int psp2link_requests_thread(SceSize args, void *argp)
{
	int ret;
	struct SceNetSockaddrIn serveraddr;
	/* Create server socket */
	configuration->psp2link_requests_sock = sceNetSocket("requests_server_sock",SCE_NET_AF_INET,SCE_NET_SOCK_STREAM,0);
	if(psp2LinkGetValue(REQUESTS_SOCK)>=0)
	{
		debugNetPrintf(DEBUG,"[PSP2LINK] Created psp2link_requests_sock: %d\n", psp2LinkGetValue(REQUESTS_SOCK));
		
	}
	memset(&serveraddr, 0, sizeof serveraddr);
  	
	/* Fill the server's address */
	serveraddr.sin_family = SCE_NET_AF_INET;
	serveraddr.sin_addr.s_addr = sceNetHtonl(SCE_NET_INADDR_ANY);
	serveraddr.sin_port = sceNetHtons(psp2LinkGetValue(REQUESTS_PORT));

	/* Bind the server's address to the socket */
	ret = sceNetBind(psp2LinkGetValue(REQUESTS_SOCK), (SceNetSockaddr *)&serveraddr, sizeof(serveraddr));
	if(ret<0)
	{
		debugNetPrintf(ERROR,"[PSP2LINK] sceNetBind error: 0x%08X\n", ret);
		sceNetSocketClose(psp2LinkGetValue(REQUESTS_SOCK));
		return -1;
	}
	/* Start listening */
	ret = sceNetListen(psp2LinkGetValue(REQUESTS_SOCK), 5);
	if(ret<0)
	{
		debugNetPrintf(ERROR,"[PSP2LINK] sceNetListen error: 0x%08X\n", ret);
		sceNetSocketClose(psp2LinkGetValue(REQUESTS_SOCK));
		return -1;
	}
	
	
   
   
	while(psp2LinkGetValue(FILEIO_ACTIVE))
	{
		debugNetPrintf(INFO,"[PSP2LINK] Waiting for connection\n", ret);
		
		/* Accept clients */
		SceNetSockaddrIn clientaddr;
		int client_sock;
		unsigned int addrlen = sizeof(clientaddr);
		client_sock = sceNetAccept(psp2LinkGetValue(REQUESTS_SOCK), (SceNetSockaddr *)&clientaddr, &addrlen);
		if (client_sock < 0) {
			debugNetPrintf(ERROR,"[PSP2LINK] sceNetAccept error (%d)", client_sock);
		    continue;
		}
		
		/* Get the client's IP address */
		remote_pc_addr = clientaddr.sin_addr.s_addr;
		char remote_ip[16];
		sceNetInetNtop(SCE_NET_AF_INET,&clientaddr.sin_addr.s_addr,remote_ip,sizeof(remote_ip));
		debugNetPrintf(INFO,"[PSP2LINK] Client connected from %s port: %i\n ",remote_ip, clientaddr.sin_port);			
		if (psp2LinkGetValue(REQUESTS_SOCK) > 0) {
			debugNetPrintf(ERROR,"[PSP2LINK] Client reconnected\n");
			// sceNetSocketClose(psp2LinkGetValue(REQUESTS_SOCK));
		}
		
		configuration->psp2link_fileio_sock = client_sock;	
		psp2link_requests_connected=1;
		
		debugNetPrintf(INFO,"[PSP2LINK] sock psp2link_fileio set %d connected %d\n",psp2LinkGetValue(FILEIO_SOCK),psp2link_requests_connected);
		
		
	}
	debugNetPrintf(INFO,"[PSP2LINK] exit thread requests\n");
	if(psp2LinkGetValue(FILEIO_SOCK))
	{
		debugNetPrintf(DEBUG,"[PSP2LINK] closing fileio_sock\n");
		sceNetSocketClose(psp2LinkGetValue(FILEIO_SOCK));
		configuration->psp2link_fileio_sock=-1;
	}
	if(psp2LinkGetValue(REQUESTS_SOCK))
	{
		debugNetPrintf(DEBUG,"[PSP2LINK] closing server_request_sock\n");
		sceNetSocketClose(psp2LinkGetValue(REQUESTS_SOCK));
		configuration->psp2link_requests_sock=-1;
	}
	
	
	psp2link_requests_connected=0;
	
	
	sceKernelExitDeleteThread(0);
	return 0;
}
