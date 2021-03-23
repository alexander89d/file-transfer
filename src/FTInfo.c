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
 * File Name:		FTInfo.c
 * File Description: 	Implementation of functions utilizing struct FTInfo. See FTInfo.h for struct
 * 			definition and description.
 * Course Name: 	CS 372-400: Introduction to Computer Networks
 * Last Modified:	03/09/2020
*****************************************************************************************************/

#include "FTInfo.h"


/***********************************************************************************************
 * Function Name:	newFTInfo
 * Description:		Allocates memory for and initializes a new struct FTInfo pointer.
 * Receives: 		A control socket file descriptor representing a socket
 * 			already connected to a client host and a string representing
 * 			the client host's address.
 * Returns: 		Newly-allocated, initialized struct FTInfo pointer.
 * Pre-Conditions: 	controlSocketFD represents a socket already connected to the client,
 * 			and clientHost represents that client's address.
 * Post-Conditions: 	The newly-allocated struct pointer has its controlSocketFD and
 * 			clientHost set to values passed in and all other
 * 			values set to NULL since those values have not yet been received
 * 			from client.
**********************************************************************************************/

struct FTInfo* newFTInfo(int controlSocketFD, char* clientHost)
{
	/* Allocate memory for new struct FTInfo. */
	struct FTInfo* myFT = (struct FTInfo*)malloc(sizeof(struct FTInfo));

	/* Set controlSocketFD and clientHost to values passed in. */ 
	myFT->controlSocketFD = controlSocketFD;
	myFT->clientHost = clientHost;
	
	/* Set clientNickname to value returned by function getNickname
	 * (will be name of flip server or IPv4 address if this is not a flip server). */
	myFT->clientNickname = getNickname(clientHost);

	/* Set dataPort, command, and filename to NULL,
	 * indicating that these have not yet been loaded with data from accepted client connection. */
	myFT->dataPort = NULL;
	myFT->command = NULL;
	myFT->filename = NULL;
	
	/* Set dataSocketFD to invalid value of -5 to indicate that it has not been connected to client yet. */
	myFT->dataSocketFD = -5;

	/* Return pointer to newly-allocated struct to calling function. */
	return myFT;
}


/***********************************************************************************************
 * Function Name:	getNickname
 * Description:		Gets the nickname of the flip server associated with clientHost's IPv4
 * 			address. If client is not running on a flip server, returns a copy of
 * 			clientHost (full IPv4 address).
 * Receives: 		A string containing the IPv4 address of the host on which the client
 * 			is running.
 * Returns: 		The name of the flip server on which the client is running
 * 			(i.e. flip1, flip2, or flip3) or a copy of the IPv4 address on which
 * 			the client is running if client is not running on a flip server.
 * Pre-Conditions: 	The string received represents the IPv4 address
 * 			of the client to which ftserver is already connected.
 * Post-Conditions: 	If the client is running on a flip server, the returned string
 * 			contains its nickname. Otherwise, returned string is a copy of its
 * 			IPv4 address.
**********************************************************************************************/

char* getNickname(char* clientHost)
{
	/* Declare char pointer to hold client host nickname.
	 * Initialize NULL to indicate name not assigned yet. */
	char* nickname = NULL;

	/* If clientHost matches flip1's IPv4 address, set nickname to flip1. */ 
	if (strcmp(clientHost, FLIP1) == 0)
	{
		nickname = "flip1";
	}

	/* Otherwise, if clientHost matches flip2's IPv4 address, set nickname to "flip2". */
	else if (strcmp(clientHost, FLIP2) == 0)
	{
		nickname = "flip2";
	}
	
	/* Otherwise, if clientHost matches flip3's IPv4 address, set nickname to "flip3". */
	else if (strcmp(clientHost, FLIP3) == 0)
	{
		nickname = "flip3";
	}
	
	/* Otherwise, set nickname to clientHost since the nickname is same
	 * as full host name (IPv4 address). */
	else
	{
		nickname = clientHost;
	}

	/* Allocate a new string to hold the nickname, copy the nickname into that string,
	 * and return the newly-allocated string to the calling function. */
	int nicknameBufferLen = strlen(nickname) + 1;
	char* nicknameCopy = (char*)malloc(nicknameBufferLen * sizeof(char));
	memset(nicknameCopy, '\0', nicknameBufferLen);
	strcpy(nicknameCopy, nickname);
	return nicknameCopy;
}


/***********************************************************************************************
 * Function Name:	deleteFTInfo
 * Description:		Deallocates memory previously allocated for the passed in struct
 * 			FTInfo pointer and closes its data socket (if ever created).
 * Receives: 		A struct FTInfo pointer.
 * Returns: 		nothing
 * Pre-Conditions: 	The passed in pointer represents a previously-allocated struct
 * 			FTInfo pointer.
 * Post-Conditions: 	All memory dynamically allocated to the passed-in struct pointer and
 * 			its data members has been freed, and its data socket has been closed
 * 			(if ever created).
**********************************************************************************************/

void deleteFTInfo(struct FTInfo* myFT)
{
	/* Free clientHost and clientNickname. */
	free(myFT->clientHost);
	myFT->clientHost = NULL;
	free(myFT->clientNickname);
	myFT->clientNickname = NULL;

	/* Free dataPort if it is non-null
	 * (only would be null if initial message received from client does not contain a data port). */
	if (myFT->dataPort != NULL)
	{
		free(myFT->dataPort);
		myFT->dataPort = NULL;
	}

	/* Free command if it is non-null (only would be null if error receiving command from client). */
	if (myFT->command != NULL)
	{
		free(myFT->command);
		myFT->command = NULL;
	}

	/* Free filename if it is non-null (may be null either if error receiving command
	 * or client requests a command that does not involve a specific filename). */
	if (myFT->filename != NULL)
	{
		free(myFT->filename);
		myFT->filename = NULL;
	}
	
	/* If a dataSocket has been connected to the client, close it. */
	if (myFT->dataSocketFD >= 0)
	{
		close(myFT->dataSocketFD);
	}

	/* Free myFT struct itself. */
	free(myFT);
	myFT = NULL;
}
