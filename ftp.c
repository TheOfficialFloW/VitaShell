/*
 * Copyright (c) 2015 Sergi Granell (xerpi)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syslimits.h>

#include <psp2/kernel/threadmgr.h>

#include <psp2/io/fcntl.h>
#include <psp2/io/dirent.h>

#include <psp2/net/net.h>
#include <psp2/net/netctl.h>

#include "main.h"
#include "file.h"

#define UNUSED(x) (void)(x)

#define FTP_PORT 1337
#define NET_INIT_SIZE 1*1024*1024
#define FILE_BUF_SIZE 4*1024*1024

#define FTP_DEFAULT_PREFIX HOME_PATH
#define FTP_DEFAULT_PATH   "/"
#define HOME_PREFIX_LENGTH 5

#define INFO(...)
#define DEBUG(...)

typedef enum {
	FTP_DATA_CONNECTION_NONE,
	FTP_DATA_CONNECTION_ACTIVE,
	FTP_DATA_CONNECTION_PASSIVE,
} DataConnectionType;

typedef struct ClientInfo {
	/* Client number */
	int num;
	/* Thread UID */
	SceUID thid;
	/* Control connection socket FD */
	int ctrl_sockfd;
	/* Data connection attributes */
	int data_sockfd;
	DataConnectionType data_con_type;
	SceNetSockaddrIn data_sockaddr;
	/* PASV mode client socket */
	SceNetSockaddrIn pasv_sockaddr;
	int pasv_sockfd;
	/* Remote client net info */
	SceNetSockaddrIn addr;
	/* Receive buffer attributes */
	int n_recv;
	char recv_buffer[512];
	/* Current working directory */
	char cur_path[PATH_MAX];
	/* Rename path */
	char rename_path[PATH_MAX];
	/* Client list */
	struct ClientInfo *next;
	struct ClientInfo *prev;
} ClientInfo;

typedef void (*cmd_dispatch_func)(ClientInfo *client);

typedef struct {
	const char *cmd;
	cmd_dispatch_func func;
} cmd_dispatch_entry;

static void *net_memory = NULL;
static int ftp_initialized = 0;
static SceNetInAddr vita_addr;
static SceUID server_thid;
static int server_sockfd;
static int number_clients = 0;
static ClientInfo *client_list = NULL;
static SceUID client_list_mtx;

#define client_send_ctrl_msg(cl, str) \
	sceNetSend(cl->ctrl_sockfd, str, strlen(str), 0)

static inline void client_send_data_msg(ClientInfo *client, const char *str)
{
	if (client->data_con_type == FTP_DATA_CONNECTION_ACTIVE) {
		sceNetSend(client->data_sockfd, str, strlen(str), 0);
	} else {
		sceNetSend(client->pasv_sockfd, str, strlen(str), 0);
	}
}

static inline int client_send_recv_raw(ClientInfo *client, void *buf, unsigned int len)
{
	if (client->data_con_type == FTP_DATA_CONNECTION_ACTIVE) {
		return sceNetRecv(client->data_sockfd, buf, len, 0);
	} else {
		return sceNetRecv(client->pasv_sockfd, buf, len, 0);
	}
}

static inline void client_send_data_raw(ClientInfo *client, const void *buf, unsigned int len)
{
	if (client->data_con_type == FTP_DATA_CONNECTION_ACTIVE) {
		sceNetSend(client->data_sockfd, buf, len, 0);
	} else {
		sceNetSend(client->pasv_sockfd, buf, len, 0);
	}
}

static void cmd_USER_func(ClientInfo *client)
{
	client_send_ctrl_msg(client, "331 Username OK, need password b0ss.\n");
}

static void cmd_PASS_func(ClientInfo *client)
{
	client_send_ctrl_msg(client, "230 User logged in!\n");
}

static void cmd_QUIT_func(ClientInfo *client)
{
	client_send_ctrl_msg(client, "221 Goodbye senpai :'(\n");
}

static void cmd_SYST_func(ClientInfo *client)
{
	client_send_ctrl_msg(client, "215 UNIX Type: L8\n");
}

static void cmd_PASV_func(ClientInfo *client)
{
	int ret;
	UNUSED(ret);

	char cmd[512];
	unsigned int namelen;
	SceNetSockaddrIn picked;

	/* Create data mode socket name */
	char data_socket_name[64];
	sprintf(data_socket_name, "FTPVita_client_%i_data_socket",
		client->num);

	/* Create the data socket */
	client->data_sockfd = sceNetSocket(data_socket_name,
		SCE_NET_AF_INET,
		SCE_NET_SOCK_STREAM,
		0);

	DEBUG("PASV data socket fd: %d\n", client->data_sockfd);

	/* Fill the data socket address */
	client->data_sockaddr.sin_family = SCE_NET_AF_INET;
	client->data_sockaddr.sin_addr.s_addr = sceNetHtonl(SCE_NET_INADDR_ANY);
	/* Let the PSVita choose a port */
	client->data_sockaddr.sin_port = sceNetHtons(0);

	/* Bind the data socket address to the data socket */
	ret = sceNetBind(client->data_sockfd,
		(SceNetSockaddr *)&client->data_sockaddr,
		sizeof(client->data_sockaddr));
	DEBUG("sceNetBind(): 0x%08X\n", ret);

	/* Start listening */
	ret = sceNetListen(client->data_sockfd, 128);
	DEBUG("sceNetListen(): 0x%08X\n", ret);

	/* Get the port that the PSVita has chosen */
	namelen = sizeof(picked);
	sceNetGetsockname(client->data_sockfd, (SceNetSockaddr *)&picked,
		&namelen);

	DEBUG("PASV mode port: 0x%04X\n", picked.sin_port);

	/* Build the command */
	sprintf(cmd, "227 Entering Passive Mode (%hhu,%hhu,%hhu,%hhu,%hhu,%hhu)\n",
		(vita_addr.s_addr >> 0) & 0xFF,
		(vita_addr.s_addr >> 8) & 0xFF,
		(vita_addr.s_addr >> 16) & 0xFF,
		(vita_addr.s_addr >> 24) & 0xFF,
		(picked.sin_port >> 0) & 0xFF,
		(picked.sin_port >> 8) & 0xFF);

	client_send_ctrl_msg(client, cmd);

	/* Set the data connection type to passive! */
	client->data_con_type = FTP_DATA_CONNECTION_PASSIVE;
}

static void cmd_PORT_func(ClientInfo *client)
{
	unsigned char data_ip[4];
	unsigned char porthi, portlo;
	unsigned short data_port;
	char ip_str[16];
	SceNetInAddr data_addr;

	sscanf(client->recv_buffer, "%*s %hhu,%hhu,%hhu,%hhu,%hhu,%hhu",
		&data_ip[0], &data_ip[1], &data_ip[2], &data_ip[3],
		&porthi, &portlo);

	data_port = portlo + porthi*256;

	/* Convert to an X.X.X.X IP string */
	sprintf(ip_str, "%d.%d.%d.%d",
		data_ip[0], data_ip[1], data_ip[2], data_ip[3]);

	/* Convert the IP to a SceNetInAddr */
	sceNetInetPton(SCE_NET_AF_INET, ip_str, &data_addr);

	DEBUG("PORT connection to client's IP: %s Port: %d\n", ip_str, data_port);

	/* Create data mode socket name */
	char data_socket_name[64];
	sprintf(data_socket_name, "FTPVita_client_%i_data_socket",
		client->num);

	/* Create data mode socket */
	client->data_sockfd = sceNetSocket(data_socket_name,
		SCE_NET_AF_INET,
		SCE_NET_SOCK_STREAM,
		0);

	DEBUG("Client %i data socket fd: %d\n", client->num,
		client->data_sockfd);

	/* Prepare socket address for the data connection */
	client->data_sockaddr.sin_family = SCE_NET_AF_INET;
	client->data_sockaddr.sin_addr = data_addr;
	client->data_sockaddr.sin_port = sceNetHtons(data_port);

	/* Set the data connection type to active! */
	client->data_con_type = FTP_DATA_CONNECTION_ACTIVE;

	client_send_ctrl_msg(client, "200 PORT command successful!\n");
}

static void client_open_data_connection(ClientInfo *client)
{
	int ret;
	UNUSED(ret);

	unsigned int addrlen;

	if (client->data_con_type == FTP_DATA_CONNECTION_ACTIVE) {
		/* Connect to the client using the data socket */
		ret = sceNetConnect(client->data_sockfd,
			(SceNetSockaddr *)&client->data_sockaddr,
			sizeof(client->data_sockaddr));

		DEBUG("sceNetConnect(): 0x%08X\n", ret);
	} else {
		/* Listen to the client using the data socket */
		addrlen = sizeof(client->pasv_sockaddr);
		client->pasv_sockfd = sceNetAccept(client->data_sockfd,
			(SceNetSockaddr *)&client->pasv_sockaddr,
			&addrlen);
		DEBUG("PASV client fd: 0x%08X\n", client->pasv_sockfd);
	}
}

static void client_close_data_connection(ClientInfo *client)
{
	sceNetSocketClose(client->data_sockfd);
	/* In passive mode we have to close the client pasv socket too */
	if (client->data_con_type == FTP_DATA_CONNECTION_PASSIVE) {
		sceNetSocketClose(client->pasv_sockfd);
	}
	client->data_con_type = FTP_DATA_CONNECTION_NONE;
}

static int gen_list_format(char *out, int n, int dir, unsigned int file_size,
	int month_n, int day_n, int hour, int minute, const char *filename)
{
	static const char num_to_month[][4] = {
		"Jan", "Feb", "Mar", "Apr", "May", "Jun",
		"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
	};

	return snprintf(out, n,
		"%c%s 1 vita vita %d %s %-2d %02d:%02d %s\r\n",
		dir ? 'd' : '-',
		dir ? "rwxr-xr-x" : "rw-r--r--",
		file_size,
		num_to_month[(month_n-1)%12],
		day_n,
		hour,
		minute,
		filename);
}

static void send_LIST(ClientInfo *client, const char *path)
{
	char buffer[512];

	if (strcmp(path, HOME_PATH"/") == 0) {
		client_send_ctrl_msg(client, "150 Opening ASCII mode data transfer for LIST.\n");

		client_open_data_connection(client);

		int i;
		for (i = 0; i < getNumberMountPoints(); i++) {
			char **mount_points = getMountPoints();
			if (mount_points[i]) {
				char *name = mount_points[i];

				SceIoStat stat;
				memset(&stat, 0, sizeof(SceIoStat));
				sceIoGetstat(name, &stat);

				gen_list_format(buffer, sizeof(buffer),
					1,
					stat.st_size,
					stat.st_mtime.month,
					stat.st_mtime.day,
					stat.st_mtime.hour,
					stat.st_mtime.minute,
					name);

				client_send_data_msg(client, buffer);
				memset(buffer, 0, sizeof(buffer));
			}
		}
	} else {
		SceUID dir;
		SceIoDirent dirent;

		char vita_path[PATH_MAX];
		strcpy(vita_path, path + HOME_PREFIX_LENGTH);
		dir = sceIoDopen(vita_path);
		if (dir < 0) {
			client_send_ctrl_msg(client, "550 Invalid directory.\n");
			return;
		}

		client_send_ctrl_msg(client, "150 Opening ASCII mode data transfer for LIST.\n");

		client_open_data_connection(client);

		memset(&dirent, 0, sizeof(dirent));

		while (sceIoDread(dir, &dirent) > 0) {
			gen_list_format(buffer, sizeof(buffer),
				SCE_S_ISDIR(dirent.d_stat.st_mode),
				dirent.d_stat.st_size,
				dirent.d_stat.st_mtime.month,
				dirent.d_stat.st_mtime.day,
				dirent.d_stat.st_mtime.hour,
				dirent.d_stat.st_mtime.minute,
				dirent.d_name);

			client_send_data_msg(client, buffer);
			memset(&dirent, 0, sizeof(dirent));
			memset(buffer, 0, sizeof(buffer));
		}

		sceIoDclose(dir);
	}

	DEBUG("Done sending LIST\n");

	client_close_data_connection(client);
	client_send_ctrl_msg(client, "226 Transfer complete.\n");
}

static void cmd_LIST_func(ClientInfo *client)
{
	char list_path[PATH_MAX];

	int n = sscanf(client->recv_buffer, "%*s %[^\r\n	]", list_path);

	if (n > 0) {  /* Client specified a path */
		send_LIST(client, list_path);
	} else {	  /* Use current path */
		send_LIST(client, client->cur_path);
	}
}

static void cmd_PWD_func(ClientInfo *client)
{
	char msg[PATH_MAX];
	/* We don't want to send the prefix */
	const char *pwd_path = strchr(client->cur_path, '/');

	sprintf(msg, "257 \"%s\" is the current directory.\n", pwd_path);
	client_send_ctrl_msg(client, msg);
}

static void cmd_CWD_func(ClientInfo *client)
{
	char cmd_path[PATH_MAX];
	char path[PATH_MAX];
	SceUID pd;
	int n = sscanf(client->recv_buffer, "%*s %[^\r\n	]", cmd_path);

	if (n < 1) {
		client_send_ctrl_msg(client, "500 Syntax error, command unrecognized.\n");
	} else {
		if (cmd_path[0] != '/') { /* Change dir relative to current dir */
			sprintf(path, "%s%s", client->cur_path, cmd_path);
		} else {
			sprintf(path, "%s%s", FTP_DEFAULT_PREFIX, cmd_path);
		}

		/* If there isn't "/" at the end, add it */
		if (path[strlen(path) - 1] != '/') {
			strcat(path, "/");
		}

		/* Check if the path exists */
		char vita_path[PATH_MAX];
		strcpy(vita_path, path + HOME_PREFIX_LENGTH);
		pd = sceIoDopen(vita_path);
		if (pd < 0) {
			client_send_ctrl_msg(client, "550 Invalid directory.\n");
			return;
		}
		sceIoDclose(pd);

		strcpy(client->cur_path, path);
		client_send_ctrl_msg(client, "250 Requested file action okay, completed.\n");
	}
}

static void cmd_TYPE_func(ClientInfo *client)
{
	char data_type;
	char format_control[8];
	int n_args = sscanf(client->recv_buffer, "%*s %c %s", &data_type, format_control);

	if (n_args > 0) {
		switch(data_type) {
		case 'A':
		case 'I':
			client_send_ctrl_msg(client, "200 Okay\n");
			break;
		case 'E':
		case 'L':
		default:
			client_send_ctrl_msg(client, "504 Error: bad parameters?\n");
			break;
		}
	} else {
		client_send_ctrl_msg(client, "504 Error: bad parameters?\n");
	}
}

static void dir_up(char *path)
{
	char *pch;
	size_t len_in = strlen(path);
	if (len_in == 1) {
		strcpy(path, "/");
		return;
	}
	if (path[len_in - 1] == '/') {
		path[len_in - 1] = '\0';
	}
	pch = strrchr(path, '/');
	if (pch) {
		size_t s = len_in - (pch - path);
		memset(pch + 1, '\0', s);
	}
}

static void cmd_CDUP_func(ClientInfo *client)
{
	char path[PATH_MAX];
	/* Path without the prefix */
	const char *normal_path = strchr(client->cur_path, '/');
	strcpy(path, normal_path);
	dir_up(path);
	sprintf(client->cur_path, "%s%s", FTP_DEFAULT_PREFIX, path);
	client_send_ctrl_msg(client, "200 Command okay.\n");
}

static void send_file(ClientInfo *client, const char *path)
{
	unsigned char *buffer;
	SceUID fd;
	unsigned int bytes_read;

	DEBUG("Opening: %s\n", path);

	if ((fd = sceIoOpen(path, SCE_O_RDONLY, 0777)) >= 0) {

		buffer = malloc(FILE_BUF_SIZE);
		if (buffer == NULL) {
			client_send_ctrl_msg(client, "550 Could not allocate memory.\n");
			return;
		}

		client_open_data_connection(client);
		client_send_ctrl_msg(client, "150 Opening Image mode data transfer.\n");

		while ((bytes_read = sceIoRead (fd, buffer, FILE_BUF_SIZE)) > 0) {
			client_send_data_raw(client, buffer, bytes_read);
		}

		sceIoClose(fd);
		free(buffer);
		client_send_ctrl_msg(client, "226 Transfer completed.\n");
		client_close_data_connection(client);

	} else {
		client_send_ctrl_msg(client, "550 File not found.\n");
	}
}

/* This function generates a PSVita valid path with the input path
 * from RETR, STOR, DELE, RMD, MKD, RNFR and RNTO commands */
static void gen_filepath(ClientInfo *client, char *vita_path)
{
	char path[PATH_MAX];
	char cmd_path[PATH_MAX];
	sscanf(client->recv_buffer, "%*[^ ] %[^\r\n	]", cmd_path);

	if (cmd_path[0] != '/') { /* The file is relative to current dir */
		/* Append the file to the current path */
		sprintf(path, "%s%s", client->cur_path, cmd_path);
	} else {
		/* Add the prefix to the file */
		sprintf(path, "%s%s", FTP_DEFAULT_PREFIX, cmd_path);
	}

	strcpy(vita_path, path + HOME_PREFIX_LENGTH);
}

static void cmd_RETR_func(ClientInfo *client)
{
	char dest_path[PATH_MAX];
	gen_filepath(client, dest_path);
	send_file(client, dest_path);
}

static void receive_file(ClientInfo *client, const char *path)
{
	unsigned char *buffer;
	SceUID fd;
	unsigned int bytes_recv;

	DEBUG("Opening: %s\n", path);

	if ((fd = sceIoOpen(path, SCE_O_CREAT | SCE_O_WRONLY | SCE_O_TRUNC, 0777)) >= 0) {

		buffer = malloc(FILE_BUF_SIZE);
		if (buffer == NULL) {
			client_send_ctrl_msg(client, "550 Could not allocate memory.\n");
			return;
		}

		client_open_data_connection(client);
		client_send_ctrl_msg(client, "150 Opening Image mode data transfer.\n");

		while ((bytes_recv = client_send_recv_raw(client, buffer, FILE_BUF_SIZE)) > 0) {
			sceIoWrite(fd, buffer, bytes_recv);
		}

		sceIoClose(fd);
		free(buffer);
		client_send_ctrl_msg(client, "226 Transfer completed.\n");
		client_close_data_connection(client);

	} else {
		client_send_ctrl_msg(client, "550 File not found.\n");
	}
}

static void cmd_STOR_func(ClientInfo *client)
{
	char dest_path[PATH_MAX];
	gen_filepath(client, dest_path);
	receive_file(client, dest_path);
}

static void delete_file(ClientInfo *client, const char *path)
{
	DEBUG("Deleting: %s\n", path);

	if (sceIoRemove(path) >= 0) {
		client_send_ctrl_msg(client, "226 File deleted.\n");
	} else {
		client_send_ctrl_msg(client, "550 Could not delete the file.\n");
	}
}

static void cmd_DELE_func(ClientInfo *client)
{
	char dest_path[PATH_MAX];
	gen_filepath(client, dest_path);
	delete_file(client, dest_path);
}

static void delete_dir(ClientInfo *client, const char *path)
{
	int ret;
	DEBUG("Deleting: %s\n", path);
	ret = sceIoRmdir(path);
	if (ret >= 0) {
		client_send_ctrl_msg(client, "226 Directory deleted.\n");
	} else if (ret == 0x8001005A) { /* DIRECTORY_IS_NOT_EMPTY */
		client_send_ctrl_msg(client, "550 Directory is not empty.\n");
	} else {
		client_send_ctrl_msg(client, "550 Could not delete the directory.\n");
	}
}

static void cmd_RMD_func(ClientInfo *client)
{
	char dest_path[PATH_MAX];
	gen_filepath(client, dest_path);
	delete_dir(client, dest_path);
}

static void create_dir(ClientInfo *client, const char *path)
{
	DEBUG("Creating: %s\n", path);

	if (sceIoMkdir(path, 0777) >= 0) {
		client_send_ctrl_msg(client, "226 Directory created.\n");
	} else {
		client_send_ctrl_msg(client, "550 Could not create the directory.\n");
	}
}

static void cmd_MKD_func(ClientInfo *client)
{
	char dest_path[PATH_MAX];
	gen_filepath(client, dest_path);
	create_dir(client, dest_path);
}

static int file_exists(const char *path)
{
	SceIoStat stat;
	return (sceIoGetstat(path, &stat) >= 0);
}

static void cmd_RNFR_func(ClientInfo *client)
{
	char from_path[PATH_MAX];
	/* Get the origin filename */
	gen_filepath(client, from_path);

	/* Check if the file exists */
	if (!file_exists(from_path)) {
		client_send_ctrl_msg(client, "550 The file doesn't exist.\n");
		return;
	}
	/* The file to be renamed is the received path */
	strcpy(client->rename_path, from_path);
	client_send_ctrl_msg(client, "250 I need the destination name b0ss.\n");
}

static void cmd_RNTO_func(ClientInfo *client)
{
	char path_to[PATH_MAX];
	/* Get the destination filename */
	gen_filepath(client, path_to);

	DEBUG("Renaming: %s to %s\n", client->rename_path, path_to);

	if (sceIoRename(client->rename_path, path_to) < 0) {
		client_send_ctrl_msg(client, "550 Error renaming the file.\n");
	}

	client_send_ctrl_msg(client, "226 Rename completed.\n");
}

static void cmd_SIZE_func(ClientInfo *client)
{
	SceIoStat stat;
	char path[PATH_MAX];
	char cmd[64];
	/* Get the filename to retrieve its size */
	gen_filepath(client, path);

	/* Check if the file exists */
	if (sceIoGetstat(path, &stat) < 0) {
		client_send_ctrl_msg(client, "550 The file doesn't exist.\n");
		return;
	}
	/* Send the size of the file */
	sprintf(cmd, "213: %lld\n", stat.st_size);
	client_send_ctrl_msg(client, cmd);
}

#define add_entry(name) {#name, cmd_##name##_func}
static const cmd_dispatch_entry cmd_dispatch_table[] = {
	add_entry(USER),
	add_entry(PASS),
	add_entry(QUIT),
	add_entry(SYST),
	add_entry(PASV),
	add_entry(PORT),
	add_entry(LIST),
	add_entry(PWD),
	add_entry(CWD),
	add_entry(TYPE),
	add_entry(CDUP),
	add_entry(RETR),
	add_entry(STOR),
	add_entry(DELE),
	add_entry(RMD),
	add_entry(MKD),
	add_entry(RNFR),
	add_entry(RNTO),
	add_entry(SIZE),
	{NULL, NULL}
};

static cmd_dispatch_func get_dispatch_func(const char *cmd)
{
	int i;
	for(i = 0; cmd_dispatch_table[i].cmd && cmd_dispatch_table[i].func; i++) {
		if (strcmp(cmd, cmd_dispatch_table[i].cmd) == 0) {
			return cmd_dispatch_table[i].func;
		}
	}
	return NULL;
}

static void client_list_add(ClientInfo *client)
{
	/* Add the client at the front of the client list */
	sceKernelLockMutex(client_list_mtx, 1, NULL);

	if (client_list == NULL) { /* List is empty */
		client_list = client;
		client->prev = NULL;
		client->next = NULL;
	} else {
		client->next = client_list;
		client->next->prev = client;
		client->prev = NULL;
		client_list = client;
	}

	sceKernelUnlockMutex(client_list_mtx, 1);
}

static void client_list_delete(ClientInfo *client)
{
	/* Remove the client from the client list */
	sceKernelLockMutex(client_list_mtx, 1, NULL);

	if (client->prev) {
		client->prev->next = client->next;
	}
	if (client->next) {
		client->next->prev = client->prev;
	}

	sceKernelUnlockMutex(client_list_mtx, 1);
}

static void client_list_thread_end()
{
	ClientInfo *it, *next;
	SceUID client_thid;

	sceKernelLockMutex(client_list_mtx, 1, NULL);

	it = client_list;

	/* Iterate over the client list and close their sockets */
	while (it) {
		next = it->next;
		client_thid = it->thid;

		/* Shutdown the client's control socket */
		sceNetShutdown(it->ctrl_sockfd, SCE_NET_SHUT_RDWR);

		/* Wait until the client threads ends */
		sceKernelWaitThreadEnd(client_thid, NULL, NULL);

		it = next;
	}

	sceKernelUnlockMutex(client_list_mtx, 1);
}

static int client_thread(SceSize args, void *argp)
{
	char cmd[16];
	cmd_dispatch_func dispatch_func;
	ClientInfo *client = (ClientInfo *)argp;

	DEBUG("Client thread %i started!\n", client->num);

	client_send_ctrl_msg(client, "220 FTPVita Server ready.\n");

	while (1) {

		memset(client->recv_buffer, 0, sizeof(client->recv_buffer));

		client->n_recv = sceNetRecv(client->ctrl_sockfd, client->recv_buffer, sizeof(client->recv_buffer), 0);
		if (client->n_recv > 0) {
			DEBUG("Received %i bytes from client number %i:\n",
				client->n_recv, client->num);

			INFO("	%i> %s", client->num, client->recv_buffer);

			/* The command are the first chars until the first space */
			sscanf(client->recv_buffer, "%s", cmd);

			/* Wait 1 ms before sending any data */
			sceKernelDelayThread(1*1000);

			if ((dispatch_func = get_dispatch_func(cmd))) {
				dispatch_func(client);
			} else {
				client_send_ctrl_msg(client, "502 Sorry, command not implemented. :(\n");
			}

		} else if (client->n_recv == 0) {
			/* Value 0 means connection closed by the remote peer */
			INFO("Connection closed by the client %i.\n", client->num);
			/* Delete itself from the client list */
			client_list_delete(client);
			break;
		} else if (client->n_recv == SCE_NET_ERROR_ECANCELED) {
			/* SCE_NET_ERROR_ECANCELED means socket shutdown */
			DEBUG("Client %i socket shutdown\n", client->num);
			break;
		} else {
			/* Other errors */
			INFO("Socket error client %i.\n", client->num);
			client_list_delete(client);
			break;
		}
	}

	/* Close the client's socket */
	sceNetSocketClose(client->ctrl_sockfd);

	/* If there's an open data connection, close it */
	if (client->data_con_type != FTP_DATA_CONNECTION_NONE) {
		sceNetSocketClose(client->data_sockfd);
		if (client->data_con_type == FTP_DATA_CONNECTION_PASSIVE) {
			sceNetSocketClose(client->pasv_sockfd);
		}
	}

	DEBUG("Client thread %i exiting!\n", client->num);

	/* Temporary newlib thread malloc bug fix */
	// free(client);

	sceKernelExitDeleteThread(0);
	return 0;
}

static int server_thread(SceSize args, void *argp)
{
	int ret;
	UNUSED(ret);

	SceNetSockaddrIn serveraddr;

	DEBUG("Server thread started!\n");

	/* Create server socket */
	server_sockfd = sceNetSocket("FTPVita_server_sock",
		SCE_NET_AF_INET,
		SCE_NET_SOCK_STREAM,
		0);

	DEBUG("Server socket fd: %d\n", server_sockfd);

	/* Fill the server's address */
	serveraddr.sin_family = SCE_NET_AF_INET;
	serveraddr.sin_addr.s_addr = sceNetHtonl(SCE_NET_INADDR_ANY);
	serveraddr.sin_port = sceNetHtons(FTP_PORT);

	/* Bind the server's address to the socket */
	ret = sceNetBind(server_sockfd, (SceNetSockaddr *)&serveraddr, sizeof(serveraddr));
	DEBUG("sceNetBind(): 0x%08X\n", ret);

	/* Start listening */
	ret = sceNetListen(server_sockfd, 128);
	DEBUG("sceNetListen(): 0x%08X\n", ret);

	while (1) {

		/* Accept clients */
		SceNetSockaddrIn clientaddr;
		int client_sockfd;
		unsigned int addrlen = sizeof(clientaddr);

		DEBUG("Waiting for incoming connections...\n");

		client_sockfd = sceNetAccept(server_sockfd, (SceNetSockaddr *)&clientaddr, &addrlen);
		if (client_sockfd >= 0) {

			DEBUG("New connection, client fd: 0x%08X\n", client_sockfd);

			/* Get the client's IP address */
			char remote_ip[16];
			sceNetInetNtop(SCE_NET_AF_INET,
				&clientaddr.sin_addr.s_addr,
				remote_ip,
				sizeof(remote_ip));

			INFO("Client %i connected, IP: %s port: %i\n",
				number_clients, remote_ip, clientaddr.sin_port);

			/* Create a new thread for the client */
			char client_thread_name[64];
			sprintf(client_thread_name, "FTPVita_client_%i_thread",
				number_clients);

			SceUID client_thid = sceKernelCreateThread(
				client_thread_name, client_thread,
				0x10000100, 0x10000, 0, 0, NULL);

			DEBUG("Client %i thread UID: 0x%08X\n", number_clients, client_thid);

			/* Allocate the ClientInfo struct for the new client */
			ClientInfo *client = malloc(sizeof(*client));
			client->num = number_clients;
			client->thid = client_thid;
			client->ctrl_sockfd = client_sockfd;
			client->data_con_type = FTP_DATA_CONNECTION_NONE;
			sprintf(client->cur_path, "%s%s", FTP_DEFAULT_PREFIX, FTP_DEFAULT_PATH);
			memcpy(&client->addr, &clientaddr, sizeof(client->addr));

			/* Add the new client to the client list */
			client_list_add(client);

			/* Start the client thread */
			sceKernelStartThread(client_thid, sizeof(*client), client);

			number_clients++;
		} else {
			/* if sceNetAccept returns < 0, it means that the listening
			 * socket has been closed, this means that we want to
			 * finish the server thread */
			DEBUG("Server socket closed, 0x%08X\n", client_sockfd);
			break;
		}
	}

	DEBUG("Server thread exiting!\n");

	sceKernelExitDeleteThread(0);
	return 0;
}

static int netctl_init = -1;
static int net_init = -1;

int ftp_init(char *vita_ip, unsigned short int *vita_port)
{
	int ret;
	UNUSED(ret);

	SceNetInitParam initparam;
	SceNetCtlInfo info;

	if (ftp_initialized) {
		return -1;
	}

	/* Init Net */
	if (sceNetShowNetstat() == SCE_NET_ERROR_ENOTINIT) {
		net_memory = malloc(NET_INIT_SIZE);

		initparam.memory = net_memory;
		initparam.size = NET_INIT_SIZE;
		initparam.flags = 0;

		net_init = sceNetInit(&initparam);
		DEBUG("sceNetInit(): 0x%08X\n", net_init);
	} else {
		DEBUG("Net is already initialized.\n");
	}

	/* Init NetCtl */
	netctl_init = sceNetCtlInit();
	DEBUG("sceNetCtlInit(): 0x%08X\n", netctl_init);

	/* Get IP address */
	ret = sceNetCtlInetGetInfo(SCE_NETCTL_INFO_GET_IP_ADDRESS, &info);
	if (ret < 0) {
		if (netctl_init >= 0) {
			sceNetCtlTerm();
			netctl_init = -1;
		}
		
		if (net_init >= 0) {
			sceNetTerm();
			net_init = -1;
		}

		if (net_memory) {
			free(net_memory);
			net_memory = NULL;
		}
		
		return ret;
	}

	DEBUG("sceNetCtlInetGetInfo(): 0x%08X\n", ret);

	/* Return data */
	strcpy(vita_ip, info.ip_address);
	*vita_port = FTP_PORT;

	/* Save the IP of PSVita to a global variable */
	sceNetInetPton(SCE_NET_AF_INET, info.ip_address, &vita_addr);

	/* Create server thread */
	server_thid = sceKernelCreateThread("FTPVita_server_thread",
		server_thread, 0x10000100, 0x10000, 0, 0, NULL);
	DEBUG("Server thread UID: 0x%08X\n", server_thid);

	/* Create the client list mutex */
	client_list_mtx = sceKernelCreateMutex("FTPVita_client_list_mutex", 0, 0, NULL);
	DEBUG("Client list mutex UID: 0x%08X\n", client_list_mtx);

	/* Start the server thread */
	sceKernelStartThread(server_thid, 0, NULL);

	ftp_initialized = 1;

	return 0;
}

void ftp_fini()
{
	if (ftp_initialized) {
		/* In order to "stop" the blocking sceNetAccept,
		 * we have to close the server socket; this way
		 * the accept call will return an error */
		sceNetSocketClose(server_sockfd);

		/* Wait until the server threads ends */
		sceKernelWaitThreadEnd(server_thid, NULL, NULL);

		/* To close the clients we have to do the same:
		 * we have to iterate over all the clients
		 * and shutdown their sockets */
		client_list_thread_end();

		/* Delete the client list mutex */
		sceKernelDeleteMutex(client_list_mtx);

		client_list = NULL;

		if (netctl_init >= 0) {
			sceNetCtlTerm();
			netctl_init = -1;
		}
		
		if (net_init >= 0) {
			sceNetTerm();
			net_init = -1;
		}

		if (net_memory) {
			free(net_memory);
			net_memory = NULL;
		}

		ftp_initialized = 0;
	}
}

int ftp_is_initialized()
{
	return ftp_initialized;
}