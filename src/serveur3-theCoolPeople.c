#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>

#define RCVSIZE 1500 //MTU
#define PORTDATA 6000
#define SQNUMB 6
#define THRESHOLD 256
#define TIMEOUT 1000 //in microseconds
#define FASTRETRANSMIT 4 //Nb maximum d'ACK dupliqués avant transmission

int main(int argc, char * argv[]) {

  struct timeval stop_t, start_t;
  double diff_t;

  //Structure de stockage
  struct sockaddr_in adresse, client;
  struct sockaddr_in adresse_data, client_data;

  //Port et leur description textuelle
  int port = atoi(argv[1]);
  int port_data = 6000;

  //Paramètre des sockets
  int valid = 1;
  int valid_data = 1;
  socklen_t alen = sizeof(client);
  socklen_t alen_data = sizeof(client_data);

  //Permission de passage au processus fils
  int connection = 0;

  //Message envoyé
  char buffer[RCVSIZE];
  char msg[RCVSIZE];

  //int sequence = 0;
  /************************************************/

  //create socket
  int server_desc = socket(AF_INET, SOCK_DGRAM, 0);

  //handle error
  if (server_desc < 0) {
    perror("Cannot create socket\n");
    return -1;
  }

  //Setting de la socket
  if (setsockopt(server_desc, SOL_SOCKET, SO_REUSEADDR, & valid, sizeof(int)) < 0) {
    perror("Cannot set socket\n");
    return -1;
  }

  //Descripteur de socket de connexion
  adresse.sin_family = AF_INET;
  adresse.sin_port = htons(port);
  adresse.sin_addr.s_addr = htonl(INADDR_ANY);

  //initialize socket
  if (bind(server_desc, (struct sockaddr * ) & adresse, sizeof(adresse)) < 0) {
    perror("Bind failed\n");
    close(server_desc);
    return -1;
  }

  //Paramètres du select
  fd_set tab;
  int borne = server_desc + 1;
  FD_ZERO( & tab);
  FD_SET(server_desc, & tab);

  //Début du programe
  while (1) {

    //Mise en place du select
    int ready = select(borne, & tab, NULL, NULL, NULL);

    if (ready < 0) {
      printf("ready value incorrect");
      return -1;
    }

    //Début select
    if (FD_ISSET(server_desc, & tab)) {

      //Reception du message de connexion du client SYN

      long recvlen = recvfrom(server_desc, buffer, RCVSIZE, 0, (struct sockaddr * ) & client, & alen);
      printf("Received packet from %s: %d\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));

      //Vérifiaction de la forme du message
      if (strncmp(buffer, "SYN", sizeof(buffer)) == 0) {
        gettimeofday(&start_t, NULL);
        printf("\n");
        printf("---------------\n");
        printf("SYN recu \n");

        //Ecriture du message de renvoie et du nouveau port
        sprintf(msg, "SYN-ACK%d", port_data);

        printf("\n");
        printf("***********\n");
        printf("Son created\n");

        //*********************************//Socket de donnée

        //Creation socket de donnée
        int server_desc_data = socket(AF_INET, SOCK_DGRAM, 0);

        //Handle error
        if (server_desc_data < 0) {
          perror("Cannot create socket\n");
          return -1;
        }

        //Mise en place de la socket de donnée

        setsockopt(server_desc_data, SOL_SOCKET, SO_REUSEADDR, & valid_data, sizeof(int));

        adresse_data.sin_family = AF_INET;
        adresse_data.sin_port = htons(port_data);
        adresse_data.sin_addr.s_addr = client.sin_addr.s_addr;

        if (bind(server_desc_data, (struct sockaddr * ) & adresse_data, sizeof(adresse_data)) == -1) {
          perror("Bind failed\n");
          close(server_desc_data);
          return -1;
        }

        printf("Data socket created\n");

        //***********************************//

        //Envoie du SYN-ACK
        sendto(server_desc, msg, RCVSIZE, 0, (struct sockaddr * ) & client, sizeof(client));
        printf("Data: %s\n", msg);
        memset(buffer, 0, sizeof(buffer));

        //Reception du ACK
        recvlen = recvfrom(server_desc, buffer, RCVSIZE, 0, (struct sockaddr * ) & client, & alen);
        if(recvlen== -1){
          perror("Can't receive message\n");
          close(server_desc_data);
          return -1;
        }
        if (strncmp(buffer, "ACK", sizeof(buffer)) == 0) {
          memset(buffer, 0, sizeof(buffer));
          connection = 1;
        }
        port_data=port_data+1;

        //Creation processus fils
        pid_t pid = fork();

        if (pid == 0 && connection == 1) {
          connection = 0;
          fd_set tab_data_fprep;

          //Preparation select de reception du premier message
          FD_ZERO( & tab_data_fprep);
          FD_SET(server_desc_data, & tab_data_fprep);

          struct timeval tv_servcli;

          tv_servcli.tv_sec = 5; //temps
          tv_servcli.tv_usec = 0;

          int file_prep = select(server_desc_data + 1, & tab_data_fprep, NULL, NULL, & tv_servcli);

          if (file_prep < 0) {
            printf("ready value incorrect");
            return -1;
          }

          //****************Traitement demande fichier************

          if (FD_ISSET(server_desc_data, & tab_data_fprep) == 1) {
            int recvlen_file = recvfrom(server_desc_data, buffer, RCVSIZE, 0, (struct sockaddr * ) & client_data, & alen_data);
            if(recvlen_file== -1){
              perror("Can't receive message\n");
              close(server_desc_data);
              return -1;
            }
            printf("Fichier demandé est : %s\n", buffer);

          } else {
            printf("out of time");
            close(server_desc_data);
          }

          //***********************
          int windowSize = 30; // nombre de paquets que je veux envoyer en une fois

          //Descripteur textuel des message à envoyer
          char fdata[RCVSIZE - SQNUMB];
          char seq_num[6]; //num seq
          char to_send[RCVSIZE];

          //Numéro du premier et dernier frame envoyés d'une seule fois
          int pUnack = 0; //numéro de séquence des segments non ACKed -1
          int pNext = 0; //numéro de séquence du prochain segment à envoyer -1
          int ack_rcv=0; //ACK recu
          int threshold = THRESHOLD; //ssthresh
          int ack_counter=0; //Compteur d'ACK dupliqué
          int prev_ack=0; //Numéro de la séquence ACK précedemment
          int RTT_seq=1; //Numéro de la séquence dont la reception de l'ACK correspond à la fin du RTT
          int seq=0;


          //Ouverture du fichier
          FILE * fichier = NULL;
          fichier = fopen(buffer, "rb");
          if (fichier == NULL) {
            printf(" Le fichier demandé n'existe pas. \n");
            return (0);
          }

          //Traitement du fichier, calcul de la taille du fichier
          memset(buffer, 0, sizeof(buffer));
          fseek(fichier, 0, SEEK_END);
          int size = (int) ftell(fichier);
          fseek(fichier, 0, SEEK_SET);
          printf("taille du fichier :  %d\n", size);

          //Calcul du nombre de fichier à envoyer
          int nbSeg = (int) size / (RCVSIZE - SQNUMB);

          if (size % (RCVSIZE - SQNUMB) != 0) {
            nbSeg += 1;
          }

          FD_ZERO( & tab_data_fprep);
          FD_SET(server_desc_data, & tab_data_fprep);

          //Début de l'envoie des données du fichier
          while(pUnack<nbSeg){
            //Si la fenêtre n'est pas pleine et que tous les segments n'ont pas encore été envoyés
            if(((pNext-pUnack)<windowSize && pNext<nbSeg)){
                seq = pNext;
                pNext +=1;
              //Traitement des segments de taille MTU-6
              if (seq < nbSeg-1) {
                sprintf(seq_num, "%06d", seq+1);

                //positionnement du pointer en fonction de l'indice
                fseek(fichier,(seq)*(RCVSIZE-SQNUMB),SEEK_SET);

                fread(fdata, 1, RCVSIZE - SQNUMB, fichier);
                memcpy(to_send, seq_num, 6);
                memcpy(to_send + 6, fdata, RCVSIZE - SQNUMB);
                sendto(server_desc_data, to_send, RCVSIZE, 0, (struct sockaddr * ) & client_data, sizeof(client_data));

              //traitement du dernier segment
              } else if(seq == nbSeg-1){

                //Calcul de la taille du dernier segment
                sprintf(seq_num, "%06d", seq+1);
                int rest = size % (RCVSIZE - SQNUMB);
                fseek(fichier,(seq)*(RCVSIZE-SQNUMB),SEEK_SET);

                if (rest == 0) {
                  rest = RCVSIZE - SQNUMB;
                }
                char last[rest];

                //Lecture du dernier morceau de donnée
                fread(last, 1, rest, fichier);
                memcpy(to_send, seq_num, 6);
                memcpy(to_send + 6, last, rest);

                //Envoie de ce morceau
                sendto(server_desc_data, to_send, rest + SQNUMB, 0, (struct sockaddr * ) & client_data, sizeof(client_data));

                memset(fdata, 0, sizeof(fdata));
                memset(to_send, 0, sizeof(to_send));
              }

              struct timeval tvNonBloquant;
              tvNonBloquant.tv_sec = 0;
              tvNonBloquant.tv_usec = 0;

              FD_SET(server_desc_data, & tab_data_fprep);
              //Reception des ACKs s'il y en a avec un select non bloquant
              int ready_data = select(server_desc_data + 1, & tab_data_fprep, NULL, NULL, &tvNonBloquant);

              if (ready_data < 0) {
                perror("ready value incorrect\n");
                return -1;
              }

              //Reception d'ACK
              if (FD_ISSET(server_desc_data, & tab_data_fprep) == 1) {
                memset(buffer, 0, sizeof(buffer));
                int recvlen_ack= recvfrom(server_desc_data, buffer, RCVSIZE, 0, (struct sockaddr * ) & client_data, & alen_data);
                if(recvlen_ack== -1){
                  perror("Can't receive message\n");
                  close(server_desc_data);
                  return -1;
                }
                ack_rcv = atoi(buffer + 3);

                if(pUnack<ack_rcv){
                  pUnack=ack_rcv;
                }

                //changement de la taille de la fenêtre à chaque pseudo-RTT
                //calculé en fonction de la taille de la fenêtre
                if(ack_rcv>=RTT_seq){
                  if(windowSize<threshold){
                    windowSize=windowSize*2;
                  } else{
                    windowSize+=1;
                  }
                  RTT_seq=pNext+1;
                }

                //Incrémentation du compteur d'ACK si dupliqué
                if(ack_rcv==prev_ack){
                  ack_counter+=1;
                  //inflation de la taille de la fenêtre
                  windowSize+=1;
                } else {
                  ack_counter=0;
                }

                prev_ack = ack_rcv;

                //Nombre d'ACK dupliqué trop grand, on retransmet et la Taille
                //de la fenêtre est réduite.
                if(ack_counter==FASTRETRANSMIT){
                  threshold = (pNext-pUnack)*0.5;

                  //Taille minimum du threshold
                  if (threshold<30){
                    threshold=30;
                  }
                  pNext=ack_rcv;
                  RTT_seq=pNext+1;
                  windowSize=threshold;
                }
                FD_ZERO(&tab_data_fprep);
              }

            //Fenêtre pleine
            } else {
              FD_SET(server_desc_data, & tab_data_fprep);

              //attente d'un ACK avec timeout
              struct timeval tvFile;
              tvFile.tv_sec = 0; //temps
              tvFile.tv_usec = TIMEOUT;
              int window_full = select(server_desc_data + 1, & tab_data_fprep, NULL, NULL, &tvFile);

              if (window_full < 0) {
                printf("ready value incorrect (window_full)\n");
                return -1;
              }

              if (FD_ISSET(server_desc_data, & tab_data_fprep) == 1) {
                memset(buffer, 0, sizeof(buffer));
                int recvlen_ackfull = recvfrom(server_desc_data, buffer, RCVSIZE, 0, (struct sockaddr * ) & client_data, & alen_data);
                if(recvlen_ackfull== -1){
                  perror("Can't receive message\n");
                  close(server_desc_data);
                  return -1;
                }
                ack_rcv = atoi(buffer + 3);

                if (pUnack<ack_rcv){
                  pUnack=ack_rcv;
                }

              //changement de la taille de la fenêtre à chaque pseudo-RTT
              //calculé en fonction de la taille de la fenêtre
              if(ack_rcv>=RTT_seq){
                if(windowSize<threshold){
                  windowSize=windowSize*2;
                } else{
                  windowSize+=1;
                }
                RTT_seq=pNext+1;
              }

              //Incrémentation du compteur d'ACK si dupliqué
              if(ack_rcv==prev_ack){
                ack_counter+=1;
              } else{
                ack_counter=0;
              }

              //Nombre d'ACK dupliqué trop grand, on retransmet et la Taille
              //de la fenêtre est réduite.
              if(ack_counter==FASTRETRANSMIT){
                threshold = threshold*0.5;
                if (threshold<30){
                  threshold=30;
                }
                pNext=ack_rcv;
                RTT_seq=pNext+1;
                windowSize=threshold;
              }
                prev_ack = ack_rcv;
                FD_ZERO(&tab_data_fprep);

              //Temps écroulé, pas d'ACK recu, réduction de la fenêtre et retransmission
              } else {
                threshold=threshold*0.5;
                if (threshold<30){
                  threshold=30;
                }
                windowSize=threshold;
                pNext=ack_rcv;
                RTT_seq=pNext+1;
              }
            }
          }
          //Tous les segments sont envoyés
          printf("Sorti de la boucle\n");
          memset(buffer, 0, sizeof(buffer));
          sendto(server_desc_data, "FIN", sizeof("FIN"), 0, (struct sockaddr * ) & client_data, sizeof(client_data));
          printf("Fin de l'envoie\n");
          //calcul du temps
          gettimeofday(&stop_t, NULL);
          diff_t = (stop_t.tv_sec - start_t.tv_sec) + (stop_t.tv_usec - start_t.tv_usec)/ 1000000.0;
          printf("Execution time = %f\n", diff_t);

          close(server_desc_data);
          fclose(fichier);
          return (0);
        }
      }
    }
  }
  close(server_desc);

  return 0;
}
