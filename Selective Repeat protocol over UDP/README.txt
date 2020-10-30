Problem 2:      File transfer using Selective Repeat protocol over UDP

Input File should be named : "input.txt"
Output File generated will be named : "output.txt"

# Run the following Commands on 4 different terminals(In Order given)

1. Server Terminal  :       gcc -o recv server.c 
                            ./recv

2. Relay1 Terminal  :       gcc -o relay1 relay1.c
                            ./relay1

3. Relay2 Terminal  :       gcc -o relay2 relay2.c
                            ./relay2

4. Client Terminal  :       gcc -o send client.c -lm
                            ./send


# Additional Instructions :





