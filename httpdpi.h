/*****************************************************************************
**	httpdpi.h                                                   2017-02-21
**
**	Data type and function definitions for the Mini HTTP Daemon project.
**      Uses POSIX Threads.
**
** 	Copyright (c) 2017 Elias Zacarias
**
** 	Permission is hereby granted, free of charge, to any person obtaining a
** 	copy of this software and associated documentation files (the "Software"),
** 	to deal in the Software without restriction, including without limitation
** 	the rights to use, copy, modify, merge, publish, distribute, sublicense,
** 	and/or sell copies of the Software, and to permit persons to whom the
** 	Software is furnished to do so, subject to the following conditions:
**
** 	The above copyright notice and this permission notice shall be included in
** 	all copies or substantial portions of the Software.
** 	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
** 	OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
** 	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
** 	THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
** 	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
** 	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
**      DEALINGS IN THE SOFTWARE.
**
*****************************************************************************/
#ifndef __HTTPDPI_H
#define __HTTPDPI_H

#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <dirent.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>


#define  HTTPDPI_VERSION    "1.0"

/* Macros */
#define startsWith(str, a) (strncmp(str, a, strlen(a)) == 0)


/* Data types */
typedef enum {
  HTTPD_SERVICE_STOPPED,
  HTTPD_SERVICE_STARTING,
  HTTPD_SERVICE_RUNNING,
  HTTPD_SERVICE_STOPPING
}HTTPD_STATUS;

typedef enum {
  HTTPD_ERROR_NONE,
  HTTPD_ERROR_CANTSETTOLISTEN,
  HTTPD_ERROR_CANTREADSOCKET,
  HTTPD_ERROR_CANTBINDSOCKET,
  HTTPD_ERROR_CANTOPENSOCKET
}HTTPD_ERROR;

typedef void (httpd_request_handler)(int socket, struct sockaddr_in in_addr, char *requestURI);
typedef void (httpd_error_handler)(int socket, HTTPD_ERROR err);

/* Functions */
HTTPD_STATUS httpd_getStatus();
bool         httpd_stop();
bool         httpd_start(unsigned int port, httpd_request_handler *req_hdl, httpd_error_handler *err_hdl);
void         httpd_response (int socket, char *content_type, char *content, int len);
void         httpd_HTMLResponse (int socket, int status, char *content);
bool         httpd_fileResponse(int socket, char *mime_type, FILE *fh);

#endif

