#include <stdio.h>
#include "httpdpi.h"


bool keepRunning = true;
int  testCounter = 0;

void HTTP_handler (int socket, struct sockaddr_in in_addr, char *requestURI){
  FILE *fh;
  char buffer[256];

  printf ("> HTTP Request received from %s,  URI:%s\n", inet_ntoa(in_addr.sin_addr), requestURI);
  /* Let's respond to the request */
  if (!strcmp(requestURI, "/stop")){
    keepRunning = false;
    httpd_HTMLResponse(socket, 200, "Done!");
  }else if (!strcmp(requestURI, "/counter")) {
    sprintf(buffer, "Counter = <b>%d</b>", testCounter);
    httpd_HTMLResponse(socket, 200, buffer);
  }else if (!strcmp(requestURI, "/picture")) {
    fh = fopen("tux.png", "rb");
    httpd_fileResponse(socket, "image/png", fh);
    fclose (fh);
  }else {
    httpd_HTMLResponse(socket, 404, "<h1>404</h1><h3>Your data is in another castle.</h3>");
  }
}

void HTTP_errHandler (int socket, HTTPD_ERROR err){
  printf ("HTTP ERROR! ");
  switch (err){
    case HTTPD_ERROR_CANTREADSOCKET:
      printf ("Can't read from socket");
      break;
    case HTTPD_ERROR_CANTBINDSOCKET:
      printf ("Can't bind socket");
      break;
    case HTTPD_ERROR_CANTOPENSOCKET:
      printf ("Can't open socket");
      break;
    case HTTPD_ERROR_CANTSETTOLISTEN:
      printf ("Can't set socket to listen");
      break;
    default:
      printf ("UNKNOWN! (%d)", err);
  }
  printf ("!!\n");
}


int main(int argc, char* argv[]){
  /* Start the Mini HTTP Server */
  printf ("Mini HTTP Server Init... ");
  if (httpd_start (8080, &HTTP_handler, &HTTP_errHandler)){
    printf ("OK\n");
  }else {
    printf (" ERROR!\n");
  }

  /* We can do other stuff while the service runs */
  while (keepRunning){
    printf("Let's count. %d\n", testCounter);
    sleep(5);
    testCounter++;
  }
}

