#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h> 

#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros 

#include "utilsDef.h"


packetNode* makeAckPacket(int seqN){

    packetNode* packet = malloc(sizeof(packetNode));
    packet->isAck=1;
    packet->seqN = seqN;
    packet->size = PACKET_SIZE;
    return packet;
}

FILE* fp;   // FILE Pointer to output file
int packetsReceived=0;  // Same work as SeqN
packetNode buffer[BUFFERSIZE];
int bufferChannel[BUFFERSIZE];
int bufLen = 0;

void addBufferedData(){
    for(int i=0;i<bufLen;i++){
        if(buffer[i].seqN == packetsReceived+1){
            printf("Got Packet SeqN = %d from buffer %d\n",buffer[i].seqN,buffer[i].channel);
            printf("Adding Data to Output File from buffer : %s\n",buffer[i].data);
            fwrite(buffer[i].data , 1 , 100 , fp);
            packetNode* ackPacket = makeAckPacket(buffer[i].seqN);
            send(bufferChannel[i],ackPacket,sizeof(packetNode),0);
            packetsReceived++;
        }
    }
}

int addToBufferOrOutput(packetNode* bufPacket,int sockNum){

    printf("Got Packet SeqN = %d from channel %d\n",bufPacket->seqN,bufPacket->channel);

    if(packetsReceived+1==bufPacket->seqN){
            // Correct Sequence
            // printf("Adding Data to Output File : %s\n",bufPacket->data);
            fwrite(bufPacket->data , 1 , 100 , fp);
            packetsReceived++;
            // Send ACK Packet
            packetNode* ackPacket = makeAckPacket(bufPacket->seqN);
            send(sockNum,ackPacket,sizeof(packetNode),0);
            printf("Ack sent for %d\n",bufPacket->seqN);

            addBufferedData();
            return 1;
    }

    if(bufLen<BUFFERSIZE){
        printf("Adding to Buffer SeqN=%d from channel %d\n",bufPacket->seqN,bufPacket->channel);
        bufferChannel[bufLen] = sockNum;
        buffer[bufLen++] = *bufPacket;
        return bufLen;
    }
    else{
        printf("Buffer is FULL\n\n");
        return -1;
    }
}

bool dropPacket(){

    // return false;    // Uncomment to avoid dropping packets!
    
    int probDrop = rand()%100;
    printf("probDrop : %d\n",probDrop);

    if(probDrop<PDR){
        return true;
    }else{
        return false;
    }
}

int main(){

    srand(time(NULL)); 

    int master_socket , addrlen , new_socket , client_socket[30];
    int max_sd,max_channels,sd;
    int activity;

    max_channels = 2;
    
    fp = fopen("output.txt","w");

    struct sockaddr_in address; 
    //set of socket descriptors  
    fd_set readfds; 

    //initialise all client_socket[] to 0 so not checked  
    for (int i = 0; i < max_channels; i++)   
    {   
        client_socket[i] = 0;   
    }   

    //create a master socket  
    if( (master_socket = socket(AF_INET , SOCK_STREAM , 0)) == 0)   
    {   
        perror("socket failed");   
        exit(EXIT_FAILURE);   
    }

    //type of socket created  
    address.sin_family = AF_INET;   
    address.sin_addr.s_addr = INADDR_ANY;   
    address.sin_port = htons(PORT);  

    if (bind(master_socket, (struct sockaddr *)&address, sizeof(address))<0)   
    {   
        perror("bind failed");   
        exit(EXIT_FAILURE);   
    }   
    printf("Listener on port %d \n", PORT); 

    addrlen = sizeof(address);   
    puts("Waiting for connections ..."); 


    int temp1 = listen(master_socket, 2); 
    if (temp1 < 0)
    {  printf ("Error in listen");
        exit (0);
    }
    printf ("Now Listening\n");

    while(1){

        // clear the socket set
        FD_ZERO(&readfds);

        FD_SET(master_socket,&readfds);
        max_sd = master_socket;

        // add child sockets to set
        for(int i=0;i<max_channels;i++){

            // socket descriptor
            sd = client_socket[i];

            // if valid socket descriptor then add to read list
            if(sd>0)
                FD_SET(sd,&readfds);
                
            if(sd>max_sd)
                max_sd = sd;
        }
         
        activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL);

        if(activity<0){
            printf("No Activity on any socket\n");
            exit(0);   
        }
        
        //If something happened on the master socket ,  
        //then its an incoming connection

        if(FD_ISSET(master_socket,&readfds)){

            if((new_socket = accept(master_socket,(struct sockaddr*)&address,(socklen_t*)&addrlen))<0){
                perror("accept");
                exit(EXIT_FAILURE);
            }

            // printf("New connection , socket fd is %d , ip is : %s , port : %d \n" , new_socket , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));

            //add new socket to array of sockets  
            for (int i = 0; i < max_channels; i++)   
            {   
                //if position is empty  
                if( client_socket[i] == 0 )   
                {   
                    client_socket[i] = new_socket;   
                    printf("Adding Channel %d\n" , i+1);  
                    break;   
                }   
            } 
        }

        // Other operations than connection making!
        for(int i=0;i<max_channels;i++){

            sd = client_socket[i];

            if(FD_ISSET(sd,&readfds)){
                packetNode tempPacket;
                int bytesRecvd = recv (sd,&tempPacket, sizeof(packetNode), 0);    
                // int bytesRecvd = recvfrom(sd, &tempPacket, sizeof(packetNode), 0, (struct sockaddr *)&address,(socklen_t*)&addrlen);
                
                // printf("Check1 : %d\n",bytesRecvd);

                if(bytesRecvd==-1){
                    perror("Receive Error\n");
                    exit(0);
                }

                // TODO : Drop Packets!
                // srand(time(0)); 

                if(dropPacket()){
                    // Packet Dropped
                    printf("Packet %d dropped!\n\n",tempPacket.seqN);
                }else{
                    addToBufferOrOutput(&tempPacket,sd);
                    // Send ACK Packet
                    // packetNode* ackPacket = makeAckPacket(tempPacket.seqN);
                    // if(send(sd,ackPacket,sizeof(packetNode),0)!=sizeof(packetNode)){
                    //     perror("send");
                    // }
                }

                if(tempPacket.last==true){

                    if(packetsReceived==tempPacket.seqN){
                        // Next packet to be received is the last then!
                        // All Packets are read!
                        close(sd);
                        exit(1);   
                    }
                }

            }

        }

    }

    close(master_socket);
}   
