#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>


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
#define error_code_illegal_tftp_operation 4
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
}
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
            strcpy(error_buffer + 4, "File not found. Please try again.");
            break;
        case error_code_access_violation:
            strcpy(error_buffer + 4, "Access violation!");
            break;
        case error_code_disk_full_or_allocation_exceeded:
            strcpy(error_buffer + 4, "Disk full or allocation exceeded.");
            break;
        case error_code_illegal_tftp_operation:
            strcpy(error_buffer + 4, "Illegal TFTP operation. Only RRQ allowed!");
            break;
        case error_code_unknown_transfer_ID:
            strcpy(error_buffer + 4, "Unknown transfer ID.");
            break;
        case error_code_file_already_exists:
            strcpy(error_buffer + 4, "File already exists.");
            break;
        case error_code_no_such_user:
            strcpy(error_buffer + 4, "No such user.");
            break;
        default:
            strcpy(error_buffer + 4, "Unknown error!");
    }
    //setting length of error array
    error_len = strlen(error_buffer + 4) + 4;
    //sending error
    sendto(sockfd, error_buffer, error_len, 0, (struct  sockaddr *)client, (socklen_t)sizeof(struct sockaddr_in));
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
    char* ip_address;
    unsigned short blocknr;
    int sockfd;
    //int timeout_counter = 0;
    struct sockaddr_in server, client;
    char server_pack[PACKET_SIZE];
    char ack_buffer[5];
    char full_path[100];

    //setting up timeout
    /*struct timeval timeout;
    timeout.tv_sec = 5;*/

    fprintf(stdout, "Listening to server number: %s\n", port); fflush(stdout);
    //creating and binding a UDP socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    //htonl and htons convert the bytes to be used by the network functions
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(atoi(port));
    bind(sockfd, (struct sockaddr *) &server, (socklen_t) sizeof(server));
    fprintf(stdout, "Socket set up complete. Waiting for request.\n"); fflush(stdout);

    // server loop
    for (;;) {
        socklen_t len = (socklen_t) sizeof(client);
        ssize_t n = recvfrom(sockfd, server_pack, sizeof(server_pack) - 1,
                             0, (struct sockaddr *) &client, &len);
        server_pack[n] = '\0';//null terminating
        //acquiring the ip address from the client
        ip_address = inet_ntoa(client.sin_addr);
        //storing information about the port from client
        unsigned short original_port = client.sin_port; 

        //the msg is a RRQ
        if(server_pack[1] == RRQ){ 
            fprintf(stdout, "The request is a read request (RRQ)\n"); fflush(stdout);
            //block number set
            blocknr = 1;
            //finding the file name from the clients request and storing it in an array for further use
            file_name = server_pack + 2;

            //checking to see if the file name contains an .. wich is an access violation error
            char * illegal_checker = strstr (file_name, "..");
            if(illegal_checker != NULL){
                fprintf(stdout, "here I send; are you crazy error \n"); fflush(stdout);
                sending_error_pack(sockfd, &client, 2);
                continue;
            }
            //putting toghether file name
            get_filename(file_name, argv[2], full_path);
            //what is the complete filename and who requested it, ip address and port
            fprintf(stdout, "File requested is: %s. From ip address: %s. Port number: %d\n", full_path, ip_address, original_port); fflush(stdout);

            FILE *file;
            file = fopen(full_path, "r");
            //file not found villa
            if(file == NULL){
                fprintf(stdout, "here I send no file found error \n"); fflush(stdout);
                sending_error_pack(sockfd, &client, 1);
                continue;
            }
            bool data_transfer_running = true;
            struct data_pack d_packet;
            //transfer loop starts
            while(data_transfer_running){
                //creating a data pack
                //setting the opcode and blocknumber
                set_data_pack_struct(&d_packet, blocknr);
                //reading a total of 512 bytes into a package to send
                size_t number_of_bytes = fread(d_packet.data, 1, 512, file);

                bool ack_is_from_receiver = true;
                while(ack_is_from_receiver){

                    //sendingng the data package
                    sendto(sockfd, (char*)&d_packet, number_of_bytes + 4, 0, (struct  sockaddr *) &client, len);
                    //receiving ack from client
                    //if ack is not really an ack
                    if (recvfrom(sockfd, ack_buffer, sizeof(ack_buffer),
                                 0, (struct sockaddr *) &client, &len) < 0) {
                        sending_error_pack(sockfd, &client, 0);   
                    }

                    //if no response, resend the data 5 times, if no answer still...timeout
                    /*while(setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
                        fprintf(stdout, "TIMEOUT \n"); fflush(stdout);
                        sendto(sockfd, (char*)&d_packet, number_of_bytes + 4, 0, (struct  sockaddr *) &client, len);
                        timeout_counter++;
                        if(timeout_counter == 5){break;}
                    }*/
                    //if ip address does not match original sender.
                    if(ip_address != inet_ntoa(client.sin_addr) && original_port != client.sin_port){
                        sending_error_pack(sockfd, &client, 5);
                    }
                    //if ack has not the same block number
                    if(ack_buffer[4] != blocknr){
                        ack_is_from_receiver = false;
                        // senda error hér ?
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
        }
        else{ //the msg is not a RRQ

            sending_error_pack(sockfd, &client, 4); 

        }
        
    }
}