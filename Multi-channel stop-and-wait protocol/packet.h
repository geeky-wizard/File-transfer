#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>
#include <poll.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>

#define PACKET_SIZE 100 // Try with different values
#define RETRANS_TIME 2  // re-transmission time : 2 sec
#define PORT 12344
#define BUFFERSIZE 2
#define PDR 10

#pragma pack(1)
typedef struct 
{
    char data[PACKET_SIZE+1];

    int size;   // in bytes - 4
    int seqN;   // in bytes - 4
    bool last;  // is last packet or not    - 1
    bool isAck;     // is Ack or Data   - 1
    int channel;    // 0 or 1 channel - 4
    
}packetNode;
#pragma pack(0)
