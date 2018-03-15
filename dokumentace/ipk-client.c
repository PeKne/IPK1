/**
 * Author:    PETR KNETL
 * Created:   1.03.2018
 *
 * Project: School project 1 to IPK class, basic client-server file transport protocol
 **/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <ctype.h>
#include <stdlib.h>
#include <fcntl.h>
#include <time.h>

//TODO : navratove hodnoty, zpravy

int main(int argc, char* argv[]){
  int cli_socket; // promenna pro SOCKET
  int port; //PORT
  struct sockaddr_in serv_addr;
  struct hostent *server;
  char file_buffer[256];
  char communication_buffer[256];
  int incomingBytes;

  if (argc != 7) {
    perror("ERROR: False number of parameters\n");
    return 4;
  }
 int par;
 int nread;
 int hflag, pflag, rflag, wflag; // pomocne promenne pro getopt()
 hflag = pflag = rflag = wflag = 0;
 char *hvalue, *pvalue, *rvalue, *wvalue; // pomocne promenne pro getopt()
 hvalue = pvalue = rvalue = wvalue = NULL;

 while ((par = getopt (argc, argv, "h:p:r:w:")) != -1) //zpracovani parametru
    switch (par)
      {
      case 'h':
        hflag = 1;
        hvalue = optarg;
        break;

      case 'p':
        pflag = 1;
        pvalue = optarg;
        break;

      case 'r':
        rflag = 1;
        rvalue = optarg;
        break;

      case 'w':
        wflag = 1;
        wvalue = optarg;
        break;

      case '?':
        if (optopt == 'h' || optopt == 'p' || optopt == 'r' || optopt == 'w')
          fprintf (stderr, "Option -%c requires an argument.\n", optopt);
        else if (isprint(optopt)){
          fprintf (stderr, "Unknown option `-%c'.\n", optopt);
        }
        else
          fprintf (stderr,
                   "Unknown option character `\\x%x'.\n",
                   optopt);
        return 4;
      default:
        abort ();
      }

 if(hflag != 1  || pflag != 1  || (rflag == wflag )){
   printf("ERROR: wrong format of parameters\n");
   return 4;
 }

  port = atoi(pvalue);

  if((cli_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0){ // inicializace socketu
    printf("ERROR: unable to create client socket\n");
    return 2;
  }


  setsockopt(cli_socket, SOL_SOCKET, SO_REUSEADDR, (const void *)1, sizeof(int));

  server = gethostbyname(hvalue); //nacteni adresy hosta
  if(server == NULL){
    printf("ERROR: no such host\n");
    return 3;
  }

  bzero((char*)& serv_addr, sizeof(serv_addr)); // nastaveni hodnot adresy
  serv_addr.sin_family = AF_INET;
  bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
  serv_addr.sin_port = htons(port);

  if (connect(cli_socket, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0){ // pripojeni na server
    printf("ERROR: unable to connect\n");
    return 4;
  }



  /* CTENI ZE SERVERU*/
  if(rflag == 1){

    const char delim = '/';
    char path[256];
    char* filename;

    bzero(path, 256);
    strcpy(path, rvalue);

    filename =strrchr(path, delim);

    bzero(communication_buffer, 256);
    char message[256];
    strcpy(message, "r");
    if (filename != NULL){
      strcat(message, filename + 1);
    }
    else{
      strcat(message, rvalue);
    }

    strcpy(communication_buffer, message);

    printf("%s\n", communication_buffer);

    int r = write(cli_socket, communication_buffer, strlen(communication_buffer));
    if(r < 0){
      perror("ERROR: unable to write to server\n");
      return 4645641;
    }

    char msg[8];
    bzero(msg, 8);
    r = read(cli_socket, msg, 7);
    if(r < 0){
      perror("ERROR: unable to read from server\n");
      return 4645641;
    }

    if(strcmp(msg, "ERRFILE") == 0){
        fprintf(stderr, "ERROR: Server doesn't have file \"%s\"! \n", filename);
         exit(43564);
    }
    FILE *client_file = fopen(rvalue, "wb");

    if(NULL == client_file){
      printf("Error opening file");
      return 1;
    }

    fwrite(msg, 1, 7, client_file);

  /* Receive data in chunks of 256 bytes */

    while((incomingBytes = read(cli_socket, file_buffer, 256)) > 0){
      printf("Bytes received %d\n",incomingBytes);
      fwrite(file_buffer, 1,incomingBytes,client_file);
    }
    if(incomingBytes < 0){
      printf("\n Read Error \n");
    }

    fclose(client_file);
  }// konec cteni ze serveru


  /* ZAPIS NA SERVER*/
  else if(wflag == 1){

    FILE *client_file = fopen(wvalue, "rb");
    if(NULL == client_file){
      printf("Error opening file");
      return 1;
    }

    const char delim = '/';
    char path[256];
    char* filename;

    bzero(path, 256);
    strcpy(path, wvalue);

    filename = strrchr(path, delim);



    bzero(communication_buffer, 256);
    char message[256];
    strcpy(message, "w");
    if (filename != NULL){
      strcat(message, filename + 1);
    }
    else{
      strcat(message, wvalue);
    }

    strcpy(communication_buffer, message);

    int w = write(cli_socket, communication_buffer, strlen(communication_buffer));
    if(w < 0){
      perror("ERROR: unable to write to server\n");
      return 4645641;
    }



    usleep(100000);

    while(1){

      unsigned char write_buff[256];
      bzero(write_buff, 256);

      nread = fread(write_buff,1 , 256, client_file);

      if(nread > 0){
        printf("Sending %d bytes to server \n", nread);
        write(cli_socket, write_buff, nread);
      }

      if (nread < 256){
            if (ferror(client_file))
            printf("Error reading\n");
            break;
      }

    }
    fclose(client_file);
  }

  close(cli_socket); // uzavreni socketu
  return 0;


}
