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
//finding the file name from the clients request and storing it in an array for further use
void get_filename(char* file_name, char* folder_name, char* full_path){
    //finding the length of the file
    size_t file_name_len = strlen(file_name);
    //finding the length of the folder
    size_t folder_len = strlen(folder_name);
    //putting folder and file name together with the appropriate "/" between
    strcpy(full_path, folder_name);
    strcpy(full_path + folder_len, "/");
    strcpy(full_path + folder_len + 1, file_name);
    full_path[folder_len + file_name_len + 1] = 0;
}/*
void reading_and_sending_packs(){

}*/
//sending the error package
void sending_error_pack(int sockfd, struct sockaddr_in* client, unsigned int error_code){
    //creating the array that will hold the error message
    char error_buffer[50];
    //initializing array to null
    memset(error_buffer, 0, 50);
    //setting error opcode to 5
    error_buffer[1] = ERROR;
    //setting error code
    error_buffer[3] = error_code;
    //length of the error array
    size_t error_len = 0;
    //setting appropriate error code
    switch(error_code)
    {
        case error_code_file_not_found:
            strcpy(error_buffer, "File not found");
            //error_len = 18;
            break;
        case error_code_access_violation:
            strcpy(error_buffer, "Access violation.");
            //error_len = 21; 
            break;
        case error_code_disk_full_or_allocation_exceeded:
            strcpy(error_buffer, "Disk full or allocation exceeded.");
            //error_len = 37;
            break;
        case error_code_illegat_tftp_operation:
            strcpy(error_buffer, "Illegal TFTP operation.");
            //error_len = 27;
            break;
        case error_code_unknown_transfer_ID:
            strcpy(error_buffer, "Unknown transfer ID.");
            //error_len = 24;
            break;
        case error_code_file_already_exists:
            strcpy(error_buffer, "File already exists.");
            //error_len = 24;
            break;
        case error_code_no_such_user:
            strcpy(error_buffer, "No such user.");
            //error_len = 17;
            break;
        default:
            strcpy(error_buffer, "Unknown error!");
            //error_len = 18;
    }
    //setting length of error array
    error_len = strlen(error_buffer + 4) + 4;
    //sending error
    sendto(sockfd, error_buffer, error_len, 0, (struct  sockaddr *)client, error_len);
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
    fprintf(stdout, "Connecting to server number: %s\n", port); fflush(stdout);

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

    fprintf(stdout, "Socket set up complete \n"); fflush(stdout);

    // server loop
    for (;;) {
        socklen_t len = (socklen_t) sizeof(client);
        ssize_t n = recvfrom(sockfd, server_pack, sizeof(server_pack) - 1,
                             0, (struct sockaddr *) &client, &len);
        server_pack[n] = '\0';//null terminating

        fprintf(stdout, "received a request from.... (TODO:setja inn client info maybe)\n"); fflush(stdout);

        if(server_pack[1] == RRQ){ //the msg is a RRQ
            fprintf(stdout, "The request is a read request (RRQ)\n"); fflush(stdout);
            //block number set
            blocknr = 1;
            //finding the file name from the clients request and storing it in an array for further use
            file_name = server_pack + 2;


            //checking to see if the file name contains an .. wich is an access violation error
            char * illegal_checker = strstr (file_name, "..");
            if(illegal_checker != NULL){
                sending_error_pack(sockfd, &client, 2);
                exit(1);
            }


            get_filename(file_name, argv[2], full_path);
            fprintf(stdout, "Full path is: %s\n", full_path); fflush(stdout);

            FILE *file;
            file = fopen(full_path, "r");
            struct data_pack d_packet;

            if(file == NULL){
                sending_error_pack(sockfd, &client, 4);
            }
            //transfer loop starts
            
            bool data_transfer_running = true;

            //transfer loop starts
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
                    if(ack_buffer[4] != blocknr){ //gets a warning that "array subscript is above array bounds". If i change array to [3] the transfer loop stops working...
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