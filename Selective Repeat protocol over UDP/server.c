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

int packetsReceived=0;

FILE* logFile;

struct sockaddr_in address;
int addrLen = sizeof(address);

packetNode buffer[BUFFERSIZE];

packetNode windowBuffer[WINDOW_SIZE];


int bufLen = 0;

FILE* outputFp;
int sock;

packetNode* makeAckPacket(int seqN){

    packetNode* packet = malloc(sizeof(packetNode));
    packet->isAck=1;
    packet->seqN = seqN;
    packet->size = PACKET_SIZE;
    packet->last = false;
    return packet;
}

void writeToLog(int seqN,int relay,int type){

    logFile = fopen("log.txt","a");
    // type - 0 for send ack, 1 for receive data
    time_t rawtime;
    struct tm * timeinfo;

    time ( &rawtime );
    timeinfo = localtime ( &rawtime );

    if(type==0)
    fprintf(logFile,"SERVER\t\t S \t\t\t\t%02d:%02d:%02d \t\t DATA \t\t %d \t\t SERVER \t\t RELAY%d\n",timeinfo->tm_hour,timeinfo->tm_min,timeinfo->tm_sec,seqN,relay);
    else if(type==1)
    fprintf(logFile,"SERVER\t\t R \t\t\t\t%02d:%02d:%02d \t\t DATA \t\t %d \t\t RELAY%d \t\t SERVER\n",timeinfo->tm_hour,timeinfo->tm_min,timeinfo->tm_sec,seqN,relay);
    else{
        printf("Unexpected LogWrite\n");
    }

    fclose(logFile);
}

void printBufferArray(){
    printf("Buffer : [");
    for(int i=0;i<bufLen;i++){
        printf("%d ",buffer[i].seqN);
    }
    printf("]\n");
}

void shiftBufferArray(int i){
    while(i<bufLen-1){
        buffer[i] = buffer[i+1];
        i++;
    }
    buffer[i].seqN = -1;
}

void addBufferedData(){
    int i;
    for(i=0;i<bufLen;i++){
        if(buffer[i].seqN == packetsReceived){
            // printf("Adding Data to Output File SeqN-%d : %s\n",buffer[i].seqN,buffer[i].data);
            // printf("Adding to Output SeqN=%d\n",buffer[i].seqN);
            fwrite(buffer[i].data , sizeof(char) , buffer[i].size , outputFp);
            packetNode* ackPacket = makeAckPacket(buffer[i].seqN);
            // printf("CHECK2 : Trying to Send to Port %d\n",ntohs(address.sin_port));
            // printf("Data: Received from %s:%d\n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));

            if(buffer[i].last){
                ackPacket->last = true;
            }

            int sentBytes = sendto(sock,ackPacket,sizeof(packetNode),0,(struct sockaddr*)&address,addrLen);

            if(ntohs(address.sin_port)==RELAY1_PORT){
                writeToLog(ackPacket->seqN,1,0);   // receive from relay1
            }else if(ntohs(address.sin_port)==RELAY2_PORT){
                writeToLog(ackPacket->seqN,2,0);   // receive from relay1
            }else{
                printf("Unexpected Send at Server\n");
                exit(0);
            }

            if(buffer[i].last==true){
                close(sock);
                printf("ACK Sent for Last Packet\n");
                printf("\nFile Transfer Complete\n");
                exit(0);
            }

            
            if(sentBytes==-1){
                printf("ERROR : ACK PACKET SeqN - %d NOT SENT\n",ackPacket->seqN);
                exit(0);
            }
            printf("SENT ACK PACKET : SeqNo - %d FROM SERVER\n",ackPacket->seqN);
            packetsReceived++;

            shiftBufferArray(i);
        }
    }

    if(bufLen!=0)
    bufLen--;
}


int addToBufferOrOutput(packetNode* dataPacket){

    if(packetsReceived==dataPacket->seqN){
            // Correct Sequence
            // printf("Adding Data to Output File SeqN-%d : %s\n",dataPacket->seqN,dataPacket->data);
            // printf("Adding to Output SeqN=%d\n",dataPacket->seqN);
            fwrite(dataPacket->data , sizeof(char) , dataPacket->size , outputFp);
            packetsReceived++;
            // Send ACK Packet
            packetNode* ackPacket = makeAckPacket(dataPacket->seqN);
            // send(sockNum,ackPacket,sizeof(packetNode),0);
            // printf("CHECK : Trying to Send to Port %d\n",ntohs(address.sin_port));
            // printf("Data: Received from %s:%d\n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));

            // struct pollfd fd;
            // int ret;

            // fd.fd = sock; // your socket handler 
            // fd.events = POLL_OUT;
            // ret = poll(&fd, 1, 5000); // 5 second for timeout

            // if(ret<=0){
            //     printf("ERROR : Server not able to send ACK\n");
            //     exit(0);
            // }

            if(dataPacket->last){
                ackPacket->last = true;
            }

            int sentBytes = sendto(sock,ackPacket,sizeof(packetNode),0,(struct sockaddr*)&address,addrLen);

            if(ntohs(address.sin_port)==RELAY1_PORT){
                writeToLog(ackPacket->seqN,1,1);   // receive from relay1
            }else if(ntohs(address.sin_port)==RELAY2_PORT){
                writeToLog(ackPacket->seqN,2,1);   // receive from relay1
            }else{
                printf("Unexpected Send at Server\n");
                exit(0);
            }


            if(sentBytes==-1){
                printf("ERROR : ACK PACKET SeqN - %d NOT SENT\n",ackPacket->seqN);
                exit(0);
            }
            printf("SENT ACK PACKET : SeqNo - %d FROM SERVER\n",ackPacket->seqN);

            addBufferedData();

            if(dataPacket->last==true){
                close(sock);
                printf("ACK Sent for Last Packet\n");
                printf("\nFile Transfer Complete\n");
                exit(0);
            }

            return 1;
    }

    if(bufLen<BUFFERSIZE){
        // printf("Adding to Buffer SeqN=%d - bufLen=%d\n",dataPacket->seqN,bufLen);
        buffer[bufLen++] = *dataPacket;
        return bufLen;
    }
    else{
        // printf("Buffer is FULL\n\n");
        return -1;
    }
}

void addWindowPackets(){
    for(int i=0;i<WINDOW_SIZE;i++){

        if(windowBuffer[i].seqN==-1)
        break;

        addToBufferOrOutput(&windowBuffer[i]);

        if(windowBuffer[i].last)
        break;
    }
    memset(windowBuffer, 0, sizeof(windowBuffer));
}

int main(){
    
    outputFp = fopen("output.txt","w");

    sock = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
    if(sock<0){
        printf("Error in opening a first socket\n");
        exit(0);
    } 

    printf("Server Socket Created\n");
    
    struct sockaddr_in serverAddr;


    memset (&serverAddr, 0, sizeof (serverAddr));
    serverAddr.sin_family = AF_INET;   
    serverAddr.sin_addr.s_addr = INADDR_ANY;   
    serverAddr.sin_port = htons(SERVER_PORT); 

    if (bind(sock, (struct sockaddr *)&serverAddr, sizeof(serverAddr))<0)   
    {   
        perror("bind failed");   
        exit(EXIT_FAILURE);   
    }   

    while(true){
        
        // packetNode tempPacket;
        int i=0;

        while(i<WINDOW_SIZE){
            int bytesRecvd = recvfrom(sock, &windowBuffer[i], sizeof(packetNode), 0, (struct sockaddr *)&address,(socklen_t*)&addrLen);

            if(windowBuffer[i].seqN==-1){
                // printf("Dummy Packet\n");
                break;
            }

            if(ntohs(address.sin_port)==RELAY1_PORT){
                writeToLog(windowBuffer[i].seqN,1,1);   // receive from relay1
            }else if(ntohs(address.sin_port)==RELAY2_PORT){
                writeToLog(windowBuffer[i].seqN,2,1);   // receive from relay1
            }else{
                printf("Unexpected Receive at Server\n");
                exit(0);
            }
            printf("Server receive Packet-%d from %d\n",windowBuffer[i].seqN,ntohs(address.sin_port));
            // if(windowBuffer[i].last){
            //     break;
            // }
            i++;
        }

        addWindowPackets();
        // addToBufferOrOutput(&tempPacket);
    }

    close(sock);
    printf("\nFile Transfer Complete\n");
    fclose(outputFp);
}