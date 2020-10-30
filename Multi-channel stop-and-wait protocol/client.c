#include "packet.h"

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


int main(){

    char* fileName = "input.txt";
    
    num_packets = ceil((double)findSize(fileName) / PACKET_SIZE);
    // printf("Number of packets required : %d\n",num_packets);    // TODO : remove num packets print
    packets = (packetNode*) malloc(sizeof(packetNode)*num_packets);
    

    read_data(packets,fileName);

    // printf("Data Read!\n");

    int sock1 = socket(PF_INET,SOCK_STREAM,IPPROTO_TCP);
    if(sock1<0){
        printf("Error in opening a first socket\n");
        exit(0);
    } 

    int sock2 = socket(PF_INET,SOCK_STREAM,IPPROTO_TCP);   
    if(sock2<0){
        printf("Error in opening a second socket\n");
        exit(0);
    }

    printf("Client Sockets Created\n");

    // Construct Server Address Structure
    struct sockaddr_in serverAddr;
    memset(&serverAddr,0,sizeof(serverAddr));

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT); // Change Port Number
    serverAddr.sin_addr.s_addr = INADDR_ANY;    // Mac IP
    // Server's IP Address here
    printf("Address assigned\n");

    //  Establish Connection for socket 1
    int c1 = connect(sock1,(struct sockaddr*) &serverAddr, sizeof(serverAddr));
    if(c1<0){
        printf("Error while establishing connection for Socket1\n");
        exit(0);
    }

    int c2 = connect(sock2,(struct sockaddr*) &serverAddr, sizeof(serverAddr));
    if(c2<0){
        printf("Error while establishing connection for Socket1\n");
        exit(0);
    }

    printf("Connection Established\n");

    // Sending Packets

    while(acksReceived<num_packets){

        while(acksReceived<sequenceNo){

            // printf("ACKS TILL NOW : %d\n",acksReceived);

            packetNode ackPacket;
            struct pollfd fd;
            int ret;

            // Wait 2 sec for ack
            if(ch1_wait>=0 && ch1_wait<ch2_wait){

                // printf("ch1_wait for seq : %d\n",ch1_wait+1);

                fd.fd = sock1; // your socket handler 
                fd.events = POLLIN;
                ret = poll(&fd, 1, RETRANS_TIME*1000); // 2 second for timeout

                if(ret<=0){
                    printf("Ack Timeout\n");
                    // Resend Packet
                    int bytesSent = send(sock1,&packets[ch1_wait],sizeof(packetNode),0);
                }else{
                    
                    recv(sock1,&ackPacket,sizeof(packetNode), 0); // get your data

                    if(ackPacket.seqN<=acksReceived){
                        // printf("Repeated ACK for seqN : %d - Ignored\n",ackPacket.seqN);
                        continue;
                    }

                    acksReceived++;
                    ch1_wait = -1;
                    // printf("ACK Received on channel : 1 = %d\n",ackPacket.seqN);
                    printf("RCVD ACK : for PKT with Seq. No. %d from channel 1\n",ackPacket.seqN);
                }

            }

            if(acksReceived==num_packets)
            break;

            if(ch1_wait>=0 && ch1_wait<ch2_wait){
                continue;
            }
            
            if(ch2_wait>=0){
                
                // printf("ch2_wait for seq : %d\n",ch2_wait+1);

                fd.fd = sock2; // your socket handler 
                fd.events = POLLIN;
                ret = poll(&fd, 1, RETRANS_TIME*1000); // 2 second for timeout

                if(ret<=0){
                    printf("Ack Timeout\n");
                    // Resend Packet
                    int bytesSent = send(sock2,&packets[ch2_wait],sizeof(packetNode),0);
                }else{

                    recv(sock2,&ackPacket,sizeof(packetNode), 0); // get your data

                    if(ackPacket.seqN<=acksReceived){
                        // printf("Repeated ACK for seqN : %d - Ignored\n",ackPacket.seqN);
                        continue;
                    }

                    acksReceived++;

                    ch2_wait = -1;
                    // printf("ACK Received on channel : 2 = %d\n",ackPacket.seqN);
                    printf("RCVD ACK : for PKT with Seq. No. %d from channel 2\n",ackPacket.seqN);
                }                    
            }

            if(acksReceived==num_packets)
            break;

        }

        if(acksReceived==num_packets)
        break;

        if(sequenceNo==num_packets)
        continue;   // All Packets sent already!

        // Send a packet through socket1    
        int bytesSent;   
        packets[sequenceNo].channel=1;
        ch1_wait = sequenceNo;
        bytesSent = send(sock1,&packets[sequenceNo++],sizeof(packetNode),0);
        printf("SENT PKT Seq. No %d of size %lu Bytes from channel 1\n",sequenceNo-1,strlen(packets[sequenceNo-1].data));
        // bytesSent = sendto(sock1,&packets[trySequenceNo++],sizeof(packetNode),0,(struct sockaddr*)&serverAddr,sizeof(serverAddr));

        if(bytesSent!=sizeof(packetNode)){
            printf("Error while sending the message through Socket 1\n");
            exit(0);
        }

        // printf("Data Sent Socket1 SeqN = %d\n",packets[sequenceNo-1].seqN);

        packets[sequenceNo].channel=2;
        ch2_wait = sequenceNo;
        bytesSent = send(sock2,&packets[sequenceNo++],sizeof(packetNode),0);
        printf("SENT PKT Seq. No %d of size %lu Bytes from channel 2\n",sequenceNo-1,strlen(packets[sequenceNo-1].data));
        
        // bytesSent = sendto(sock1,&packets[trySequenceNo++],sizeof(packetNode),0,(struct sockaddr*)&serverAddr,sizeof(serverAddr));

        if(bytesSent!=sizeof(packetNode)){
            printf("Error while sending the message through Socket 2\n");
            exit(0);
        }

        // printf("Data Sent Socket2 SeqN = %d\n",packets[sequenceNo-1].seqN);
    }

    printf("\nFile Transfer Complete\n");

    close(sock1);
    close(sock2);

}
