#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>
#include <time.h>

#include <poll.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "packet.h"

#define CLIENT_PORT 12330
#define SERVER_PORT 12335
#define RELAY1_PORT 12340
#define RELAY2_PORT 12345

FILE* logFile;
int sockR2;

void delay(){

    int delay = rand()%2000;
    float milli_seconds = (float)delay/1000; 
    // printf("Delay by %f ms\n",milli_seconds);
    clock_t start_time = clock();
    while (clock() < start_time + milli_seconds);
}

bool dropPacket(){

    // return false;    // Uncomment to avoid dropping packets!
    int probDrop = rand()%100;
    // printf("probDrop : %d\n",probDrop);

    if(probDrop<PDR){
        return true;
    }else{
        return false;
    }
}

void writeToLog(int seqN,int relay,int type){

   // type - 0 for sendToClient, 1 for recvFromClient, 2 for sendToServer , 3 for recvFromServer, 4 for packetLoss
    logFile = fopen("log.txt","a");

    time_t rawtime;
    struct tm * timeinfo;

    time ( &rawtime );
    timeinfo = localtime ( &rawtime );

    if(type==0)
    fprintf(logFile,"RELAY%d\t\t S \t\t\t\t%02d:%02d:%02d \t\t DATA \t\t %d \t\t RELAY%d \t\t CLIENT\n",relay,timeinfo->tm_hour,timeinfo->tm_min,timeinfo->tm_sec,seqN,relay);
    else if(type==1)
    fprintf(logFile,"RELAY%d\t\t R \t\t\t\t%02d:%02d:%02d \t\t DATA \t\t %d \t\t CLIENT \t\t RELAY%d\n",relay,timeinfo->tm_hour,timeinfo->tm_min,timeinfo->tm_sec,seqN,relay);
    else if(type==2)
    fprintf(logFile,"RELAY%d\t\t S \t\t\t\t%02d:%02d:%02d \t\t DATA \t\t %d \t\t RELAY%d \t\t SERVER\n",relay,timeinfo->tm_hour,timeinfo->tm_min,timeinfo->tm_sec,seqN,relay);
    else if(type==3)
    fprintf(logFile,"RELAY%d\t\t R \t\t\t\t%02d:%02d:%02d \t\t DATA \t\t %d \t\t SERVER \t\t RELAY%d\n",relay,timeinfo->tm_hour,timeinfo->tm_min,timeinfo->tm_sec,seqN,relay);
    else if(type==4)
    fprintf(logFile,"RELAY%d\t\t D \t\t\t\t%02d:%02d:%02d \t\t DATA \t\t %d \t\t CLIENT \t\t RELAY%d \n",relay,timeinfo->tm_hour,timeinfo->tm_min,timeinfo->tm_sec,seqN,relay);
    else{
        printf("Unexpected LogWrite\n");
    }

    fclose(logFile);
}

int main(){
    
    srand(time(0)); 
    

    sockR2 = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);  

    if(sockR2<0){
        printf("Error in opening a first socket\n");
        exit(0);
    } 

    struct sockaddr_in serverAddr;
    struct sockaddr_in clientAddr;
    struct sockaddr_in address;
    int addrLen = sizeof(address);
    int clientAddrlen = sizeof(clientAddr);  
    int serverAddrlen = sizeof(clientAddr);  

    serverAddr.sin_family = AF_INET;   
    serverAddr.sin_addr.s_addr = INADDR_ANY;   
    serverAddr.sin_port = htons(SERVER_PORT); 
        
    clientAddr.sin_family = AF_INET;   
    clientAddr.sin_addr.s_addr = INADDR_ANY;   
    clientAddr.sin_port = htons(CLIENT_PORT);

    struct sockaddr_in relay2Addr;
    memset (&relay2Addr, 0, sizeof (relay2Addr));
    relay2Addr.sin_family = AF_INET;   
    relay2Addr.sin_addr.s_addr = INADDR_ANY;   
    relay2Addr.sin_port = htons(RELAY2_PORT);
    
    if (bind(sockR2, (struct sockaddr *)&relay2Addr, sizeof(relay2Addr))<0)   
    {   
        perror("bind failed");   
        exit(EXIT_FAILURE);   
    }   


    while(true){

        packetNode tempPacket;
        int bytesRecvd = recvfrom(sockR2, &tempPacket, sizeof(packetNode), 0, (struct sockaddr *)&address,(socklen_t*)&addrLen);

        if(tempPacket.seqN==-2){
            printf("\nFile Transfer Complete\n");
            close(sockR2);
            exit(1);
        }

        if(bytesRecvd==-1){
            printf("ERROR : PACKET NOT RECEIVED ON RELAY2\n");
            exit(0);
        }

        if(tempPacket.seqN==-1){
            // dummy packet
            sendto(sockR2,&tempPacket,sizeof(packetNode),0,(struct sockaddr*)&serverAddr,sizeof(serverAddr));
            continue;
        }

        // printf("Relay2 receive Packet-%d from %d\n",tempPacket.seqN,ntohs(address.sin_port));

        if(tempPacket.isAck){

            printf("Relay2 receive ACK Packet-%d from SERVER\n",tempPacket.seqN);

            writeToLog(tempPacket.seqN,2,1);    // receive log
            sendto(sockR2,&tempPacket,sizeof(packetNode),0,(struct sockaddr*)&clientAddr,sizeof(clientAddr));

            printf("Relay2 send ACK Packet-%d to CLIENT\n",tempPacket.seqN);

            writeToLog(tempPacket.seqN,2,0);    // send log

            if(tempPacket.last){
                close(sockR2);
                exit(1);
            }

        }else{

            printf("Relay2 receive Data Packet-%d from CLIENT\n",tempPacket.seqN);

            if(!dropPacket()){
                delay();
                writeToLog(tempPacket.seqN,2,3);    // receive log
                sendto(sockR2,&tempPacket,sizeof(packetNode),0,(struct sockaddr*)&serverAddr,sizeof(serverAddr));
                printf("Relay2 send Data Packet-%d to SERVER\n",tempPacket.seqN);
                writeToLog(tempPacket.seqN,2,2);    // send log
            }else{
                printf("Packet SeqN-%d dropped at Relay2\n",tempPacket.seqN);
                writeToLog(tempPacket.seqN,2,4);
            }
        }

    }

}

