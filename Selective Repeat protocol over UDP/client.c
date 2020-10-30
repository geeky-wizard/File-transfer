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


// Global Variables
packetNode* packets;    // Packet array -global
int num_packets;    // No. of Packets -global
int sequenceNo = 0;         // This will tell us how many packets were tried to be sent!
int acksReceived = 0;     // This will tell us how many packets were successfully sent!
FILE* logFile;

int window[WINDOW_SIZE] = {-1};
bool ackWindow[WINDOW_SIZE] = {false};

void printWindow(){

    printf("\n[");

    for(int i=0;i<WINDOW_SIZE;i++){
        printf("%d  ",window[i]);
    }

    printf("]\n");

    printf("[");

    for(int i=0;i<WINDOW_SIZE;i++){
        printf("%d  ",ackWindow[i]);
    }

    printf("]\n");

}

int findSize(char* fileName) 
{ 
    FILE* fp = fopen(fileName,"r");

    if (fp == NULL) { 
        printf("Input File Not Found!\n"); 
        return -1; 
    } 
  
    fseek(fp, 0, SEEK_END);
    int res = ftell(fp); 
  
    // closing the file 
    fclose(fp); 
  
    return res; 
} 

packetNode* makePacket(char* data,int seqN){

    packetNode* packet = malloc(sizeof(packetNode));
    packet->isAck=0;
    // printf("Add Data to Packet : %s - %d\n",data,strlen(data));
    strcpy(packet->data,data);
    packet->seqN = seqN;
    packet->size = PACKET_SIZE;

    if(seqN==num_packets-1)
        packet->last = true;
    else
        packet->last = false;
        
    return packet;
}

void read_data(packetNode* packetArr,char* fileName){

    FILE* fp = fopen(fileName,"r");

    int i=0;
    
    while(fp!=NULL && i<num_packets){
        char data[PACKET_SIZE];
        fread(data, sizeof(char), PACKET_SIZE, fp);
        packetArr[i] = *makePacket(data,i);
        //printf("Packet %d : isLast-%d\n",i+1,packetArr[i].last);
        i++;
    }

}

void writeToLog(int seqN,int relay,int type){

    // type - 0 for send, 1 for receive ack, 2 for timeOut , 3 for reTransMission
    logFile = fopen("log.txt","a");
    time_t rawtime;
    struct tm * timeinfo;

    time ( &rawtime );
    timeinfo = localtime ( &rawtime );

    if(type==0)
    fprintf(logFile,"CLIENT\t\t S \t\t\t\t%02d:%02d:%02d \t\t DATA \t\t %d \t\t CLIENT \t\t RELAY%d\n",timeinfo->tm_hour,timeinfo->tm_min,timeinfo->tm_sec,seqN,relay);
    else if(type==1)
    fprintf(logFile,"CLIENT\t\t R \t\t\t\t%02d:%02d:%02d \t\t DATA \t\t %d \t\t RELAY%d \t\t CLIENT\n",timeinfo->tm_hour,timeinfo->tm_min,timeinfo->tm_sec,seqN,relay);
    else if(type==2) // TODO : Doubt in this case write
    fprintf(logFile,"CLIENT\t\t TO \t\t\t%02d:%02d:%02d \t\t DATA \t\t %d \t\t CLIENT \t\t RELAY%d\n",timeinfo->tm_hour,timeinfo->tm_min,timeinfo->tm_sec,seqN,relay);
    else if(type==3)
    fprintf(logFile,"CLIENT\t\t RE \t\t\t\t%02d:%02d:%02d \t\t DATA \t\t %d \t\t CLIENT \t\t RELAY%d\n",timeinfo->tm_hour,timeinfo->tm_min,timeinfo->tm_sec,seqN,relay);
    else{
        printf("Unexpected LogWrite\n");
    }

    fclose(logFile);
}

void shiftWindow(){

    int i=0;

    while(ackWindow[i]==true && i<WINDOW_SIZE){
        window[i] = -1;
        ackWindow[i] = false;
        i++;
    }
    

    if(i==WINDOW_SIZE){
        // Acks received for the whole window
        for(int i=0;i<WINDOW_SIZE;i++){
            if(sequenceNo==num_packets)
            return;

            window[i] = sequenceNo++;
        }
        return;
    }
    // -1 -1 3 4 -> 3 4 5 6
    // i=2

    if(window[0]!=-1)
    return;


    int j=0;

    while(i<WINDOW_SIZE && window[j]!=window[i]){
        window[j++] = window[i++];

    }    

    // if(window[j]==num_packets-1)
    // return;

    while(j<WINDOW_SIZE && sequenceNo<num_packets){
        window[j++] = sequenceNo++;
    }

    while(j<WINDOW_SIZE){
        window[j++] = -1;
    }

}

int main(){
    
    logFile = fopen("log.txt","w");
    fprintf(logFile,"NodeName    \tEventType       \t\tTimeStamp      \t\tPacketType  \tSeqNo    \tSource          \tDestination\n\n");
    fclose(logFile);

    char* fileName = "input.txt";

    int fileSize = findSize(fileName);
    num_packets = ceil((double)fileSize / PACKET_SIZE);

    printf("FileSize = %d bytes\tNumber of Packets = %d\n",fileSize,num_packets);   // TODO : remove num packets print
    packets = (packetNode*) malloc(sizeof(packetNode)*num_packets);
    read_data(packets,fileName);

    int sock = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);    // will connect to relay1
    if(sock<0){
        printf("Error in opening a first socket\n");
        exit(0);
    } 

    printf("Client Socket Created\n");

    // Construct Server Address Structure
    struct sockaddr_in serverAddr_R1;
    struct sockaddr_in serverAddr_R2;

    memset(&serverAddr_R1,0,sizeof(serverAddr_R1));
    memset(&serverAddr_R2,0,sizeof(serverAddr_R2));

    serverAddr_R1.sin_family = AF_INET;
    serverAddr_R1.sin_port = htons(RELAY1_PORT); 
    serverAddr_R1.sin_addr.s_addr = INADDR_ANY;    // IP of Relay1

    serverAddr_R2.sin_family = AF_INET;
    serverAddr_R2.sin_port = htons(RELAY2_PORT);
    serverAddr_R2.sin_addr.s_addr = INADDR_ANY;    // IP of Relay2


    struct sockaddr_in clientAddr; 
    memset (&clientAddr, 0, sizeof (clientAddr));
    clientAddr.sin_family = AF_INET;   
    clientAddr.sin_addr.s_addr = INADDR_ANY;   
    clientAddr.sin_port = htons(CLIENT_PORT);

    if (bind(sock, (struct sockaddr *)&clientAddr, sizeof(clientAddr))<0)   
    {   
        perror("bind failed");   
        exit(EXIT_FAILURE);   
    }   

    // Server's IP Address here
    // printf("Address assigned\n");

    // Odd Packets go through sock1 to Relay1
    // Even Packets go through sock2 to Relay2

    packetNode ackPacket;
    struct pollfd fd;
    int ret;


    while(acksReceived<num_packets){

        int bytesSent;

        if(window[0]==-1){
            // shift window
            shiftWindow();
        }

        // printWindow();

        // if window[0]!=-1 , then 3 4 -1 5 6

        // Check if base of window shifted!

        int i=0;

        if(window[0]!=-1){

            // May send window again if ACK Received out of order!

            while(i<WINDOW_SIZE){

                if(window[i]==-1){
                    packetNode dummy;
                    dummy.seqN = -1;
                    bytesSent = sendto(sock,&dummy,sizeof(packetNode),0,(struct sockaddr*)&serverAddr_R1,sizeof(serverAddr_R1));
                    break;
                }

                if(window[i]%2==0){
                    writeToLog(window[i],1,0);
                    bytesSent = sendto(sock,&packets[window[i]],sizeof(packetNode),0,(struct sockaddr*)&serverAddr_R1,sizeof(serverAddr_R1));
                    printf("SENT PACKET : SeqNo - %d FROM CLIENT\n",window[i]);
                }else{
                    writeToLog(window[i],2,0);
                    bytesSent = sendto(sock,&packets[window[i]],sizeof(packetNode),0,(struct sockaddr*)&serverAddr_R2,sizeof(serverAddr_R2));
                    printf("SENT PACKET : SeqNo - %d FROM CLIENT\n",window[i]);
                }     

                if(bytesSent==-1){
                    printf("ERROR : Couldn't Send packet %d\n",window[i]);
                    exit(0);
                }

                i++;
            }
        }else{
            printf("Unexpected Error : window[0]\n");
            exit(0);
        }

        // int startTime = clock();

        fd.fd = sock; // your socket handler 
        fd.events = POLLIN;
        ret = poll(&fd, 1, RETRANS_TIME*1000); // 2 second for timeout

        // printf("Check Poll Time : %f \n",(float)(startTime-clock())/CLOCKS_PER_SEC);

        if(ret<=0){
            printf("Ack Timeout\n");
            // Resend All Packets in Window
            writeToLog(window[0],1,2);
            continue;
        }

        // ACK Received
        recv(sock,&ackPacket,sizeof(packetNode), 0);
        printf("RECV PACKET : SeqNo - %d IN CLIENT\n",ackPacket.seqN);

        // Check if ACK Is Out or Order
        

        for(int i=0;i<WINDOW_SIZE;i++){
            if(window[i]==ackPacket.seqN){
                window[i] = -1;
                ackWindow[i] = true;
                // ACK RECEIVED FOR window[i]
                acksReceived++;
                // if(acksReceived==ackPacket.seqN){
                //     printf("ACK RECEIVED IN ORDER\n");
                //     // acksReceived++;
                // }
                writeToLog(ackPacket.seqN,1,1);

                break;
            }
        }

    }

    packetNode dummy;
    dummy.seqN = -2;    // To End Relay1 and Relay2 Program
    int bytesSent = sendto(sock,&dummy,sizeof(packetNode),0,(struct sockaddr*)&serverAddr_R1,sizeof(serverAddr_R1));
    bytesSent = sendto(sock,&dummy,sizeof(packetNode),0,(struct sockaddr*)&serverAddr_R2,sizeof(serverAddr_R2));

    printf("\nFile Transfer Complete\n");
    close(sock);

}
