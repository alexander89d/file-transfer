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
 * File Name:		ftserver.c
 * File Description: 	Implementation file for main function (see below for function description).
 * Course Name: 	CS 372-400: Introduction to Computer Networks
 * Last Modified:	03/09/2020
*****************************************************************************************************/

#include "manageConnections.h"


/***********************************************************************************************
 * Function Name:	main
 * Description:		Entry point for ftserver execution. Receives command line arguments.
 * 			Validates that only 1 argument was entered after the program name on the
 * 			command line and that that argument is a non-negative integer for
 * 			representing a port number. Upon validation of command line arguments,
 * 			calls startup(), from which other functions handling communication are called.
 * 			When startup() returns after SIGINT is received, main() returns.
 * Receives: 		An array of strings representing command line arguments.
 * Returns: 		Error code 3 upon unexpected return (calls startup function which, in turn,
 * 			calls a function which enters an endless loop. Signal handler registered
 * 			to SIGINT should cause process to exit with status code 0).
 * Pre-Conditions: 	The command line arguments consist only of the program name and a
 * 			port number on which to establish a listening socket.
 * Post-Conditions: 	Unless the process has exited due to an error establishing a listening
 * 			socket, the listening socket has been shut down by a signal handler
 * 			once a SIGINT is received, and that signal handler has exited the 
 * 			process with status code 0. Status code 3 indicates unxpected
 * 			return from main.
***********************************************************************************************/

int main(int argc, char** argv)
{
	/* If the incorrect number of arguments were entered, print error message and exit. */
	if (argc != 2)
	{
		fprintf(stderr, "USAGE: %s SERVER_PORT\n", argv[0]);
		exit(1);
	}

	/* Validate portnum entered on command line, printing error message and exiting if it is invalid format. */
	char* portnum = argv[1];
	if (!validatePortnum(portnum))
	{
		fprintf(stderr, "USAGE: %s SERVER_PORT\n", argv[0]);
		fprintf(stderr, "The SERVER_PORT entered is not a valid non-negative integer.\n");
		exit(1);
	}
	
	/* Call startup function, which will call functions necessary for communication with clients. */
	startup(portnum);
	
	/* In the unlikely event that startup returns (one of the functions it calls
	 * contains an endless loop), return status code 3 to indicate unknown error. */
	return 3;
}
