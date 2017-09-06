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

#define PACKET_SIZE 516

struct data_pack{
    unsigned short opcode;
    unsigned short blocknumber;
    char data[512];
};

int main(int argc, char **argv)
{
    // If we receive less than 3 arguments, terminate.
    if(argc != 3){
        perror("Invalid arguments!\n");
        exit(EXIT_FAILURE);
    }
    char* port = argv[1];

    fprintf(stdout, "Setting up server nr. %s\n", port); fflush(stdout);


    int block;
    int sockfd;
    struct sockaddr_in server, client;
    //char client_pack[PACKET_SIZE];
    char server_pack[PACKET_SIZE];
    char error[512];
    char ack[4];


    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;

    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(atoi(port));
    bind(sockfd, (struct sockaddr *) &server, (socklen_t) sizeof(server));

    fprintf(stdout, "Socket set up and bound\n"); fflush(stdout);

    for (;;) {
        socklen_t len = (socklen_t) sizeof(client);
        ssize_t n = recvfrom(sockfd, server_pack, sizeof(server_pack) - 1,
                             0, (struct sockaddr *) &client, &len);


        fprintf(stdout, "received something from.... (TODO)\n"); fflush(stdout);


        if(server_pack[1] == RRQ){ //the msg is a RRQ
            block = 1;
            char* file_name = server_pack + 2;

            size_t file_name_len = strlen(file_name);

            size_t argv_len = strlen(argv[2]);

            char full_path[argv_len + file_name_len + 2];

            strcpy(full_path, argv[2]);
            strcpy(full_path + argv_len, "/");
            strcpy(full_path + argv_len + 1, file_name);


            full_path[argv_len + file_name_len + 1] = 0;


            fprintf(stdout, "Full path is: %s\n", full_path); fflush(stdout);

            FILE *file;
            file = fopen(full_path, "r");

            if(file == NULL){
                error[0] = 0;
                error[1] = 5;
                error[2] = 0;
                error[3] = 1;
                sprintf(error + 4, "File not found!");
                sendto(sockfd, error, 19, 0, (struct  sockaddr *) &client, len);
            }

            while(1){
                //creating a data pack
                struct data_pack d_packet;
                d_packet.opcode = htons(3);
                d_packet.blocknumber = htons(block);
                size_t number_of_bytes = fread(d_packet.data, 1, 512, file);
                sendto(sockfd, (char*)&d_packet, number_of_bytes + 4, 0, (struct  sockaddr *) &client, len);
                fprintf(stdout, "things have been sent \n"); fflush(stdout);
                ssize_t ack_received = recvfrom(sockfd, ack, sizeof(ack) - 1,
                             0, (struct sockaddr *) &client, &len);
                fprintf(stdout, "ack received\n"); fflush(stdout);
                /*if(ack[1] != 4){
                    exit(1);
                    //gera mad error
                }
                else if(ack[4] != block){
                    exit(1);
                    //gera mad error
                }*/
                while (number_of_bytes == 512){
                    block++;
                    d_packet.opcode = htons(3);
                    d_packet.blocknumber = htons(block);
                    number_of_bytes = fread(d_packet.data, 1, 512, file);
                    sendto(sockfd, (char*)&d_packet, number_of_bytes + 4, 0, (struct  sockaddr *) &client, len);
                    fprintf(stdout, "inside inner loop\n"); fflush(stdout);
                    ack_received = recvfrom(sockfd, ack, sizeof(ack) - 1,
                                 0, (struct sockaddr *) &client, &len);
                    /*if(ack[1] != 4){
                        exit(1);
                        //gera mad error
                    }
                    else if(ack[4] != block){
                        exit(1);
                        //gera mad error
                    }*/
                }
                block++;
                d_packet.opcode = htons(3);
                d_packet.blocknumber = htons(block);
                number_of_bytes = fread(d_packet.data, 1, 512, file);
                sendto(sockfd, (char*)&d_packet, number_of_bytes + 4, 0, (struct  sockaddr *) &client, len);
                fprintf(stdout, "sending last pack\n"); fflush(stdout);
                ack_received = recvfrom(sockfd, ack, sizeof(ack) - 1,
                             0, (struct sockaddr *) &client, &len);
                if(ack[1] != 4){
                    exit(1);
                    //gera mad error
                }
                else if(ack[4] != block){
                    exit(1);
                    //gera mad error
                }
                break;
            }
            fclose(file);

            /*char testfile[513];
            while(1){

                int nnn = fread(testfile, 1, 512, file);
                fprintf(stdout, "N = %d\n", nnn); fflush(stdout); 
                fprintf(stdout, "%s\n", testfile); fflush(stdout);
                fprintf(stdout, "------------------------------------------\n"); fflush(stdout);
                if (nnn < 512) break;
            }
            fflush(stdout);*/



            /*      ° store info about sender
                    ° start >>> transfer loop <<<*/
        }
        else{ //the msg is not a RRQ

            fprintf(stdout, "Some retard tried something stuid\n"); fflush(stdout);

            error[0] = 0;
            error[1] = 5;
            error[2] = 0;
            error[3] = 0;
            sprintf(error + 4, "COCK ERROR");
            sendto(sockfd, error, (size_t) n, 0, (struct  sockaddr *) &client, len);
        }
        
    }
}