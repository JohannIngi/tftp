#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

// Define OP Codes.
#define RRQ 1
#define WRQ 2
#define DATA 3
#define ACK 4
#define ERROR 5

int getOpcode(char* message){
    return message[1];
}

int main(int argc, char **argv)
{
    if(argc != 3){
        perror("Invalid arguments!\n");
        exit(EXIT_FAILURE);
    }
    char* port = argv[1];

    int block
    int sockfd;
    struct sockaddr_in server, client;
    char message[512];


    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;

    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(atoi(port));
    bind(sockfd, (struct sockaddr *) &server, (socklen_t) sizeof(server));

    for (;;) {
        socklen_t len = (socklen_t) sizeof(client);
        ssize_t n = recvfrom(sockfd, message, sizeof(message) - 1,
                             0, (struct sockaddr *) &client, &len);
        int opCode = getOpcode(message);
        if(message[1] == 1){
            if()
        }
        else{
            error[0] = 0;
            error[1] = 5;
            error[2] = 0;
            error[3] = 0;
            error[4] = 'C';
            error[5] = 'o';
            error[6] = 'C';
            error[7] = 'K';
            error[8] = 0;
            sendto(sockfd, error, (size_t) n, 0, (struct  sockaddr *) &client, len);
        }
        

    }
}