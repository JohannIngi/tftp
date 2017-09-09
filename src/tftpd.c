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

#define error_code_file_not_found 1
#define error_code_access_violation 2
#define error_code_disk_full_or_allocation_exceeded 3
#define error_code_illegat_tftp_operation 4
#define error_code_unknown_transfer_ID 5
#define error_code_file_already_exists 6
#define error_code_no_such_user 7



typedef int bool;
#define true 1
#define false 0


//the variables needed for sending a data pack
struct data_pack{
    unsigned short opcode;
    unsigned short blocknumber;
    char data[512];
};
//setting the opcode number of the data pack and initializing the bloknumber
void set_data_pack_struct(struct data_pack* data_p, unsigned short blocknr){
    data_p->opcode = htons(3);
    data_p->blocknumber = htons(blocknr);
}
void get_filename(char* file_name, char* folder_name, char* full_path){
    //finding the file name from the clients request and storing it in an array for further use

    size_t file_name_len = strlen(file_name);
    size_t folder_len = strlen(folder_name);
    strcpy(full_path, folder_name);
    strcpy(full_path + folder_len, "/");
    strcpy(full_path + folder_len + 1, file_name);
    full_path[folder_len + file_name_len + 1] = 0;
}/*
void reading_and_sending_packs(){

}*/
void sending_error_pack(int sockfd, struct sockaddr_in* client, unsigned int error_code){
    char error_buffer[50];
    memset(error_buffer, 0, 50);
    error_buffer[1] = ERROR;
    error_buffer[3] = error_code;
    size_t error_len = 0;
    switch(error_code)
    {
        case error_code_file_not_found:
            strcpy(error_buffer, "File not found");
            break;
        case error_code_access_violation:
            strcpy(error_buffer, "Access violation.");
            break;
        case error_code_disk_full_or_allocation_exceeded:
            strcpy(error_buffer, "Disk full or allocation exceeded.");
            break;
        case error_code_illegat_tftp_operation:
            strcpy(error_buffer, "Illegal TFTP operation.");
            break;
        case error_code_unknown_transfer_ID:
            strcpy(error_buffer, "Unknown transfer ID.");
            break;
        case error_code_file_already_exists:
            strcpy(error_buffer, "File already exists.");
            break;
        case error_code_no_such_user:
            strcpy(error_buffer, "No such user.");
            break;
        default:
            strcpy(error_buffer, "Unknown error!");
    }

    error_len = strlen(error_buffer + 4) + 4;
    sendto(sockfd, error_buffer, 19, 0, (struct  sockaddr *)client, error_len);
}

int main(int argc, char **argv)
{
    // If we receive less than 3 arguments, terminate.
    if(argc != 3){
        perror("Invalid arguments!\n");
        exit(EXIT_FAILURE);
    }
    char* port = argv[1];
    char* file_name;
    fprintf(stdout, "Setting up server nr. %s\n", port); fflush(stdout);

    unsigned short blocknr;
    int sockfd;
    struct sockaddr_in server, client;
    //char client_pack[PACKET_SIZE];
    char server_pack[PACKET_SIZE];
    char error[512];
    char ack_buffer[4];
    char full_path[100];

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;

    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(atoi(port));
    bind(sockfd, (struct sockaddr *) &server, (socklen_t) sizeof(server));

    fprintf(stdout, "Socket set up and bound\n"); fflush(stdout);

    // server loop
    for (;;) {
        socklen_t len = (socklen_t) sizeof(client);
        ssize_t n = recvfrom(sockfd, server_pack, sizeof(server_pack) - 1,
                             0, (struct sockaddr *) &client, &len);
        server_pack[n] = '\0';

        fprintf(stdout, "received something from.... (TODO:setja inn client info maybe)\n"); fflush(stdout);

        if(server_pack[1] == RRQ){ //the msg is a RRQ
            blocknr = 1;
            //setja í aðferð fyrir að finna nafnið
            //finding the file name from the clients request and storing it in an array for further use
            file_name = server_pack + 2;
            get_filename(file_name, argv[2], full_path);
            fprintf(stdout, "Full path is: %s\n", full_path); fflush(stdout);

            //setja í aðferð til að opna og senda file
            FILE *file;
            file = fopen(full_path, "r");
            struct data_pack d_packet;

            //setja error í aðferð, samt 7 mismunandi aðferðir...
            if(file == NULL){
                sending_error_pack(sockfd, &client, 4);
            }
            //transfer loop starts
            
            bool data_transfer_running = true;

            //transfer loop
            while(data_transfer_running){
                //creating a data pack
                //setting the opcode and blocknumber
                set_data_pack_struct(&d_packet, blocknr);
                //reading a total of 512 bytes into a package to send
                size_t number_of_bytes = fread(d_packet.data, 1, 512, file);


                bool ack_is_from_receiver = true;
                while(ack_is_from_receiver){
                    
                    //sendiingng the data package
                    sendto(sockfd, (char*)&d_packet, number_of_bytes + 4, 0, (struct  sockaddr *) &client, len);
                    //receiving ack from client
                    if (recvfrom(sockfd, ack_buffer, sizeof(ack_buffer),
                                 0, (struct sockaddr *) &client, &len) < 0) {
                        // error
                        sending_error_pack(sockfd, &client, 4);
                        exit(1);
                    }
                    //if ack has not the same block number
                    if(ack_buffer[4] != blocknr){
                        ack_is_from_receiver = false;
                    }
                }

                //if number of bytes is not 512 then 
                if(number_of_bytes < 512){
                    data_transfer_running = false;
                } 

                //if ack is not really an ack
                if(ack_buffer[1] != 4){
                    sending_error_pack(sockfd, &client, 5);

                }
                //if ack has not the same block number
                blocknr++;
            }
            fprintf(stdout, "closing file\n"); fflush(stdout);
            fclose(file);

            /*      ° store info about sender
                    ° start >>> transfer loop <<<*/
        }
        else{ //the msg is not a RRQ

            fprintf(stdout, "Some retard tried something stupid\n"); fflush(stdout);
            sending_error_pack(sockfd, &client, 4); 
        }
        
    }
}