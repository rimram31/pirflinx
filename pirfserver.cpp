#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

#include <pthread.h>
#include <pigpio.h>

#include "pirfserver.h"
#include "rfHandlerV2.h"
#include "Logger.h"

static rfHandlerV2 *aRfHandlerV2 = NULL;

static char outputBuffer[512] = "";
static uint32_t lastMessageTick = 0L;

static char OK[] = "20;00;OK\n";

extern CLogger _log;

void writeRF(char *message) {

    _log.Log(LOG_STATUS,"writeRF: (%s)",message);
    
    // This will call RawSendRF ...
    aRfHandlerV2->sendRFMessage(message);    
    
}

static int clients[] = { -1, -1, -1, -1, -1 };
#define MAX_CLIENTS (sizeof(clients)/sizeof(clients[0]))

int writeClients(char *msg) {
   for (int i=0; i!= MAX_CLIENTS; i++) {
       if (clients[i] > 0) {
           int socket = clients[i];
           _log.Log(LOG_STATUS,"writeClients: (%s)",msg);
           write(socket, msg, strlen(msg));
       }
   }
}

int new_client(int asocket) {
   for (int i=0; i!= MAX_CLIENTS; i++) {
       if (clients[i] < 0) {
           clients[i] = asocket;
           return 1;
       }
   }
   return 0;
}

void remove_client(int asocket) {
   for (int i=0; i!= MAX_CLIENTS; i++) {
       if (clients[i] == asocket)
           clients[i] = -1;
   }
}

int closeClients() {
   for (int i=0; i!= MAX_CLIENTS; i++) {
       if (clients[i] > 0) {
           int socket = clients[i];
           close(socket);
       }
   }
}

/* ----------------------------------------------------------------------- */
void *connection_handler(void *socket_desc) {
    //Get the socket descriptor
    int sock = *(int*)socket_desc;
    int read_size;
    char client_message[2000];
     
    //Receive a message from client
    while( (read_size = recv(sock , client_message , 2000 , 0)) > 0 ) {
        //end of string marker
        client_message[read_size] = '\0';

        // A message received from client ...
        
        char *dup = strdup(client_message);
        char *line, *tofree;
        if (dup!=NULL) {
            tofree = dup;
            while((line = strsep(&dup,"\n")) != NULL) {
                if (strlen(line) > 0)
                    writeRF(line);
            }
            free(dup);
        }
        // Return OK to client
        //_log.Log(LOG_STATUS,"writeClient: (%s)",OK);
        write(sock, OK, strlen(OK));
        //writeRF(client_message);

        //clear the message buffer
        memset(client_message, 0, 2000);
    }

    // remove client
    remove_client(sock);

    // Connection closed by the client 
    if (read_size == 0) {
        fflush(stdout);
    } else if(read_size == -1) {
        perror("recv failed");
    }
    return 0;
}

void *accept_connection_handler(void *socket_server) {
   int sock = *(int*)socket_server;
   struct sockaddr_in caddress;
   int sock_client, caddress_size;

   pthread_t thread_id;
   
   caddress_size = sizeof(struct sockaddr_in);
   while(sock_client = accept(sock,(struct sockaddr *)&caddress,(socklen_t*)&caddress_size)) {
       // Add new client
       if (new_client(sock_client)) {
           if( pthread_create(&thread_id,NULL, connection_handler ,(void*) &sock_client) < 0) {
               //fprintf(stderr, "pthread_create socket failed\n");
               ;
           }
       } else
           // Cannot acccept a new connection ...
           close(sock_client);
   }
}

pthread_t pirfserver_thread;
int b_isStopped = false;
pthread_t pirfsocket_thread;

#define REPEAT_DELAY (1000*1000L)   // Delay 500 ms -> 1s

void *pirfserver_work(void *parm) {

    _log.Log(LOG_STATUS,"start pirfserver_work");
    // Infinite loop
    while (!b_isStopped) {
        // An event on the stack ?
        OokDecoder *pdecoderFound = aRfHandlerV2->popMatches();
        if (pdecoderFound!=NULL) {
            char *out = pdecoderFound->display();
            // If something happen, a RFlink line is in outputBuffer
            if (strlen(out) > 0) {
                uint32_t tick = gpioTick();
                _log.Log(LOG_STATUS,"receiveRF: %s",outputBuffer);
                // The delay mecanism prevent to send several time the same message
                if (strcmp(outputBuffer,out) || ((tick - lastMessageTick) > REPEAT_DELAY)) {
                    outputBuffer[0] = '\0';
                    memset(outputBuffer, 0, sizeof(outputBuffer));
                    strcpy(outputBuffer,out);
                    // Write this line to all clients
                    writeClients(outputBuffer);
                    lastMessageTick = tick;
                }
            }
        }
        gpioDelay(100000); /* check remote 10 times per second */
    }
    
}

int pirfserver_start(int socketPort) {
   int sock = -1;
   struct sockaddr_in address;

    _log.Log(LOG_STATUS,"start pirfserver_start");
    
   // Initialisations 
   // pigpio init
   if (gpioInitialise() < 0)
      return 1 ;
    
   /* create socket */
   sock = socket(AF_INET , SOCK_STREAM , 0);
   //sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
   if (sock <= 0)
       return 1;

   /* bind socket to port */
   address.sin_family = AF_INET;
   address.sin_addr.s_addr = INADDR_ANY;
   address.sin_port = htons(socketPort);
   if (bind(sock, (struct sockaddr *)&address, sizeof(struct sockaddr_in)) < 0)
       return 1;

   int option = 1;
   if (setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,(char*)&option,sizeof(option)))
//   if (setsockopt(sock,SOL_SOCKET,(SO_REUSEPORT | SO_REUSEADDR),(char*)&option,sizeof(option)))
       return 1;
    
   listen(sock,10);

   if( pthread_create(&pirfsocket_thread,NULL, accept_connection_handler ,(void*) &sock) < 0)
       return 1;

   aRfHandlerV2 = new rfHandlerV2(DATA_PIN,TRSEL_PIN,RSST_PIN,PWDN_PIN);

   b_isStopped = false;
   pthread_create(&pirfserver_thread,NULL, pirfserver_work, NULL);
   
   return 0;
}

int pirfserver_stop() {
    
   pthread_join(pirfserver_thread, NULL);
   
   gpioTerminate();
   closeClients();
   
   delete(aRfHandlerV2);
   
   return 0;
}
