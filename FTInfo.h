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
 * File Name:		FTInfo.h
 * File Description: 	Definition of a struct to keep track of variables used for interaction
 * 			with an indivdioual client program and prototypes of initializer and destroyer.
 * Course Name: 	CS 372-400: Introduction to Computer Networks
 * Last Modified:	03/09/2020
*****************************************************************************************************/

#ifndef FT_INFO
#define FT_INFO

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Global constants containing IPv4 addresses of each flip server. */
#define FLIP1 "128.193.54.168"
#define FLIP2 "128.193.54.182"
#define FLIP3 "128.193.36.41"

/* Definition of struct containing variables related to communication with an individual
 * client program. See below for variable descriptions. */
struct FTInfo
{
	int controlSocketFD; 	/* Socket used for control connection to client. */
	char* clientHost;	/* Address of client from which a control connection has been accepted. */
	char* clientNickname;	/* Name of the flip server on which client is running (if applicable) or IP address. */
	char* dataPort;		/* Port number at which to establish data connection with client. */ 
	char* command;		/* Requested command to be executed. */
	char* filename;		/* Name of file to be sent to client (if applicable). */
	int dataSocketFD;	/* Socket used for data connection to client. */
};

/* Function prototypes. */
struct FTInfo* newFTInfo(int controlSocketFD, char* clientHost);
char* getNickname(char* clientHost);
void deleteFTInfo(struct FTInfo* myFT);

#endif
