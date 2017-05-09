/***************************************************************************
                                 ibtest.c
                             -------------------

Example program which uses gpib c library.  More complex than an example
program needs to be really, but useful for testing library functions.

    copyright            : (C) 2002 by Frank Mori Hess
    email                : fmhess@users.sourceforge.net
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <getopt.h>
#include "gpib/ib.h"

#define DEBUG_LEVEL_0

/* returns a device descriptor after prompting user for primary address */
int set_device(int minor, int pad)	{
	int ud;
	const int sad = 0;
	const int send_eoi = 1;
	const int eos_mode = 0;
	const int timeout = T3s;

	if(pad < 0 || pad > 30){
		fprintf(stderr,"primary address must be between 0 and 30\n");
		return -1;
	}

#ifdef DEBUG_LEVEL1
	fprintf(stderr, "trying to open pad = %i on /dev/gpib%i ...\n", pad, minor);
#endif
	
	ud = ibdev(minor, pad, sad, timeout, send_eoi, eos_mode);
	if(ud < 0)
	{
		fprintf(stderr,"ibdev error\n");
		abort();
	}

	return ud;
}

int ib_read(int ud, int max_num_bytes, char *buffer)	{
	int buffer_size = max_num_bytes + 1;

	ibrd(ud, buffer, buffer_size - 1);

#ifdef DEBUG_LEVEL_2
	int is_string;
	int i;

	is_string = 1;
	for(i = 0; i < ThreadIbcntl(); ++i)
	{
		if(isascii(buffer[i]) == 0) 
		{
			is_string = 0;
			break;
		}
	}

	if(is_string)
	{
		fprintf(stderr,"received string: '%s'\n", buffer);
	}else
	{
		fprintf(stderr,"received binary data (hex): ");
		for(i = 0; i < ThreadIbcntl(); ++i)
		{
			fprintf(stderr," %02x ", (uint8_t)buffer[i]);
		}
		fprintf(stderr,"\n");
	}
	fprintf(stderr,"Number of bytes read: %li\n", ThreadIbcntl());
#endif	

	if(ThreadIbsta() & ERR)
		return -1;
	return ThreadIbcntl();	
}

int ib_write(int ud, char *command)	{
	
	if( ibwrt(ud, command, strlen(command)) & ERR )
	{
		fprintf(stderr,"Can not send command %s\n\r",command);
		return -1;
	}
#ifdef DEBUG_LEVEL_2
		fprintf(stderr,"Command %s send\n\r",command);	
#endif	
	return 0;
}

int ib_query(int ud, uint max_num_bytes, char *command, char *buffer)	{
	int result;
	
	result = ib_write(ud,command);
	if (result == -1)
		return -1;
	usleep(10000);		
	
	result = ib_read(ud, max_num_bytes, buffer);
	return result;
}
