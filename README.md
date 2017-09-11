/*
 * Programming Assignment 1 – Trivial File Transfer Protocol
 *
 * Team:
 * Ármann Pétur Ævarsson [armanna16@ru.is]
 * Jóhann Ingi Bjarnason [johannb16@ru.is]
 *
 */

 Our TFTP server is set up on the server structure that we got from the class teacher.

 In the main function we connect the socket to appropriate port and start an infinite server loop. At the start of the infinite server loop we ask what kind of an package is being received. If it is an read request (RRQ) package our server goes into a further funtions and loops that deals with the data transfer and other things, see below. If the package is not an RRQ, maybe an  write request (WRQ) for example, there will be an error package sent with error code 4, claiming it to be an illegal TFTP operation.

If the package is an RRQ we extract the filename from server and client using a function that we implemented called void get_filename(). After we open the file we enter another loop for data transfer, which does not break unless data transfer is complete or a specific error occurs. Inside data transfer loop there is an inner transfer loop that makes sure that the acknowledgments (ACK) that we receive are correct. 

We do a number of error handling for numerous errors that might occur and for that we implemented a function called void sending_error_pack(), that actually sends all error packages.

Regarding the mode sets that RRQ and WRQ use. We were not able to handle them due to lack of time and inexperiance in C.

Server has been tested on standard RedHat Enterprise Linux Server Version 7.3
For usage of the server:
./tftpd <port> <shared_directory>
