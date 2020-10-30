Problem 1:      File transfer using multi-channel stop-and-wait protocol

Input File should be named : "input.txt"
Output File generated will be named : "output.txt"

# Run the following Commands on 2 different terminals(In Order given)

1. Server Terminal  :       gcc -o recv server.c 
                            ./recv

2. Client Terminal  :       gcc -o send client.c -lm
                            ./send


Implementation Details :-


1. At starting, I send packet with SeqNo=1 through channel-1 and SeqNo=2 through channel-2. Then the logic given in the doc is followed.

2. I used the poll function for keeping a check if ACK is received at Client within retransmission time. Poll is kept after sending every 2 packets. 

3. On Server Side, select is used to see from which channel it came from.

4. Repeated ACKs are handled at client side.

5. BufferSize is kept limited as mentioned.





