/**
 * Author:    PETR KNETL
 * Created:   1.03.2018
 *
 * Project: School project 1 to IPK class, basic client-server file transport protocol
 **/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <net/ethernet.h>
#include <arpa/inet.h>


 int main(int argc, char* argv[]){
   if (argc != 3) { // spatny pocet parametru
     perror("ERROR: False number of parameters\n");
     exit(1);
   }

  int par, pflag; // pomocne promenne pro getopt()
  char *pvalue = NULL;
  int pid; // promenna pro cislo precesu ditete

  while ((par = getopt (argc, argv, "p:")) != -1) //zpracovani parametru
     switch (par)
       {
       case 'p':
         pflag = 1;
         pvalue = optarg;
         break;

       case '?':
         if (optopt == 'p')
           fprintf (stderr, "Option -%c requires an argument.\n", optopt);
         else if (isprint (optopt)){
           fprintf (stderr, "Unknown option `-%c'.\n", optopt);
         }
         else
           fprintf (stderr,
                    "Unknown option character `\\x%x'.\n",
                    optopt);
         exit(4);
       default:
         abort ();
       }

  if(pflag != 1){ // chybejici paramter
    printf("ERROR: missing parameter \"-p\"\n");
    exit(1);
  }

  int rflag = 0; // pomocne flagy
  int wflag = 0;
  int nread;

  char communication_buffer[256]; //buffer pro prenos zprav o zpusobu komunikace
  char filename[252];
  char RorW[2]; // flag zapis nebo cteni


  int ser_socket; // hlavni (welcome) SERVER SOCKET
  int comm_socket; //socket pro prenos dat
  int port = atoi(pvalue); // PORT
  socklen_t clilen;

  struct sockaddr_in serv_addr, cli_addr; //ADRESY

 /* SOCKET */
  if((ser_socket = socket(AF_INET, SOCK_STREAM,0)) <= 0){ //inicializace socketu
    perror("ERROR: unable to create socket\n");
    exit(2);
  }

  setsockopt(ser_socket, SOL_SOCKET, SO_REUSEADDR, (const void *)1, sizeof(int));

 /* BIND */
  bzero((char *) &serv_addr, sizeof(serv_addr)); //nastaveni hodnot adresy
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons((unsigned short) port);
  serv_addr.sin_addr.s_addr = INADDR_ANY;

  if(bind(ser_socket, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0){ // navazani adresy na socket
    perror("ERROR: unable to bind socket and address\n");
    exit(2);
  }

  printf("INFO: server inicialized and ready for connection(s) by client(s) \n");

  /* LISTEN */
  if ( listen(ser_socket, 5) < 0 ){ // prijimani requestu
    perror("ERROR: listen failed\n");
    exit(2);
  }

  /* ACCEPT */
  while (1){

    clilen = sizeof(cli_addr);
    comm_socket = accept(ser_socket, (struct sockaddr*) &cli_addr, &clilen);
    printf("INFO: new client connected from address %s\n",inet_ntoa(cli_addr.sin_addr));

    pid = fork();// rozdeleni procesu na podprocesy

    if (pid  == 0){ // proces ditete
      close(ser_socket);

      bzero(communication_buffer, 256);
      nread = read(comm_socket, communication_buffer, 256); // prijem informaci o zpusobu komunikace ze strany klienta
      if (nread < 0){
        printf("ERROR: unable to read data from client");
        exit(3);
      }

      bzero(RorW, 2);
      bzero(filename, 252);

      //nastaveni parametru komunikace
      strncpy(RorW, communication_buffer, 1);
      strcpy(filename, communication_buffer + 1);


      if(strcmp(RorW, "r") == 0){
        rflag = 1;
      }
      else if(strcmp(RorW, "w") == 0){
        wflag = 1;
      }

      /*CTENI ZE SERVERU*/
      if (rflag == 1){
        rflag = 0;

        FILE *server_file = fopen(filename, "rb");
        if (server_file == NULL){
          bzero(communication_buffer, 256);
          strcpy(communication_buffer, "ERRFILE"); //odeslani zpravy klientovi o selhani otevreni souboru
          int r = write(comm_socket, communication_buffer, strlen(communication_buffer));
          if (r < 0){
            perror("ERROR: unable to write to client\n");
            close(comm_socket);
            exit(3);
          }
          fprintf(stderr, "ERROR: fopen() for file \"%s\" failed\n", filename);
          close(comm_socket);
          exit(3);
      }

        while(1){ // cyklus odesilani dat klientovi

          unsigned char write_buff[256]; // zapis po blocich o 256 bajtech
          bzero(write_buff, 256);

          nread = fread(write_buff,1 , 256, server_file); //cteni ze souboru

          if(nread > 0){
            write(comm_socket, write_buff, nread); // odeslani
          }

          if (nread < 256){
            if (ferror(server_file))
            printf("Error reading\n");
            break;
          }


        }
        printf("INFO: file \"%s\" sucessfuly sent to client (%s) \n", filename, inet_ntoa(cli_addr.sin_addr));
        fclose(server_file); // zavreni souboru
      } // konec cteni

      /* ZAPIS NA SERVER*/
      else if(wflag == 1){

        wflag = 0;
        unsigned char read_buff[256];
        int incomingBytes;

        FILE *server_file = fopen(filename, "wb");
        if (server_file == NULL){
          perror("ERROR: fopen() failed\n");
          exit(3);
        }

        //cteni dat ze socketu po blocich o 256 bajtech
        while((incomingBytes = read(comm_socket, read_buff, 256)) > 0){
          fwrite(read_buff, 1,incomingBytes, server_file); //zapis do souboru
        }

        if(incomingBytes < 0){
          printf(" ERROR: cannot read the file \n");
        }

        printf("INFO: file \"%s\" sucessfuly downloaded from client (%s) \n", filename, inet_ntoa(cli_addr.sin_addr));
        fclose(server_file);

      } // konec zapisu

      close(comm_socket);
      exit(0);
    }// konec forku
    else if(pid < 0){
      perror("ERROR: unable to fork process...");
      exit(4);
    }
    else{ // proces rodice
      close(comm_socket);
    }
  }// konec hlavniho while cyklu

  close(comm_socket); // uzavreni socketu
  close(ser_socket); // uzavreni socketu

   return 0;
}
