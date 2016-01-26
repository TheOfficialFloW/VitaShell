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

#ifndef _PSP2LINK_INTERNAL_H_
#define _PSP2LINK_INTERNAL_H_

#define PSP2LINK_OPEN_CMD     0xbabe0111
#define PSP2LINK_OPEN_RLY     0xbabe0112
#define PSP2LINK_CLOSE_CMD    0xbabe0121
#define PSP2LINK_CLOSE_RLY    0xbabe0122
#define PSP2LINK_READ_CMD     0xbabe0131
#define PSP2LINK_READ_RLY     0xbabe0132
#define PSP2LINK_WRITE_CMD    0xbabe0141
#define PSP2LINK_WRITE_RLY    0xbabe0142
#define PSP2LINK_LSEEK_CMD    0xbabe0151
#define PSP2LINK_LSEEK_RLY    0xbabe0152
#define PSP2LINK_OPENDIR_CMD  0xbabe0161
#define PSP2LINK_OPENDIR_RLY  0xbabe0162
#define PSP2LINK_CLOSEDIR_CMD 0xbabe0171
#define PSP2LINK_CLOSEDIR_RLY 0xbabe0172
#define PSP2LINK_READDIR_CMD  0xbabe0181
#define PSP2LINK_READDIR_RLY  0xbabe0182
#define PSP2LINK_REMOVE_CMD   0xbabe0191
#define PSP2LINK_REMOVE_RLY   0xbabe0192
#define PSP2LINK_MKDIR_CMD    0xbabe01a1
#define PSP2LINK_MKDIR_RLY    0xbabe01a2
#define PSP2LINK_RMDIR_CMD    0xbabe01b1
#define PSP2LINK_RMDIR_RLY    0xbabe01b2



#define PSP2LINK_MAX_PATH   256


typedef struct
{
    unsigned int cmd;
    unsigned short len;
} __attribute__((packed)) psp2link_pkt_hdr;

typedef struct
{
    unsigned int cmd;
    unsigned short len;
    int retval;
} __attribute__((packed)) psp2link_pkt_file_rly;

typedef struct
{
    unsigned int cmd;
    unsigned short len;
    int flags;
    char path[PSP2LINK_MAX_PATH];
} __attribute__((packed)) psp2link_pkt_open_req;

typedef struct
{
    unsigned int cmd;
    unsigned short len;
    int fd;
} __attribute__((packed)) psp2link_pkt_close_req;

typedef struct
{
    unsigned int cmd;
    unsigned short len;
    int fd;
    int nbytes;
} __attribute__((packed)) psp2link_pkt_read_req;

typedef struct
{
    unsigned int cmd;
    unsigned short len;
    int retval;
    int nbytes;
} __attribute__((packed)) psp2link_pkt_read_rly;

typedef struct
{
    unsigned int cmd;
    unsigned short len;
    int fd;
    int nbytes;
} __attribute__((packed)) psp2link_pkt_write_req;

typedef struct
{
    unsigned int cmd;
    unsigned short len;
    int fd;
	int offset;
    int whence;
} __attribute__((packed)) psp2link_pkt_lseek_req;

typedef struct
{
    unsigned int cmd;
    unsigned short len;
    char name[PSP2LINK_MAX_PATH];
} __attribute__((packed)) psp2link_pkt_remove_req;

typedef struct
{
    unsigned int cmd;
    unsigned short len;
    int mode;
    char name[PSP2LINK_MAX_PATH];
} __attribute__((packed)) psp2link_pkt_mkdir_req;

typedef struct
{
    unsigned int cmd;
    unsigned short len;
    char name[PSP2LINK_MAX_PATH];
} __attribute__((packed)) psp2link_pkt_rmdir_req;

typedef struct
{
    unsigned int cmd;
    unsigned short len;
    int fd;
} __attribute__((packed)) psp2link_pkt_dread_req;

typedef struct
{
    unsigned int cmd;
    unsigned short len;
    int retval;
	unsigned int mode;
	unsigned int attr;
	unsigned int size;
	unsigned short ctime[8];
	unsigned short atime[8];
	unsigned short mtime[8];
    char name[256];
} __attribute__((packed)) psp2link_pkt_dread_rly;

#define PSP2LINK_EXECELF_CMD 0xbabe0201
#define	PSP2LINK_EXECSPRX_CMD 0xbabe0202
#define	PSP2LINK_EXIT_CMD 0xbabe0203


typedef struct
{
    unsigned int cmd;
    unsigned short len;
    int  argc;
    char argv[PSP2LINK_MAX_PATH];
} __attribute__((packed)) psp2link_pkt_exec_cmd;


#define PSP2LINK_MAX_WRITE_SEGMENT (65535 - sizeof(psp2link_pkt_write_req))  //1460
#define PSP2LINK_MAX_READ_SEGMENT  (65535 - sizeof(psp2link_pkt_read_rly)) //1460

int psp2link_requests_thread(SceSize args, void *argp);
int psp2link_commands_thread(SceSize args, void *argp);

#endif
