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

//the variables needed for sending a data pack
struct data_pack{
    unsigned short opcode;
    unsigned short blocknumber;
    char data[512];
};
//setting the opcode number of the data pack and initializing the bloknumber
void set_data_pack_struct(struct data_pack *data_p, unsigned short blocknr){
    data_p->opcode = htons(3);
    data_p->blocknumber = htons(blocknr);
}
/*
void get_filename(){
    //setja í aðferð fyrir að finna nafnið
    //finding the file name from the clients request and storing it in an array for further use
    char* file_name = server_pack + 2;
    size_t file_name_len = strlen(file_name);
    size_t argv_len = strlen(argv[2]);
    char full_path[argv_len + file_name_len + 2];
    strcpy(full_path, argv[2]);
    strcpy(full_path + argv_len, "/");
    strcpy(full_path + argv_len + 1, file_name);
    full_path[argv_len + file_name_len + 1] = 0;
    fprintf(stdout, "Full path is: %s\n", full_path); fflush(stdout);
}/*
void reading_and_sending_packs(){

}
void sending_error(){
	
}
*/
int main(int argc, char **argv)
{
    // If we receive less than 3 arguments, terminate.
    if(argc != 3){
        perror("Invalid arguments!\n");
        exit(EXIT_FAILURE);
    }
    char* port = argv[1];

    fprintf(stdout, "Setting up server nr. %s\n", port); fflush(stdout);

    unsigned short blocknr;
    int sockfd;
    struct sockaddr_in server, client;
    //char client_pack[PACKET_SIZE];
    char server_pack[PACKET_SIZE];
    char error[512];
    char ack_buffer[4];

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

        fprintf(stdout, "received something from.... (TODO:setja inn client info maybe)\n"); fflush(stdout);

        if(server_pack[1] == RRQ){ //the msg is a RRQ
            blocknr = 1;
            //setja í aðferð fyrir að finna nafnið
            //finding the file name from the clients request and storing it in an array for further use
            char* file_name = server_pack + 2;
            size_t file_name_len = strlen(file_name);
            size_t argv_len = strlen(argv[2]);
            char full_path[argv_len + file_name_len + 2];
            strcpy(full_path, argv[2]);
            strcpy(full_path + argv_len, "/");
            strcpy(full_path + argv_len + 1, file_name);
            full_path[argv_len + file_name_len + 1] = 0;
            fprintf(stdout, "Full path is: %s\n", full_path); fflush(stdout);

//setja í aðferð til að opna og senda file
            FILE *file;
            file = fopen(full_path, "r");

//setja error í aðferð, samt 7 mismunadni aðferðir...
            if(file == NULL){
                error[0] = 0;
                error[1] = 5;
                error[2] = 0;
                error[3] = 1;
                sprintf(error + 4, "File not found!");
                sendto(sockfd, error, 19, 0, (struct  sockaddr *) &client, len);
            }
            //transfer loop starts
            while(1){
                //creating a data pack
                struct data_pack d_packet;
                set_data_pack_struct(&d_packet, blocknr);
				size_t number_of_bytes = fread(d_packet.data, 1, 512, file);
				sendto(sockfd, (char*)&d_packet, number_of_bytes + 4, 0, (struct  sockaddr *) &client, len);
				fprintf(stdout, "inside inner loop\n"); fflush(stdout);
				ssize_t ack_received = recvfrom(sockfd, ack_buffer, sizeof(ack_buffer) - 1,
				0, (struct sockaddr *) &client, &len);

                /*if(ack_buffer[1] != 4){
                    exit(1);
                    //gera mad error
                }
                else if(ack_buffer[4] != block){
                    exit(1);
                    //gera mad error
                }*/
                while (number_of_bytes == 512){
                    blocknr++;
	                set_data_pack_struct(&d_packet, blocknr);
	                number_of_bytes = fread(d_packet.data, 1, 512, file);
	                sendto(sockfd, (char*)&d_packet, number_of_bytes + 4, 0, (struct  sockaddr *) &client, len);
	                fprintf(stdout, "inside inner loop\n"); fflush(stdout);
	                ack_received = recvfrom(sockfd, ack_buffer, sizeof(ack_buffer) - 1,
	                             0, (struct sockaddr *) &client, &len);
                    /*if(ack_buffer[1] != 4){
                        exit(1);
                        //gera mad error
                    }
                    else if(ack_buffer[4] != block){
                        exit(1);
                        //gera mad error
                    }*/
                }
                blocknr++;
                set_data_pack_struct(&d_packet, blocknr);
                number_of_bytes = fread(d_packet.data, 1, 512, file);
                sendto(sockfd, (char*)&d_packet, number_of_bytes + 4, 0, (struct  sockaddr *) &client, len);
                fprintf(stdout, "inside inner loop\n"); fflush(stdout);
                ack_received = recvfrom(sockfd, ack_buffer, sizeof(ack_buffer) - 1,
                             0, (struct sockaddr *) &client, &len);
                /*if(ack_buffer[1] != 4){
                    exit(1);
                    //gera mad error
                }
                else if(ack_buffer[4] != block){
                    exit(1);
                    //gera mad error
                }*/
                break;
            }
            fclose(file);

            /*      ° store info about sender
                    ° start >>> transfer loop <<<*/
        }
        else{ //the msg is not a RRQ

            fprintf(stdout, "Some retard tried something stupid\n"); fflush(stdout);

            error[0] = 0;
            error[1] = 5;
            error[2] = 0;
            error[3] = 0;
            sprintf(error + 4, "COCK ERROR");
            sendto(sockfd, error, (size_t) n, 0, (struct  sockaddr *) &client, len);
        }
        
    }
}