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
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <ctype.h>
#include <stdlib.h>
#include <fcntl.h>
#include <time.h>

int main(int argc, char* argv[]){
  int cli_socket; // promenna pro SOCKET
  int port; //PORT
  struct sockaddr_in serv_addr;
  struct hostent *server;
  char file_buffer[256]; // buffer pro prenos dat souboru
  char communication_buffer[256]; // buffer pro prenos komunikacnich zprav
  int incomingBytes;

  if (argc != 7) { //spatny pocet parametru
    perror("ERROR: False number of parameters\n");
    return 1;
  }

// pomocne promenne pro getopt()
 int par;
 int nread;
 int hflag, pflag, rflag, wflag;
 hflag = pflag = rflag = wflag = 0;
 char *hvalue, *pvalue, *rvalue, *wvalue;
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
        return 1;
      default:
        abort ();
      }

 if(hflag != 1  || pflag != 1  || (rflag == wflag )){
   printf("ERROR: wrong format of parameters\n");
   return 1;
 }

  port = atoi(pvalue);

  if((cli_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0){ // inicializace socketu
    printf("ERROR: unable to create client socket\n");
    return 2;
  }


  setsockopt(cli_socket, SOL_SOCKET, SO_REUSEADDR, (const void *)1, sizeof(int));

  server = gethostbyname(hvalue); //nacteni adresy SERVERU
  if(server == NULL){
    printf("ERROR: no such host\n");
    return 2;
  }

  bzero((char*)& serv_addr, sizeof(serv_addr)); // nastaveni hodnot adresy
  serv_addr.sin_family = AF_INET;
  bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
  serv_addr.sin_port = htons(port);

  if (connect(cli_socket, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0){ // pripojeni na server
    printf("ERROR: unable to connect to \"%s\"\n", hvalue);
    return 4;
  }

  printf("INFO: connection establised\n" );



  /* CTENI ZE SERVERU*/
  if(rflag == 1){


    const char delim = '/'; //delimer pro odeleni nazvu souboru od cesty
    char raw_path[256], dir_path[256];
    char* filename;

    bzero(raw_path, 256);
    bzero(dir_path, 256);
    strcpy(raw_path, rvalue);

    filename =strrchr(raw_path, delim);

    //ulozeni cesty k souboru
    strncpy(dir_path, raw_path, (strlen(raw_path) - strlen(filename) + 1));

    bzero(communication_buffer, 256);
    char message[256];

    strcpy(message, "r");

    if (filename != NULL){ //pokud soubor je ve slozce
      filename = filename + 1;
      strcat(message, filename);
      mkdir(dir_path, 0777);
      printf("INFO: directory \"%s\" was created\n", dir_path);
    }
    else{ //samotny soubor
      strcat(message, rvalue);
      filename = rvalue;
    }

    strcpy(communication_buffer, message);

    //odeslani konfiguraci zpravy serveru
    int r = write(cli_socket, communication_buffer, strlen(communication_buffer));
    if(r < 0){
      perror("ERROR: unable to write to server\n");
      return 4;
    }

    char msg[8];
    bzero(msg, 8);
    r = read(cli_socket, msg, 7); // cteni zpravy od serveru zda soubor existuje
    if(r < 0){
      perror("ERROR: unable to read from server\n");
      return 4;
    }

    if(strcmp(msg, "ERRFILE") == 0){ // soubor neexistuje
        fprintf(stderr, "ERROR: Server doesn't have file \"%s\"! \n", filename);
         return 4;
    }

    FILE *client_file = fopen(rvalue, "wb");
    if(NULL == client_file){
      printf("Error opening file");
      return 5;
    }

    fwrite(msg, 1, 7, client_file); // zapis jiz prectenych dat

    // prijem dat souboru po blocich o 256 bajtech
    while((incomingBytes = read(cli_socket, file_buffer, 256)) > 0){
      fwrite(file_buffer, 1,incomingBytes,client_file); // zapis do souboru
    }
    if(incomingBytes < 0){
      printf("\n Read Error \n");
    }
    else{
      printf("INFO: file \"%s\" was downloaded from server on address %s \n", filename, hvalue);
    }

    fclose(client_file);
  }// konec cteni ze serveru


  /* ZAPIS NA SERVER*/
  else if(wflag == 1){

    FILE *client_file = fopen(wvalue, "rb");
    if(NULL == client_file){
      printf("Error opening file");
      return 5;
    }

    const char delim = '/'; //delimer pro odeleni nazvu souboru od cesty
    char path[256];
    char* filename;

    bzero(path, 256);
    strcpy(path, wvalue);

    filename = strrchr(path, delim); //ziskani nazvu souboru



    bzero(communication_buffer, 256);
    char message[256];
    strcpy(message, "w");
    if (filename != NULL){ // soubor je ulozen ve slozce
      strcat(message, filename + 1);
    }
    else{ //samotny soubor
      strcat(message, wvalue);
      filename = wvalue;
    }

    strcpy(communication_buffer, message);

    //odeslani konfiguracni zpravy severu
    int w = write(cli_socket, communication_buffer, strlen(communication_buffer));
    if(w < 0){
      perror("ERROR: unable to write to server\n");
      return 4;
    }
    usleep(100000); //casove pauza pr oddeleni zapisu zpravy a zapisu dat souboru

    while(1){ //cyklus zapisu souboru

      unsigned char write_buff[256];
      bzero(write_buff, 256);

      nread = fread(write_buff,1 , 256, client_file);

      if(nread > 0){
        write(cli_socket, write_buff, nread); // zapis souboru po blocich o 256 bajtech
      }

      if (nread < 256){
            if (ferror(client_file))
            printf("Error reading\n");
            break;
      }

    } // konec zapisu dat
    printf("INFO: file \"%s\" was sent to server on address %s \n", filename, hvalue);
    fclose(client_file);
  } // konec if (wflag == 1)

  close(cli_socket); // uzavreni socketu
  return 0;


}
