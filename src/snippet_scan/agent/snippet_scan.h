/**************************************************************
 Copyright (C) 2010-2011 Hewlett-Packard Development Company, L.P.

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 2 as published by the Free Software Foundation.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License along
 with this program; if not, write to the Free Software Foundation, Inc.,
 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

 ***************************************************************/
/**
 * \file
 * \brief pkgagent header
 */
#ifndef _SNIPPET_SCAN_H
#define _SNIPPET_SCAN_H 1

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <libgen.h>
#include <time.h>

#include <sys/wait.h>


#include <libfossology.h>

#define MAXCMD 5000
#define MAXLENGTH 256


extern PGconn* db_conn;         ///< the connection to Database

int ProcessUpload(long upload_pk);
void Usage(char *Name);
int runScan(char *path,unsigned char ** detected);


#endif /*  _SNIPPET_SCAN_H */
