#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>

#include <poll.h>
#include <sys/socket.h>
// for socket(), connect(), send(), recv()
#include <arpa/inet.h>
// different address structures are declared
#include <unistd.h>

#include "utilsDef.h"

// Global Variables
packetNode* packets;    // Packet array -global
int num_packets;    // No. of Packets -global
int sequenceNo = 0;         // This will tell us how many packets were tried to be sent!
int acksReceived = 0;     // This will tell us how many packets were successfully sent!

int ch1_wait=-1;
int ch2_wait=-1;

int findSize(char* fileName) 
{ 
    FILE* fp = fopen(fileName,"r");

    if (fp == NULL) { 
        printf("File Not Found!\n"); 
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

    if(seqN==num_packets)
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
        packetArr[i] = *makePacket(data,i+1);
        // printf("Packet %d : %d\n",i+1,packetArr[i].seqN);
        i++;
    }

}

bool getAckPacket(int sockNum){

        packetNode ackPacket;
        struct pollfd fd;
        int ret;

        fd.fd = sockNum; // your socket handler 
        fd.events = POLL_IN;
        ret = poll(&fd, 1, 2000); // 2 second for timeout
        switch (ret) {
            case -1:
                // Error
                printf("Ack Error\n");
                return false;
            case 0:
                // Timeout 
                printf("Ack Timeout\n");
                return false;
            default:
                recv(sockNum,&ackPacket,sizeof(packetNode), 0); // get your data
                printf("ACK Received SeqN = %d\n",ackPacket.seqN);
                return true;
                }
}
