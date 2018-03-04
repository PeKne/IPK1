#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

 int main(int argc, char* argv[]){
   if (argc != 3) {
     perror("ERROR: False number of parameters\n");
     return 4;
   }

  int par, pflag; // pomocne promenne pro getopt()
  char *pvalue = NULL; // pomocne promenne pro getopt()

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
         return 4;
       default:
         abort ();
       }

  if(pflag != 1){
    return 4;
  }

  //TODO: check ze port je cislo

  int RWswitch;
  int rflag = 0;
  int wflag = 0;
  char message[6];

  int ser_socket; // SERVER SOCKET
  int port = atoi(pvalue); // PORT

  struct sockaddr_in serv_addr, cli_addr; //ADRESY

 /* SOCKET */
  if((ser_socket = socket(AF_INET, SOCK_STREAM,0)) <= 0){ //inicializace socketu
    perror("ERROR: unable to create socket\n");
    return 1;
  }
  setsockopt(ser_socket, SOL_SOCKET, SO_REUSEADDR, (const void *)1, sizeof(int));

 /* BIND */
  bzero((char *) &serv_addr, sizeof(serv_addr)); //nastaveni hodnot adresy
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons((unsigned short) port);
  serv_addr.sin_addr.s_addr = INADDR_ANY;

  if(bind(ser_socket, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0){ // navazani adresy na socket
    perror("ERROR: unable to bind socket adn address\n");
    return 2;
  }

  /* LISTEN */
  if ( listen(ser_socket, 1) < 0 ){ // prijimani requestu
    perror("ERROR: listen failen\n");
    return 3;
  }

  /* ACCEPT */
  socklen_t clilen = sizeof(cli_addr);
  int comm_socket;
  while (1){ // TODO: aktivni cekani

    comm_socket = accept(ser_socket, (struct sockaddr*) &cli_addr, &clilen);

    bzero(message, 6);
    RWswitch = read(comm_socket, message, 5);
    if(RWswitch < 0){
      perror("chyba message\n");
      return 56434;
    }

    if(strcmp(message, "read") == 0){
      rflag = 1;
    }
    else if(strcmp(message, "write") == 0){
      wflag = 1;
    }

    /*CTENI ZE SERVERU*/
    if (rflag == 1){
      FILE *server_file = fopen("testfile.txt", "rb");
      if (server_file == NULL){
        perror("ERROR: fopen() failed\n");
        return 7945;
      }

      while(1){

        unsigned char write_buff[256];
        bzero(write_buff, 256);

        int nread = fread(write_buff,1,256, server_file);

        if(nread > 0){
          printf("Sending \n");
          write(comm_socket, write_buff, nread);
        }

        if (nread < 256){
              if (ferror(server_file))
              printf("Error reading\n");
              break;
        }

      }
    } // konec cteni

    /* ZAPIS NA SERVER*/
    else if(wflag == 1){
      printf("vse funguje\n");
    } // konec zapisu

    if (comm_socket < 0){
      perror("ERROR: Accept failed \n");
      close(ser_socket);
      return 5;
    }

    if (shutdown(comm_socket, SHUT_RDWR) == -1) {
        perror("ERROR: shutdown failed\n");
        close(comm_socket);
        close(ser_socket);
        return 6;
      }

  }
  close(comm_socket); // uzavreni socketu
  close(ser_socket); // uzavreni socketu

   return 0;
}
