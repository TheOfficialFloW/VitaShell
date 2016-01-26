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
#include <debugnet.h>
#include "psp2link.h"

int psp2link_requests_thread(SceSize args, void *argp);
int psp2link_commands_thread(SceSize args, void *argp);



SceUID server_request_thid;
SceUID server_command_thid;

psp2LinkConfiguration *configuration=NULL;

int external_conf=0;


/**
 * Init psp2link library 
 *
 * @par Example:
 * @code
 * int ret;
 * ret = psp2LinkInit("172.26.0.2",0x4711,0x4712,0x4712, DEBUG);
 * @endcode
 * 
 * @param serverIp - server ip for udp debug
 * @param requestPort - psp2  port server for requests
 * @param debugPort - udp port for debug
 * @param commandPort - psp2  port server for commands
 * @param level - DEBUG,ERROR,INFO or NONE 
 */
int psp2LinkInit(char *serverIp, int requestPort,int debugPort, int commandPort, int level)
{
	int ret;
	
	if(psp2LinkCreateConf())
	{
		return psp2LinkGetValue(PSP2LINK_ACTIVE);
	}
	configuration->psp2link_requests_port=requestPort;
	configuration->psp2link_commands_port=commandPort;
	configuration->psp2link_debug_port=debugPort;
	
	if(debugNetInit(serverIp,debugPort,level))
	{			
		
		
		configuration->debugconf=debugNetGetConf();
		
		server_request_thid = sceKernelCreateThread("psp2link_request_server_thread", psp2link_requests_thread, 64, 0x80000, 0, 0, NULL);
		

		if(server_request_thid<0)
		{
			debugNetPrintf(ERROR,"[PSP2LINK] Server request thread could not create error: 0x%08X\n", server_request_thid);
			psp2LinkFinish();
			return 0;
		}
		debugNetPrintf(DEBUG,"[PSP2LINK] Server request thread UID: 0x%08X\n", server_request_thid);
		
		
		/* Start the server request thread */
		ret=sceKernelStartThread(server_request_thid, 0, NULL);
		if(ret<0)
		{
				
			debugNetPrintf(ERROR,"Server command thread could not start error: 0x%08X\n", ret);
			sceKernelDeleteThread(server_request_thid);
			psp2LinkFinish();
			return 0;
				
		}
		
	
		server_command_thid = sceKernelCreateThread("psp2link_command_server_thread", psp2link_commands_thread, 0x10000100, 0x10000, 0, 0, NULL);
		
		if(server_command_thid<0)
		{
			debugNetPrintf(ERROR,"Server command thread could not create error: 0x%08X\n", server_command_thid);
			sceKernelDeleteThread(server_request_thid);
			psp2LinkFinish();
			return 0;
		}
		debugNetPrintf(DEBUG,"Server command thread UID: 0x%08X\n", server_command_thid);

		
		/* Start the server command thread */
		ret=sceKernelStartThread(server_command_thid, 0, NULL);
		if(ret<0)
		{
			debugNetPrintf(ERROR,"Server command thread could not start error: 0x%08X\n", ret);
			sceKernelDeleteThread(server_command_thid);
			sceKernelDeleteThread(server_request_thid);
			psp2LinkFinish();
			return 0;
		}
		
	
		
	
		/*library psp2link initialized*/	
	    configuration->psp2link_initialized = 1;
		
	}
	else
	{
		configuration->psp2link_initialized = 0;
		psp2LinkFinish();
	}

    return psp2LinkGetValue(PSP2LINK_ACTIVE);

}
/**
 * Set configuration to PSP2Link Library 
 *
 * @par Example:
 * @code
 * psp2LinkSetConfig(myConf);
 * @endcode
 */
int psp2LinkSetConfig(psp2LinkConfiguration *conf)
{
	if(!conf)
	{
		return 0;
	}
	configuration=conf;
	external_conf=1;
	return 1;
}
/**
 * Get configuration from PSP2Link Library 
 *
 * @par Example:
 * @code
 * psp2LinkConfiguration *myConf=psp2LinkGetConfig();
 * @endcode
 */
psp2LinkConfiguration *psp2LinkGetConfig()
{
	if(!configuration)
	{
		return NULL;
	}
	return configuration;
}
/**
 * Init with configuration  
 *
 * @par Example:
 * @code
 * psp2LinkInitWithConf(externalconf);
 * @endcode
 */
int psp2LinkInitWithConf(psp2LinkConfiguration *conf)
{
	
	int ret;
	ret=psp2LinkSetConfig(conf);
	if(ret)
	{
		if(debugNetInitWithConf(conf->debugconf))
		{
		
			debugNetPrintf(INFO,"psp2link already initialized using configuration from psp2link\n");			
			debugNetPrintf(INFO,"psp2link_fileio_active=%d\n",psp2LinkGetValue(FILEIO_ACTIVE));
			debugNetPrintf(INFO,"psp2link_cmdsio_active=%d\n",psp2LinkGetValue(CMDSIO_ACTIVE));
			debugNetPrintf(INFO,"psp2link_initialized=%d\n",psp2LinkGetValue(PSP2LINK_ACTIVE));
			debugNetPrintf(INFO,"psp2link_requests_port=%d\n",psp2LinkGetValue(REQUESTS_PORT));
			debugNetPrintf(INFO,"psp2link_commands_port=%d\n",psp2LinkGetValue(COMMANDS_PORT));
			debugNetPrintf(INFO,"psp2link_debug_port=%d\n",psp2LinkGetValue(DEBUG_PORT));
			debugNetPrintf(INFO,"psp2link_fileio_sock=%d\n",psp2LinkGetValue(FILEIO_SOCK));
			debugNetPrintf(INFO,"psp2link_requests_sock=%d\n",psp2LinkGetValue(REQUESTS_SOCK));
			debugNetPrintf(INFO,"psp2link_commands_sock=%d\n",psp2LinkGetValue(COMMANDS_SOCK));
			return psp2LinkGetValue(PSP2LINK_ACTIVE);
		}
		else
		{
			return 0;
		}
	}
	else
	{
		return 0;
	}
}
/**
 * Create configuration values from PSP2Link library 
 *
 * @par Example:
 * @code
 * psp2LinkCreateConf();
 * @endcode
 */
int psp2LinkCreateConf()
{	
	if(!configuration)
	{
		configuration=malloc(sizeof(psp2LinkConfiguration));
		configuration->psp2link_fileio_active=1;
		configuration->psp2link_cmdsio_active=1;
		configuration->psp2link_initialized=0;
		configuration->psp2link_fileio_sock=-1;
		configuration->psp2link_requests_sock=-1;
		configuration->psp2link_commands_sock=-1;
		return 0;
	}
	if(configuration->psp2link_initialized)
	{
		return 1;
	}
	return 0;			
}


/**
 * Get configuration values from PSP2Link library 
 *
 * @par Example:
 * @code
 * psp2LinkGetValue(PS4LINK_ACTIVE);
 * @endcode
 */
int psp2LinkGetValue(psp2LinkValue val)
{
	int ret;
	switch(val)
	{
		case FILEIO_ACTIVE:
			ret=configuration->psp2link_fileio_active;
			break;
		case CMDSIO_ACTIVE:
			ret=configuration->psp2link_cmdsio_active;
			break;
		case DEBUGNET_ACTIVE:
			ret=configuration->debugconf->debugnet_initialized;
			break;
		case PSP2LINK_ACTIVE:
			ret=configuration->psp2link_initialized;
			break;
		case REQUESTS_PORT:
			ret=configuration->psp2link_requests_port;
			break;
		case COMMANDS_PORT:
			ret=configuration->psp2link_commands_port;
			break;
		case DEBUG_PORT:
			ret=configuration->psp2link_debug_port;
			break;
		case FILEIO_SOCK:
			ret=configuration->psp2link_fileio_sock;
			break;
		case REQUESTS_SOCK:
			ret=configuration->psp2link_requests_sock;
			break;
		case COMMANDS_SOCK:
			ret=configuration->psp2link_commands_sock;
			break;
		case DEBUG_SOCK:
			ret=configuration->debugconf->SocketFD;
			break;
		case LOG_LEVEL:
			ret=configuration->debugconf->logLevel;
			break;
		default:
			ret=0;
			break;
	}
	return ret;
}

/**
 * Finish debugnet library 
 *
 * @par Example:
 * @code
 * psp2LinkFinish();
 * @endcode
 */
void psp2LinkFinish()
{	
	if(!external_conf)
	{
		configuration->psp2link_fileio_active=0;
		configuration->psp2link_cmdsio_active=0;
    	configuration->psp2link_initialized=0;
		psp2LinkRequestsAbort();
		while(psp2LinkRequestsIsConnected())
		{
		
		}
		debugNetFinish();
		free(configuration->debugconf);
		free(configuration);
	}
}
