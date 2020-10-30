#define PACKET_SIZE 100 // Try with different values
#define RETRANS_TIME 2  // re-transmission time : 2 sec
#define BUFFERSIZE 2
#define PDR 10
#define WINDOW_SIZE 4

#pragma pack(1)
typedef struct 
{
    char data[PACKET_SIZE+1]; // - 101

    int size;   // in bytes - 4
    int seqN;   // in bytes - 4
    bool last;  // is last packet or not    - 1
    bool isAck;     // is Ack or Data   - 1
    // int channel;    // 0 or 1 channel - 4
    
}packetNode;
#pragma pack(0)

