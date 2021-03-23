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
 * File Name:		manageConnections.c
 * File Description: 	Implementation file for multiple functions responsible for validating SERVER_PORT,
 * 			establishing a listening socket at that port, and then entering an endless loop 
 * 			awaiting new connections.
 * Course Name: 	CS 372-400: Introduction to Computer Networks
 * Last Modified:	03/09/2020
*****************************************************************************************************/

#include "manageConnections.h"

/* Global variable definitions. */
int listeningSocketFD = -5;			/* Listening socket file descriptor closed by SIGINT handler. */
char* serverPort = NULL;			/* Server port number; used when printing error messages. */

/***********************************************************************************************
 * Function Name:	validatePortnum
 * Description:		Verifies that portnum passed in consists of only digits (since portnums
 * 			must be non-negative integers).
 * Receives: 		Portnum represented as string.
 * Returns: 		1 if the portnum is only digits; 0 otherwise.
 * Pre-Conditions: 	The address of portnum is non-null.
 * Post-Conditions: 	It has been verified whether portnum is in an acceptable format.
**********************************************************************************************/

int validatePortnum(char* portnum)
{
	/* Scan portnum to ensure all characters are digits (since port must be non-negative). */
	for (int i = 0; i < strlen(portnum); i++)
	{
		/* If the character at the current index is not a digit,
		 * return 0 for false to indicate invalid format. */
		if (!isdigit(portnum[i]))
		{
			return 0;
		}	
	}

	/* Otherwise, since portnum is in valid format, return 1 for true. */
	return 1;
}


/***********************************************************************************************
 * Function Name:	startup
 * Description:		Creates listening socket, registers signal handler to close listening
 * 			socket upon SIGINT, and calls acceptConnection to enter main
 * 			server loop.
 * Receives: 		The desired portnum at which to bind the listening socket,
 * 			represented as a string.
 * Returns: 		nothing (Listening socket file descriptor stored in global variable
 * 			so that it can be accessed by signal handler.)
 * Pre-Conditions: 	Portnum has been validated to be a non-null string representing a
 * 			correctly-formatted non-negative integer. 
 * Post-Conditions: 	Unless error occurs while binding socket and opening it for listening
 * 			(causing the program to exit), the value stored in global variable
 * 			listeningSocketFD represents a socket that has been bound to portnum
 * 			and activated for listening for incoming connections.
**********************************************************************************************/

void startup(char* portnumIn)
{
	/* Store SERVER_PORT received on command line in global variable
	 * (for reference when referring to control connections in error messages). */
	serverPort = portnumIn;
	
	/* Create listening socket and print message to indicate server is now listening for connections
	 * on portnum. */
	listeningSocketFD = establishListeningSocket(serverPort);
	printf("Server listening on port %s.\n", serverPort);

	/* Register signal handler to close listening socket upon sigint. */
	setSIGINThandler();

	/* Call acceptConnection to enter main server loop of listening for
	 * and then accepting client connections. */
	acceptConnection();
}


/***********************************************************************************************
 * Function Name:	setSIGINThandler
 * Description:		Registers a signal handler to SIGINT for closing the listening socket
 * 			before exiting.
 * Receives: 		nothing
 * Returns: 		nothing
 * Pre-Conditions: 	listeningSocketFD represents a listening socket that has already
 * 			been bound to the desired port number and activated for listening.
 * Post-Conditions: 	Any SIGINT received will call the newly-registered SIGINT handler.
 * ** CITATION **	Concept of how to register signal handlers in C learned in 
 * 			CS-344-400 Fall 2019, Block 3.
**********************************************************************************************/

void setSIGINThandler()
{
	/* Declare sigaction struct for SIGINT signal, initializing as empty. */
	struct sigaction SIGINT_action;
	memset(&SIGINT_action, 0, sizeof(struct sigaction));

	/* Set data members of SIGINT_action struct so that signal handler is catchSIGINT,
	 * all other incoming signals are blocked until the signal handler returns, and no flags are set.  */
	SIGINT_action.sa_handler = catchSIGINT;
	sigfillset(&SIGINT_action.sa_mask);
	SIGINT_action.sa_flags = 0;

	/* Call sigaction function to set signal actions for when SIGINT is received. */
	sigaction(SIGINT, &SIGINT_action, NULL);
}


/***********************************************************************************************
 * Function Name:	catchSIGINT
 * Description:		Handles a SIGINT when one is raised. Closes the listening socket
 * 			and exits with status code 0 to indicate successful server shutdown.
 * Receives: 		The signal number of the signal raised.
 * Returns: 		nothing
 * Pre-Conditions: 	listeningSocketFD represents a listening socket that has previously
 * 			been bound to the desired port number and activated for listening.
 * Post-Conditions: 	The listening socket has been closed and the process has exited.
**********************************************************************************************/

void catchSIGINT(int signo)
{
	close(listeningSocketFD);
	exit(0);
}


/***********************************************************************************************
 * Function Name:	acceptConnection
 * Description:		Loops between accepting and handling incoming client connections
 * 			on the listening socket until SIGINT is received.
 * Receives: 		nothing (listeningSocketFD is stored in global variable)
 * Returns: 		nothing
 * Pre-Conditions: 	listeningSocketFD represents a socket that has been bound to the 
 * 			desired port and activated for listening. SIGINT handler has been
 * 			registered to catch SIGINTs.
 * Post-Conditions: 	Once SIGINT is received, signal handler closes listening socket
 * 			and exits process with status code 0.
**********************************************************************************************/

void acceptConnection()
{
	/* Enter endless loop to accept and then handle client connections
	 * until SIGINT is received. */
	while (1)
	{
		/* Print message indicating that server is awaiting new connection. */
		printf("Awaiting new connection...\n");
		
		/* Accept connection, receiving new FTInfo returned (or NULL on error). If
		 * error occurs, continue to next iteration. */
		struct FTInfo* myFT = acceptClientConnection(listeningSocketFD);
		if (myFT == NULL)
		{
			continue;
		}
		
		/* Print that connection received from myFT->clientNickname (which will be
		 * flip server or IP address). */
		printf("Connection from %s\n", myFT->clientNickname);
		
		/* Receive and validate initial message with data port
		 * from client. If invalid conection, free FTInfo and continue
		 * to next iteration. */
		if (!validateControlConnection(myFT))
		{
			deleteFTInfo(myFT);
			myFT = NULL;
			continue;
		}

		/* Handle request. */
		handleRequest(myFT);

		/* Delete FTInfo (which will also close its data socket if ever created). */
		deleteFTInfo(myFT);
		myFT = NULL;
	}
}


/***********************************************************************************************
 * Function Name:	validateControlConnection
 * Description:		Ensures that initial message received from client is in the expected
 * 			format ("DATA_PORT: <portnum>"). Sends a greeting in response to the
 * 			client if client's initial message is valid and error message
 * 			to client otherwise.
 * Receives: 		A struct FTInfo containing information about the client.
 * Returns: 		True if client is validated based on initial message; false otherwise.
 * Pre-Conditions: 	The struct FTInfo has been allocated and initialized with a controlSocketFD
 * 			connected to the client as well as information about the client's host.
 * Post-Conditions: 	If true is returned, the dataport requested by the client
 * 			has been stored in the struct FTInfo passed into the function.
**********************************************************************************************/

int validateControlConnection(struct FTInfo* myFT)
{
	/* Receive initial message from client, returning false if NULL message received. */
	char* messageFromClient = recvMessage(myFT->controlSocketFD);
	if (messageFromClient == NULL)
	{
		return 0;
	}

	/* Ensure message from client is in the form "DATA_PORT: <portnum>". First,
	 * declare variables for use with strtok_r. */
	int messageError = 0;	/* Flag to track if message contains error. Set upon error detection. */
	char* saveptr;		/* Pointer to save place in strtok_r call. */
	
	/* Get first token of message. If it is NULL or not "DATA_PORT:", set messageError flag. */
	char* token1 = strtok_r(messageFromClient, " ", &saveptr);
	if (token1 == NULL || strcmp(token1, "DATA_PORT:") != 0)
	{
		messageError = 1;
	}

	/* If no message error detected yet, get second token of message. */
	if (!messageError)
	{
		char* token2 = strtok_r(NULL, " ", &saveptr);
		
		/* If token2 is null or not in valid port number format, set messageError flag. */
		if (token2 == NULL || !validatePortnum(token2))
		{
			messageError = 1;
		}

		/* Otherwise, if a third call to strtok_r does not return NULL, set
		 * messageErr flag to indicate too many tokens. */
		else if (strtok_r(NULL, " ", &saveptr) != NULL)
		{
			messageError = 1;
		}

		/* Otherwise, since token2 is portnum in valid format and is the last token,
		 * store token2 in myFT->dataPort. */
		else
		{
			myFT->dataPort = copyToken(token2);
		}
	}

	/* Free messageFromClient now that it has been parsed. */
	free(messageFromClient);
	messageFromClient = NULL;
	
	/* If an error was detected, report error to client program and return 0 to indicate invalid connection. */
	if (messageError)
	{
		char* errMessage = "MESSAGE FORMAT ERROR: Initial message must be formatted as: \"DATA_PORT: <portnum>\"";
		
		/* If there is no error sending error message, print message
		 * to console explaining invalid message format received. */
		if (sendMessage(myFT->controlSocketFD, errMessage) == 0)
		{
			fprintf(stderr, "%s\n", errMessage);
		}
		
		/* Return 0 to calling function to indicate invalid connection. */
		return 0;
	}

	/* Otherwise, send message to client informing them they have successfully connected to this server and 
	 * return 1 to indicate valid connection. */
	else
	{
		char* successMessage = "FTSERVER CONNECTION ESTABLISHED";
		
		/* Attempt to send message, returning 0 to calling function upon error. */
		if (sendMessage(myFT->controlSocketFD, successMessage) == -1)
		{
			return 0;
		}

		/* Otherwise, return 1 since sending message was successful. */
		return 1;
	}
}


/***********************************************************************************************
 * Function Name:	handleRequest
 * Description:		Receives and interprets client's request. If valid request, calls
 * 			apporpriate function to process it. If invalid request, sends error
 * 			to client.
 * Receives: 		struct FTInfo containing information about the connection to
 * 			the client.
 * Returns: 		nothing
 * Pre-Conditions: 	The struct FTInfo has been allocated and initialized with a controlSocketFD
 * 			connected to the client, information about the client's host, and 
 *			the port on which the client is listening for a data connection.
 * Post-Conditions: 	The client's request has been fulfilled or an error message
 * 			has been sent to the client.
**********************************************************************************************/

void handleRequest(struct FTInfo* myFT)
{
	/* Get and validate client request, returning from this function
	 * if a NULL message is received. */
	char* clientRequest = recvMessage(myFT->controlSocketFD);
	if (clientRequest == NULL)
	{
		return;
	}

	/* Declare variable for use with strtok_r. */
	int requestError = 0;		/* Flag set if error detected with request. */
	char* saveptr;			/* Pointer used by strtok_r to save place in string. */
	char* errMessage = NULL;	/* Error message to send to client, if applicable. */

	/* Get first token of string. */
	char* token1 = strtok_r(clientRequest, " ", &saveptr);

	/* If first token is NULL (clientRequest is string containing only spaces), set requestError. */
	if (token1 == NULL)
	{
		requestError = 1;
		errMessage = "NO COMMAND RECEIVED";
	}
	
	/* Otherwise, get second token (which should be NULL unless -g command is received,
	 * in which case it should be a filename). */
	else
	{
		char* token2 = strtok_r(NULL, " ", &saveptr);

		/* See if token1 is GET_FILE command. */
		if (strcmp(token1, GET_FILE) == 0)
		{
			/* If token2 is null, set requestError flag. */
			if (token2 == NULL)
			{
				requestError = 1;
				errMessage = "BAD REQUEST: <filename> required after -g command.";
			}

			/* Otherwise, if there is an unexpected non-null token after token2, set requestError flag. */
			else if (strtok_r(NULL, " ", &saveptr) != NULL)
			{
				requestError = 1;
				errMessage = "BAD REQUEST: only <filename> should come after -g command.";
			}

			/* Otherwise, since command syntax appears valid, set command and filename
			 * of struct FTInfo. */
			else
			{
				myFT->command = copyToken(token1);
				myFT->filename = copyToken(token2);
			}
		}

		/* Otherwise, if token1 is -l, process it. */
		else if (strcmp(token1, LIST_FILES) == 0)
		{
			/* If there is an unexpected second token after -l command,
			 * set requestError flag. */
			if (token2 != NULL)
			{
				requestError = 1;
				errMessage = "BAD REQUEST: no arguments should appear after -l command.";
			}

			/* Otherwise, store command in struct FTInfo. */
			else
			{
				myFT->command = copyToken(token1);
			}
		}

		/* Otherwise, if token1 is -ltxt, process it. */
		else if (strcmp(token1, LIST_TXT_FILES) == 0)
		{
			/* If there is an unexpected second token after -ltxt command,
			 * set requestError flag. */
			if (token2 != NULL)
			{
				requestError = 1;
				errMessage = "BAD REQUEST: no arguments should appear after -ltxt command.";
			}

			/* Otherwise, store command in struct FTInfo. */
			else
			{
				myFT->command = copyToken(token1);
			}
		}

		/* Otherwise, command is invalid. Set requestError flag. */
		else
		{
			requestError = 1;
			errMessage = "UNRECOGNIZED COMMAND: Accepted commands are -l, -ltxt, and -g <filename>.";
		}
	}

	/* Free clientRequest now that it has been tokenized and tokens have either been copied to struct FTInfo
	 * (if valid) or can be completely discarded (if invalid). */
	free(clientRequest);
	clientRequest = NULL;

	/* If there was a request error, send error message to client on control socket, print error message
	 * upon send success, and return control to calling function. */
	if (requestError)
	{
		if (sendMessage(myFT->controlSocketFD, errMessage) == 0)
		{
			fprintf(stderr, "%s\n", errMessage);
		}
		return;
	}

	/* Otherwise, establish a data connection with the client, sending initial message and receiving response
	 * to validate connection. Return control to calling function upon validation failure. */
	else if (!validateDataConnection(myFT))
	{
		return;
	}

	/* Now that data connection has been established and validated, call appropriate request handler
	 * based on command received. If command received is GET_FILE, call sendFileToClient.  */
	if (strcmp(myFT->command, GET_FILE) == 0)
	{
		sendFileToClient(myFT);
	}

	/* Otherwise, command is -l or -ltxt. Call sendListingToClient. */
	else
	{
		sendListingToClient(myFT);
	}
}


/***********************************************************************************************
 * Function Name:	validateDataConnection
 * Description:		Connects data socket of struct FTInfo received to client. Sends initial
 * 			validation message to client and receives validation response.
 * Receives: 		A pointer to a struct FTInfo.
 * Returns: 		True if the data socket was connected to the client and validation messages
 * 			sent and received correctly; false otherwise.
 * Pre-Conditions: 	myFT points to a struct FTInfo that has already been allocated and
 * 			initialized with controlSocket connected to client and client address.
 * Post-Conditions: 	If true is returned, data socket is connected to client, initial
 * 			validation messages have been sent and received, and data socket is ready
 * 			for sending data client requested.
**********************************************************************************************/

int validateDataConnection(struct FTInfo* myFT)
{
	/* Establish data socket, returning false upon error. */
	int socketFDReturned = establishDataSocket(myFT->clientHost, myFT->dataPort);
	if (socketFDReturned == -1)
	{
		return 0;
	}
	
	/* Since valid socketFD was returned, store it as data socket in myFT. */
	myFT->dataSocketFD = socketFDReturned;

	/* Send initial validation message to client on data connection, returning false if error. */
	char* validationMessage = "FTSERVER DATA CONNECTION INITIALIZATION";
	if (sendMessage(myFT->dataSocketFD, validationMessage) == -1)
	{
		return 0;
	}

	/* Receive initial response from client, returning false if NULL message received. */
	char* responseReceived = recvMessage(myFT->dataSocketFD);
	if (responseReceived == NULL)
	{
		return 0;
	}

	/* Otherwise, compare message from client to expected message and free
	 * responseReceived since it is no longer needed. */
	char* responseExpected = "FTSERVER DATA CONNECTION ACCEPTED";
	int strcmpResult = strcmp(responseReceived, responseExpected);

	/* If the received response is not that which was expected, report error, free the responseReceived, 
	 * and return false. */
	if (strcmpResult != 0)
	{
		/* Print error message informing user of client response expected and that received. */
		fprintf(stderr, "DATA CONNECTION VALIDATION ERROR: Invalid response from client.\n");
		fprintf(stderr, "Response expected on data connection: %s\n", responseExpected);
		fprintf(stderr, "Response received on data connection: %s\n", responseReceived);

		/* Free responseReceived and return false to calling function. */
		free(responseReceived);
		responseReceived = NULL;
		return 0;
	}

	/* Otherwise, the response received is valid. Free the response received and return true to calling function. */
	else
	{
		free(responseReceived);
		responseReceived = NULL;
		return 1;
	}
}


/***********************************************************************************************
 * Function Name:	sendFileToCLient
 * Description:		Attempts to send the requested file to the client. If unable to open
 * 			file, sends error message to client on control socket. Otherwise, reads
 * 			up to MAX_SEND_SIZE bytes of file at a time, sending them to client on data
 * 			socket. Finally, sends confirmation message to client over control socket
 * 			of number of bytes sent over data socket.
 * Receives: 		A struct FTInfo pointer.
 * Returns: 		nothing
 * Pre-Conditions: 	The controlSocketFD and dataSocketFD refer to connections already
 * 			established with the client.
 * Post-Conditions: 	Either all bytes of the file have been sent out through the data
 * 			socket and a confirmation message has been sent through the
 * 			control socket, or an error message has been sent through the
 * 			control socket.
**********************************************************************************************/

void sendFileToClient(struct FTInfo* myFT)
{
	/* Print info about request. */
	printf("File \"%s\" requested on port %s.\n", myFT->filename, myFT->dataPort);
	
	/* Open file with filename requested for reading, sending error message to client
	 * and returning upon error. */
	int fileToSend = open(myFT->filename, O_RDONLY);
	if (fileToSend < 0)
	{
		sendErrorMessage(myFT);
		return;
	}

	/* Since file was opened successfully, print message about sending it to client. */
	printf("Sending \"%s\" to %s:%s\n", myFT->filename, myFT->clientNickname, myFT->dataPort);
	
	/* Declare buffer to hold up to MAX_SEND_SIZE bytes read from file per iteration
	 * in loop below and accumulator to keep track of total number of bytes read. */
	char readBuffer[MAX_SEND_SIZE + 1];
	memset(readBuffer, '\0', sizeof(readBuffer));
	unsigned long long int totalCharsRead = 0;

	/* Loop until EOF is reached (read call returns 0) or error occurs (read call returns -1). */
	int charsRead = -5;	/* Keeps track of chars read each iteration. */
	do
	{
		/* Get up to MAX_SEND_SIZE chars from file, storing them in buffer. */
		charsRead = read(fileToSend, readBuffer, MAX_SEND_SIZE);

		/* If chars were read, send them to client. */
		if (charsRead > 0)
		{
			/* Set the byte after the last byte read into the buffer to null terminator, and update
			 * totalCharsRead. */
			readBuffer[charsRead] = '\0';
			totalCharsRead += charsRead;

			/* Attempt to send bytes just read to client over data connection,
			 * returning control to calling function upon error. */
			if (sendMessage(myFT->dataSocketFD, readBuffer) == -1)
			{
				return;
			}
		}
	} while (charsRead > 0);
	
	/* Close file now that it is no longer in use. */
	close(fileToSend);

	/* Now that loop above has exited, send success or error message to client accordingly. If charsRead is 0 
	 * and function has not returned, all chars have been successfully read from file (since eof has been reached) 
	 * and sent out through dataSocket. */
	if (charsRead == 0)
	{
		sendSuccessMessage(myFT, totalCharsRead);
	}

	/* Otherwise, reading error has occurred. Send error message to client
	 * over control socket*/
	else
	{
		sendErrorMessage(myFT);
	}
}


/***********************************************************************************************
 * Function Name:	sendListingToClient
 * Description:		Sends either listing of all files in current directory to client
 * 			(if command is LIST_FILES) or listing of all files in current directory
 * 			with .txt extension to client (if command is LIST_TXT_FILES). 
 * Receives: 		A pointer to a struct FTInfo.
 * Returns: 		nothing
 * Pre-Conditions: 	The struct FTInfo pointer has been allocated. controlSocketFD
 * 			and dataSocketFD represent connections successfully established
 * 			with the client. command is non-null and is either LIST_FILES
 * 			or LIST_TXT_FILES
 * Post-Conditions: 	Unless error occurs during sending, the requested listing has been
 * 			sent to the client over the data connection and a success message
 * 			has been sent to the client over the control connection.
**********************************************************************************************/

void sendListingToClient(struct FTInfo* myFT)
{
	/* Declare flag that keeps track of whether or not to include all files or just .txt files.
	 * Initialize it to being set, clearing it if command is -ltxt. */
	int includeAllFiles = 1;

	/* Declare request messages for printing what client has requested, initializing them as if client
	 * has requested all files and updating them if client has requested only .txt files. */
	char* requestMessage1 = "List directory requested on port ";
	char* requestMessage2 = "Sending directory contents to ";
	
	/* If the command is LIST_TXT_FILES, clear includeAllFiles flag and update
	 * request messages. */
	if (strcmp(myFT->command, LIST_TXT_FILES) == 0)
	{
		includeAllFiles = 0;
		requestMessage1 = "List directory .txt files requested on port ";
		requestMessage2 = "Sending directory .txt filenames to ";
	}

	/* Print requestMessage1 to console indicating what was requested. */
	printf("%s%s.\n", requestMessage1, myFT->dataPort);

	/* Attempt to open current directory, sending error message to client upon failure before returning
	 * control to calling function. */
	DIR* currentDir = opendir(".");
	if (currentDir == NULL)
	{
		sendErrorMessage(myFT);
		return;
	}

	/* Print requestMessage2 to indicate that directory is open and listing about to be sent to client. */
	printf("%s%s:%s\n", requestMessage2, myFT->clientNickname, myFT->dataPort);

	/* Since readdir call returns NULL both on error (in which case errno is set) and on reaching end of directory
	 * (in which case errno remains unchanged), reset errno to 0 in case it was set to a non-zero value previously. */
	errno = 0;

	/* Declare pointer to current directory entry to be updated in each iteration of the loop below, initializing it
	 * to the first directory entry. */
	struct dirent* currentEntry = readdir(currentDir);
	
	/* Declare a buffer to hold consecutive directory listings so that up to MAX_SEND_SIZE bytes worth of
	 * listing information can be sent in one call to sendMessage. */
	char sendBuffer[MAX_SEND_SIZE+1];
	memset(sendBuffer, '\0', sizeof(sendBuffer));

	/* Declare a pointer to the next position in sendBuffer into which to write new data, an int to keep
	 * track of number of chars currently in buffer, and an int to keep track of total chars sent. */
	char* posInSendBuffer = sendBuffer;
	int charsInSendBuffer = 0;
	unsigned long long int totalCharsSent = 0;

	/* Loop until end of directory is reached or error occurs. */
	while (currentEntry != NULL)
	{		
		/* Get the filename of the current directory entry. */
		char* currentFilename = currentEntry->d_name;

		/* Only add currentFilename to send buffer if either all files should be included
		 * or currentFilename ends with the .txt extension. */
		if (includeAllFiles || isTxtFile(currentFilename))
		{
			/* Since this file should be included in the directory listing,
		 	 * add it + a newline character to separate it from the next directory entry
			 * to the send buffer. Determine entry length (length of filename + 1 char for newline
			 * character). */
			int entryLen = strlen(currentFilename) + 1;

			/* If there is not enough room for this filename in the send buffer,
			 * send the contents of the send buffer, empty the send buffer, and reset
			 * posInSendBuffer and charsInSendBuffer before adding this new filename. */
			if (charsInSendBuffer + entryLen > MAX_SEND_SIZE)
			{
				/* Send current contents of sendBuffer to client over data socket,
				 * returning control to calling function upon failure. */
				if (sendMessage(myFT->dataSocketFD, sendBuffer) == -1)
				{
					return;
				}

				/* Update totalCharsSent, empty out the sendBuffer, 
				 * and reset charsInSendBuffer and posInSendBuffer. */
				totalCharsSent += charsInSendBuffer;
				memset(sendBuffer, '\0', sizeof(sendBuffer));
				charsInSendBuffer = 0;
				posInSendBuffer = sendBuffer;
			}
			
			/* Print filename + newline character into the send buffer
			 * starting at the current position, updating charsInSendBuffer
			 * and posInSendBuffer accordingly. */
			sprintf(posInSendBuffer, "%s\n", currentFilename);
			posInSendBuffer += entryLen;
			charsInSendBuffer += entryLen;
		}	

		/* Get next directory entry in preparation for next iteration. */
		currentEntry = readdir(currentDir);
	}

	/* Close directory now that loop above has finished processing it. */
	closedir(currentDir);

	/* If errno is a non-zero value, send error message to client. */
	if (errno != 0)
	{
		sendErrorMessage(myFT);
	}

	/* Otherwise, if includeAllFiles is false, charsInSendBuffer is 0, and totalCharsSent is 0,
	 * this indicates that there were no files with .txt extensions in this directory. 
	 * Send message to client accordingly. */
	else if (!includeAllFiles && charsInSendBuffer == 0 && totalCharsSent == 0)
	{
		/* Attempt to send message to client about there being no text files,
		 * returning upon failure to send. */
		char* noTxtFilesMessage = "There are no files with the .txt extension in this directory.";
		if (sendMessage(myFT->controlSocketFD, noTxtFilesMessage) == -1)
		{
			return;
		}

		/* Since message sent successfully, wait to close the data connection until client has received
		 * the message. */
		waitToCloseDataSocket(myFT);
	}

	/* Otherwise, since there were no errors sending data, send any bytes remaining in sendBuffer to client, 
	 * and send success message to client to indicate full directory
	 * listing has been sent on data connection. */
	else
	{
		/* If there are any bytes remaining in sendBuffer, send them. */
		if (charsInSendBuffer > 0)
		{
			/* Attempt to send chars, returning control to calling function upon send error. */
			if (sendMessage(myFT->dataSocketFD, sendBuffer) == -1)
			{
				return;
			}

			/* Update totalCharsSent with the number of bytes just sent. */
			totalCharsSent += charsInSendBuffer;
		}
		
		/* Send success message with total number of chars sent to client. */
		sendSuccessMessage(myFT, totalCharsSent);
	}
}


/***********************************************************************************************
 * Function Name:	isTxtFile
 * Description:		Verifies whether or not the filename passed in ends with the .txt
 * 			extension.
 * Receives: 		A string representing a filename.
 * Returns: 		1 if the filename ends with the .txt extension; 0 otherwise.
 * Pre-Conditions: 	Filename is a non-null string.
 * Post-Conditions: 	It has been verified whether or not filename
 * 			ends with the .txt extension.
**********************************************************************************************/

int isTxtFile(char* filename)
{
	/* Get length of filename since it will be used mutliple times
	 * in calculations below. */
	int filenameLen = strlen(filename);
	
	/* If filename is shorter than TXT_EXTENSION_LEN, return 0 since filename cannot possibly
	 * end with TXT_EXTENSION. */
	if (filenameLen < TXT_EXTENSION_LEN)
	{
		return 0;
	}

	/* Otherwise, see if last 4 characters of filename exactly match TXT_EXTENSION. Get starting
	 * position of final 4 characters of filename. */
	int offset = filenameLen - TXT_EXTENSION_LEN;
	char* finalChars = filename + offset;

	/* If finalChars are TXT_EXTENSION, return 1 for true. */
	if (strcmp(finalChars, TXT_EXTENSION) == 0)
	{
		return 1;
	}

	/* Otherwise, return 0 since file does not end with .txt extension. */
	else
	{
		return 0;
	}
}


/***********************************************************************************************
 * Function Name:	sendSuccessMessage
 * Description:		Sends a success message to the client through the control socket
 * 			with the number of bytes sent through the data socket to indicate
 * 			that all requested data has been sent through the data socket.
 * 			If sending through control socket succeeds,
 * 			waits for the client to close control socket before returning
 * 			(and closing data connection) to ensure client has received
 * 			everything sent through control and data sockets.
 * Receives: 		A struct FTInfo pointer and the number of bytes sent to the client
 * 			over the data socket.
 * Returns: 		0 on send success; -1 on failure.
 * Pre-Conditions: 	The struct FTInfo pointer has been allocated with a controlSocket
 * 			connected to the client, and bytesSent represents
 * 			the actual number of bytes sent out to the client over
 * 			the data socket. All requested data has actually been sent out.
 * Post-Conditions: 	Unless error occurs during sending, the success message
 * 			has successfully been sent out to the client over the
 * 			control socket, and the process has waited for the
 * 			client to close the control socket before returning.
**********************************************************************************************/

int sendSuccessMessage(struct FTInfo* myFT, unsigned long long int bytesSent)
{
	/* Declare prefix to come before number of bytes sent,
	 * and suffix to come after number of bytes sent. */
	char* successPrefix = "SUCCESS! ";
	char* successSuffix = " bytes sent over data connection.";

	/* Declare buffer to hold full message. Allocate with enough room
	 * for successPrefix, successSuffix, and number of bytes sent. */
	int successMessageBufferLen = strlen(successPrefix) + MAX_ULLINT_DIGITS + strlen(successSuffix) + 1;
	char successMessage[successMessageBufferLen];
	memset(successMessage, '\0', sizeof(successMessage));

	/* Write full success message of prefix + bytesSent + suffix into successMessage string. */
	sprintf(successMessage, "%s%llu%s", successPrefix, bytesSent, successSuffix);

	/* Send success message to client over control socket, returning -1 upon error. */
	if (sendMessage(myFT->controlSocketFD, successMessage) == -1)
	{
		return -1;
	}

	/* Otherwise, no error detected sending message. Wait for client to close
	 * data connection to be sure client has received all data sent through control and data connection
	 * before returning 0 to indicate success. */
	waitToCloseDataSocket(myFT);
	return 0;
}


/***********************************************************************************************
 * Function Name:	sendErrorMessage
 * Description:		Sends an error message to the client through the control socket
 * 			indicating the error specified by errno. If sending through control
 * 			socket succeeds, waits for the client to close control socket before
 * 			returning (and closing data connection) to ensure client has received
 * 			everything sent through control and data sockets.
 * Receives: 		A struct FTInfo pointer.
 * Returns: 		0 on send success; -1 on failure.
 * Pre-Conditions: 	The struct FTInfo pointer has been allocated with a controlSocket
 * 			connected to the client, and errno has been set to a non-zero
 * 			value by this process's most recent attempt to get the data requested
 * 			to send to the client.
 * Post-Conditions: 	Unless error occurs during sending, the error message
 * 			has successfully been sent out to the client over the
 * 			control socket, and the process has waited for the
 * 			client to close the control socket before returning.
**********************************************************************************************/

int sendErrorMessage(struct FTInfo* myFT)
{
	/* Get error that errno represents as string. */
	char* errMessage = strerror(errno);

	/* If sending error message to client succeeds,
	 * print error message to screen and then wait to close data
	 * connection until client closes control connection before returning
	 * 0 to indicate success sending error message. */
	if (sendMessage(myFT->controlSocketFD, errMessage) == 0)
	{
		fprintf(stderr, "%s. Sending error message to %s:%s\n",
			errMessage, myFT->clientNickname, serverPort);
		waitToCloseDataSocket(myFT);
		return 0;
	}

	/* Otherwise, simply return -1 to indicate error sending message
	 * (without awaiting client closing control connection since there was an error
	 * utilizing control connection). */
	else
	{
		return -1;
	}
}


/***********************************************************************************************
 * Function Name:	copyToken
 * Description:		Allocates and returns a new string holding a copy of the token
 * 			passed in. Used so that copies of string tokens are kept when
 * 			original tokenized string is deleted.
 * Receives: 		A pointer to the copy to token.
 * Returns: 		A pointer to a newly-allocated string containing the token copy.
 * Pre-Conditions: 	Token is a non-null string.
 * Post-Conditions: 	The returned string contains a copy of the token.
**********************************************************************************************/

char* copyToken(char* token)
{
	/* Allocate tokenCopy to hold token. */
	int tokenCopyBufferLen = strlen(token) + 1;
	char* tokenCopy = (char*)malloc(tokenCopyBufferLen * sizeof(char));
	memset(tokenCopy, '\0', tokenCopyBufferLen);

	/* Copy token into tokenCopy and return tokenCopy to calling function. */
	strcpy(tokenCopy, token);
	return tokenCopy;
}


/***********************************************************************************************
 * Function Name:	waitToCloseDataSocket
 * Description:		Waits for the client to finish reading from the control socket and
 * 			data socket (indicated by the client closing the control socket)
 * 			before returning.
 * Receives: 		A struct FTInfo pointer.
 * Returns: 		nothing
 * Pre-Conditions: 	The controlSocket and dataSocket have been successfully connected
 * 			to the client. No more data will be sent to the client,
 * 			and no more messages are expected to be received from the client.
 * Post-Conditions: 	When function returns, it is safe to assume the client has closed the
 * 			control connection and that the data connection can now be closed.
 * ** CITATIONS **	Function adapted from my implementation of a function with a
 * 			similar purpose in the Block 4 Project in CS 344-400 Fall 2019.
**********************************************************************************************/

void waitToCloseDataSocket(struct FTInfo* myFT)
{
	/* Attempt to read 1 character from client on control socket (no more data is expected on control socket),
	 * which will cause process to block until client shuts down control socket. */
	char waitBuff[1];
	recv(myFT->controlSocketFD, waitBuff, sizeof(waitBuff), 0);
}
