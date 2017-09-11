#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <errno.h>


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

int main(int argc, char **argv){
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
    struct sockaddr_in server, client;
    char server_pack[PACKET_SIZE];
    char ack_buffer[4];
    char full_path[100];


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
        //receiving a package
        ssize_t n = recvfrom(sockfd, server_pack, sizeof(server_pack) - 1,
                             0, (struct sockaddr *) &client, &len);
        //null terminating
        server_pack[n] = '\0';
        //acquiring the ip address from the client
        ip_address = inet_ntoa(client.sin_addr);
        unsigned long ip_address_number = client.sin_addr.s_addr;
        //storing information about the port from client
        unsigned short original_port = client.sin_port; 

        //the message is a RRQ
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
            struct data_pack d_packet;
            //transfer loop starts
            while(true){
            	fprintf(stdout, "transfer loop starts \n"); fflush(stdout);
                //creating a data pack
                //setting the opcode and blocknumber
                set_data_pack_struct(&d_packet, blocknr);
                //reading a total of 512 bytes into a package to send
                size_t number_of_bytes = fread(d_packet.data, 1, 512, file);

                //sendingng the data package
                sendto(sockfd, (char*)&d_packet, number_of_bytes + 4, 0, (struct  sockaddr *) &client, len);
                while(true){
                    //receiving ack from client
                    //Getting a value from ack
                    //fprintf(stdout, "about to receive \n"); fflush(stdout);
                    ssize_t receiver = recvfrom(sockfd, ack_buffer, sizeof(ack_buffer),
                                 0, (struct sockaddr *) &client, &len);
                    fprintf(stdout, "inner transfer loop starts \n"); fflush(stdout);
                    //get the blocknumber from ack received

                    if(receiver < 0){
                        //system error if ack value is < 0 
                        fprintf(stdout, "Terminating the program \n"); fflush(stdout);
                        sending_error_pack(sockfd, &client, 0);
                        exit(1);   
                    }
                    //if ip address does not match original sender, send error and continue running the server
                    if(ip_address_number != client.sin_addr.s_addr && original_port != client.sin_port){
                        sending_error_pack(sockfd, &client, 5);
                        continue;
                    }
                    //if ack is not really an ack send error and continue running the server
                    if(ack_buffer[1] != 4){
                        sending_error_pack(sockfd, &client, 5);
                        continue;
                    }
                    
                    unsigned short recv_block = ((unsigned char)ack_buffer[2] << 8) + (unsigned char)ack_buffer[3];
                    fprintf(stdout, "my block number: %hu. Ack blocknumber: %hu\n", blocknr, recv_block); fflush(stdout);
                    //if ack has not the same block number
                    if(recv_block != blocknr){
                        //re-send and counter ++
                        sendto(sockfd, (char*)&d_packet, number_of_bytes + 4, 0, (struct  sockaddr *) &client, len);
                        continue;
                    }

                    break;
                }
            	//if number of bytes is not 512 then 
            	if(number_of_bytes < 512){
            		fprintf(stdout, "number of bytes is less than 512\n"); fflush(stdout);
                	break;
                }
                blocknr++;
            }
			fprintf(stdout, "closing file\n"); fflush(stdout);
			fclose(file);
        }
        else //the msg is not a RRQ
        sending_error_pack(sockfd, &client, 4);    
    }
}