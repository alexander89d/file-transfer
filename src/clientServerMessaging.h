/******************************************************************************************************
 * Programmer Name: 	Alexander Densmore
 * Program Name: 	ftserver
 * Program Description:	Implementation of the server side of a client-server file transfer protocol. 
 * 			Client receives SERVER_PORT from the command line. Once SERVER_PORT is verified
 * 			to be a non-negative integer, server attempts to establish listening socket on
 * 			SERVER_PORT. Upon success, server enters endless loop of listening for and then
 * 			accepting incoming connections until a SIGINT is received.
 * 			
 * 			Upon accepting an incoming connection (which becomes the control connection
 * 			for the file transfer), the server receives the desired data port at the client
 * 			to which to send responses to valid commands. The server then receives and interprets
 * 			the command from the client. If the command is valid, the server connects a new socket
 * 			to the client at clienthost:dataport. The server sends the desired data through the
 * 			data port and then sends a confirmation message that the transfer is complete
 * 			through the control port. The server then closes the data port. 
 * 			If the command from the client is invalid, the server sends an error message through
 * 			the control port and does not initiate a data connection. 
 * 			Then, the server leaves the client responsible for closing the control port connection and 
 * 			awaits a new incoming client connection.
 * *** EXTRA CREDIT ***	In addition to the required commands of -l (list all files in current directory)
 *			and -g (get file with filename), I have implemented an option -ltxt (list all files
 *			in current directory with .txt extension). Upon receiving -ltxt command,
 *			server filters current directory listing for only files with .txt extension,
 *			either sending list of such files to client or reporting that there
 *			are no files with .txt extension in the current directory.
 * File Name:		clientServerMessaging.h
 * File Description: 	Header file for functions related to the establishment, binding and connecting
 * 			of sockets as well as sending and receiving messages using sockets.
 * Course Name: 	CS 372-400: Introduction to Computer Networks
 * Last Modified:	03/09/2020
*****************************************************************************************************/

#ifndef CLIENT_SERVER_MESSAGING
#define CLIENT_SERVER_MESSAGING

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include "FTInfo.h"

/* Constant representing max number of connections awaiting acceptance. */
#define MAX_BACKLOG 10

/* Function prototypes. */
int establishListeningSocket(char* serverPort);
struct FTInfo* acceptClientConnection(int listeningSocketFD);
int establishDataSocket(char* clientHost, char* dataPort);
int sendMessage(int socketFD, char* message);
int sendCompleteString(int socketFD, char* message);
char* recvMessage(int socketFD);
int recvError(int charsRead);

#endif
