/*****************************************************************************
**      httpdpi.c                                                   2017-02-21
**
**      Code for the Mini HTTP Daemon project. Uses POSIX Threads to run in
**      parallel with your code.
**
**      Copyright (c) 2017 Elias Zacarias
**
**      Permission is hereby granted, free of charge, to any person obtaining a
**      copy of this software and associated documentation files (the "Software"),
**      to deal in the Software without restriction, including without limitation
**      the rights to use, copy, modify, merge, publish, distribute, sublicense,
**      and/or sell copies of the Software, and to permit persons to whom the
**      Software is furnished to do so, subject to the following conditions:
**
**      The above copyright notice and this permission notice shall be included in
**      all copies or substantial portions of the Software.
**      THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
**      OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
**      FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
**      THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
**      LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
**      FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
**      DEALINGS IN THE SOFTWARE.
**
*****************************************************************************/
#include "httpdpi.h"

/* Private data structures */
typedef struct {
  int                   socket;
  unsigned int          port;
  struct sockaddr_in    in_addr;
  pthread_t             thread;
  httpd_request_handler *reqHandler;
  httpd_error_handler   *errHandler;
}httpd_conn;

/* Private data */
HTTPD_STATUS httpdStatus = HTTPD_SERVICE_STOPPED;
httpd_conn   HTTPConn;


/*###########################################################################
 ##                                                                        ##
 ##                    P R I V A T E   F U N C T I O N S                   ##
 ##                                                                        ##
 ###########################################################################*/
const char *httpCodeDesc(int code){
  switch (code){
    case 100: return "Continue";
    case 101: return "Switching Protocols";
    case 102: return "Processing";
    case 200: return "OK";
    case 201: return "Created";
    case 202: return "Accepted";
    case 203: return "Non-Authoritative Information";
    case 204: return "No Content";
    case 205: return "Reset Content";
    case 206: return "Partial Content";
    case 207: return "Multi-Status";
    case 208: return "Already Reported";
    case 226: return "IM Used";
    case 300: return "Multiple Choices";
    case 301: return "Moved Permanently";
    case 302: return "Found";
    case 303: return "See Other";
    case 304: return "Not Modified";
    case 305: return "Use Proxy";
    case 306: return "(Unused)";
    case 307: return "Temporary Redirect";
    case 308: return "Permanent Redirect";
    case 400: return "Bad Request";
    case 401: return "Unauthorized";
    case 402: return "Payment Required";
    case 403: return "Forbidden";
    case 404: return "Not Found";
    case 405: return "Method Not Allowed";
    case 406: return "Not Acceptable";
    case 407: return "Proxy Authentication Required";
    case 408: return "Request Timeout";
    case 409: return "Conflict";
    case 410: return "Gone";
    case 411: return "Length Required";
    case 412: return "Precondition Failed";
    case 413: return "Payload Too Large";
    case 414: return "URI Too Long";
    case 415: return "Unsupported Media Type";
    case 416: return "Range Not Satisfiable";
    case 417: return "Expectation Failed";
    case 421: return "Misdirected Request";
    case 422: return "Unprocessable Entity";
    case 423: return "Locked";
    case 424: return "Failed Dependency";
    case 426: return "Upgrade Required";
    case 428: return "Precondition Required";
    case 429: return "Too Many Requests";
    case 431: return "Request Header Fields Too Large";
    case 451: return "Unavailable For Legal Reasons";
    case 500: return "Internal Server Error";
    case 501: return "Not Implemented";
    case 502: return "Bad Gateway";
    case 503: return "Service Unavailable";
    case 504: return "Gateway Timeout";
    case 505: return "HTTP Version Not Supported";
    case 506: return "Variant Also Negotiates";
    case 507: return "Insufficient Storage";
    case 508: return "Loop Detected";
    case 510: return "Not Extended";
    case 511: return "Network Authentication Required";
  }
  return "????";
}

void socketSendStr (int socket, const char *str){
  if (str) write (socket, str, strlen(str));
  write (socket, "\r\n", 2);
}


void httpdResponse (int socket, int status, char *content_type, char *content, int length){
  char response[512];
  time_t now = time(0);
  struct tm tm = *gmtime(&now);

  snprintf(response, sizeof response, "HTTP/1.1 %d %s", status, httpCodeDesc(status));
  socketSendStr (socket, response);

  socketSendStr(socket, "Server: RPi Mini HTTPD Server");

  strftime(response, sizeof response, "Date: %a, %d %b %Y %H:%M:%S %Z", &tm);
  snprintf(response, sizeof response, "Content-Type: %s", content_type);
  socketSendStr(socket, response);

  if (content){
    snprintf(response, sizeof response, "Content-Length: %d", length);
    socketSendStr (socket, response);
  }

  /* End of headers. This will generate a line break */
  socketSendStr(socket, NULL);

  if (content) {
    write (socket, content, length);
    /* Another line break */
    socketSendStr(socket, NULL);
  }
}

/* WARNING: This function will call the error handler and close the socket */
void httpRaiseError (httpd_conn *conn_data, HTTPD_ERROR err) {
  if (!conn_data) return;
  if (conn_data->errHandler) conn_data->errHandler (conn_data->socket, err);
  close (conn_data->socket);
  /* Deallocate connection data unless it's our root socket */
  if (conn_data != & HTTPConn) free ((void *)conn_data);
}


/* Thus function should be responsible of freeing the memory used by the
   httpd_conn data */
void *httpSocketProc(void *httpdConnPtr){
  httpd_conn *httpdConn = (httpd_conn *)httpdConnPtr;
  int socket = httpdConn->socket;
  char buffer[512];
  char *httpLine;
  char *strPtr;

  bzero(buffer, 256);
  if (read(socket, buffer,255) < 0) {
    httpRaiseError (httpdConn, HTTPD_ERROR_CANTREADSOCKET);
    free (httpdConnPtr);
    return NULL;
  }

  // Parse the HTTP message
  httpLine = strtok(buffer, "\r\n");
  // We are only interested in the first line
  if (startsWith(httpLine, "GET ")){
    // URL
    strPtr = strchr(&httpLine[4], 32); // Search for a space delimitator
    if (strPtr) *strPtr = 0; // Seal the string there. Now the requested URL is at &httpLine[4].
    strPtr = &httpLine[4];
    if (httpdConn->reqHandler) httpdConn->reqHandler (socket, httpdConn->in_addr, strPtr);
  }

  // Close
  shutdown(socket, SHUT_RDWR);
  close(socket);
  free (httpdConnPtr);

  return NULL;
}

void *httpListenProc(void *data){
  int                reqSocket;
  socklen_t          clilen;
  httpd_conn         *httpdConn = NULL;


  if (httpdStatus != HTTPD_SERVICE_STOPPED) return NULL;


  httpdStatus = HTTPD_SERVICE_STARTING;
  HTTPConn.socket = socket(AF_INET, SOCK_STREAM, 0);
  if (HTTPConn.socket < 0) {
    httpRaiseError(&HTTPConn, HTTPD_ERROR_CANTOPENSOCKET);
    return;
  }

  bzero((char *) &HTTPConn.in_addr, sizeof (struct sockaddr_in));
  HTTPConn.in_addr.sin_family = AF_INET;
  HTTPConn.in_addr.sin_addr.s_addr = INADDR_ANY;
  HTTPConn.in_addr.sin_port = htons(HTTPConn.port);

  if (bind(HTTPConn.socket, (struct sockaddr *) &HTTPConn.in_addr, sizeof (struct sockaddr_in)) < 0) {
    httpRaiseError (&HTTPConn, HTTPD_ERROR_CANTBINDSOCKET);
    return;
  }

  if (listen(HTTPConn.socket, 5)){
    httpRaiseError (&HTTPConn, HTTPD_ERROR_CANTSETTOLISTEN);
    return;
  }

  httpdStatus = HTTPD_SERVICE_RUNNING;

  while (httpdStatus == HTTPD_SERVICE_RUNNING){
    clilen = sizeof(struct sockaddr_in);

    /* Prepare the client data structure. Allocate if needed */
    if (httpdConn == NULL){
      httpdConn = (httpd_conn *)malloc (sizeof(httpd_conn));
      httpdConn->thread = 0;
      httpdConn->port = HTTPConn.port;
      httpdConn->errHandler = HTTPConn.errHandler;
      httpdConn->reqHandler = HTTPConn.reqHandler;
    }
    httpdConn->socket = -1;
    bzero ((char *)&httpdConn->in_addr, sizeof(struct sockaddr_in));

    /* Accept an incoming connection. Use the previously prepared structure to hold relevant data */
    httpdConn->socket = accept(HTTPConn.socket, (struct sockaddr *) &httpdConn->in_addr, &clilen);
    if (httpdConn->socket >= 0 && pthread_create(&(httpdConn->thread), NULL, &httpSocketProc, (void *)httpdConn) >= 0) {
      printf("HTTP Connection accepted!\n");
      /* Once the connection has been accepted, httpSocket proc should take care of the memory used by httpdConn,
         so we now clear our local pointer, causing our code to allocate a new structure for the next connection */
      httpdConn = NULL;
    }else {
      printf("\nWARNING: Couldn't accept connection.\n");
    }
  }

  if (httpdConn != NULL) free ((void*)httpdConn);
  close(HTTPConn.socket);
  httpdStatus = HTTPD_SERVICE_STOPPED;
}


/*###########################################################################
 ##                                                                        ##
 ##                     P U B L I C   F U N C T I O N S                    ##
 ##                                                                        ##
 ###########################################################################*/
bool httpd_start(unsigned int port, httpd_request_handler *reqHdl, httpd_error_handler *errHdl){
  if (httpdStatus != HTTPD_SERVICE_STOPPED) return false;
  HTTPConn.reqHandler = reqHdl;
  HTTPConn.errHandler = errHdl;
  HTTPConn.port = port;

  if (pthread_create(&(HTTPConn.thread), NULL, &httpListenProc, NULL) >= 0) return true;
  return false;
}


HTTPD_STATUS httpd_getStatus(){
  return httpdStatus;
}


void httpd_response (int socket, char *content_type, char *content, int len){
  httpdResponse (socket, 200, content_type, content, len);
}


void httpd_HTMLResponse (int socket, int status, char *content){
  httpdResponse (socket, status, "text/html; charset=UTF-8", content, strlen(content));
}


bool httpd_fileResponse(int socket, char *mime_type, FILE *fh) {
  char *buffer = NULL;
  int curPos = ftell(fh);
  int len;
  fseek (fh, 0, SEEK_END);
  len = ftell(fh) - curPos;
  fseek (fh, 0, SEEK_SET);

  buffer = (char *)malloc (len);
  if (!buffer) return false;
  fread(buffer, 1, len, fh);
  httpdResponse (socket, 200, mime_type, buffer, len);
  free (buffer);

  return true;
}


bool httpd_stop(){
  if (httpdStatus == HTTPD_SERVICE_RUNNING) {
    httpdStatus = HTTPD_SERVICE_STOPPING;
    return true;
  }
  return false;
}

