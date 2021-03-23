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
 * File Name:		manageConnections.h
 * File Description: 	Header file for multiple functions responsible for validating SERVER_PORT,
 * 			establishing a listening socket at that port, and then entering an endless loop 
 * 			awaiting new connections.
 * Course Name: 	CS 372-400: Introduction to Computer Networks
 * Last Modified:	03/09/2020
*****************************************************************************************************/

#ifndef MANAGE_CONNECTIONS
#define MANAGE_CONNECTIONS

#include <ctype.h>
#include <dirent.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include "clientServerMessaging.h"
#include "FTInfo.h"

/* Global constants representing possible commands. */
#define GET_FILE "-g"
#define LIST_FILES "-l"
#define LIST_TXT_FILES "-ltxt"

/* Global constants representing .txt extension and extension length. */
#define TXT_EXTENSION ".txt"
#define TXT_EXTENSION_LEN 4

/* Global constant representing maximum number of bytes that can be read from file / sent to 
 * client at one time. */
#define MAX_SEND_SIZE 10000

/* Global constant representing max number of digits of unsigned long long int when compiling using gcc compiler 
 * (determined by running test program using gcc compiler, limits.h built-in header,
 * and printing value of MAX_ULLONG macro, as suggested at the following webpage
 * on the GNU website: https://www.gnu.org/software/libc/manual/html_node/Range-of-Type.html. */
#define MAX_ULLINT_DIGITS 20

/* Global variable declarations. */
extern int listeningSocketFD;			/* Listening socket file descriptor closed by SIGINT handler. */
extern char* serverPort;			/* SERVER_PORT received on command line; used when printing errors. */

/* Function prototypes. */
int validatePortnum(char* portnum);
void startup(char* portnum);
void setSIGINThandler();
void catchSIGINT(int signo);
void acceptConnection();
int validateControlConnection(struct FTInfo* myFT);
void handleRequest(struct FTInfo* myFT);
int validateDataConnection(struct FTInfo* myFT);
void sendFileToClient(struct FTInfo* myFT);
void sendListingToClient(struct FTInfo* myFT);
int isTxtFile(char* filename);
int sendSuccessMessage(struct FTInfo* myFT, unsigned long long int bytesSent);
int sendErrorMessage(struct FTInfo* myFT);
char* copyToken(char* token);
void waitToCloseDataSocket(struct FTInfo* myFT);

#endif
