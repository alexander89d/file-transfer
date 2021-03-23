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
 * File Description: 	Implementation file for functions related to the establishment, binding and connecting
 * 			of sockets as well as sending and receiving messages using sockets.
 * Course Name: 	CS 372-400: Introduction to Computer Networks
 * Last Modified:	03/09/2020
*****************************************************************************************************/

#include "clientServerMessaging.h"


/***********************************************************************************************
 * Function Name:	establishListeningSocket
 * Description:		Establishes a socket for listening for incoming connections at the
 * 			desired port passed in through the command line.
 * Receives: 		The desired port number at which the socket should listen, represented
 * 			as a string.
 * Returns: 		The file descriptor of the newly-established and enabled listening
 * 			socket upon success.
 * Pre-Conditions: 	The serverPort passed in is a valid nonnegative integer.
 * Post-Conditions: 	Unless error occurs (in which case process exits), the file descriptor
 * 			returned represents a socke that has been bound to the desired port
 * 			and opened for listening.
 * **CITATION**		Code for working with socket api adapted from:
 * 			Hall, Brian, "Beej’s Guide to Network Programming: Using Internet Sockets."
 * 			Retrieved 02/08/2020 from http://beej.us/guide/bgnet
***********************************************************************************************/

int establishListeningSocket(char* serverPort)
{
	/* Declare addrinfo structs for establishing connection. */
	struct addrinfo hints;		/* Contains info about desired connection. */
	struct addrinfo* addrList;	/* Will contain linked list of possible matches to client info. */
	
	/* Empty and initialize hints. */
	memset(&hints, 0, sizeof hints); 
	hints.ai_family = AF_INET; 		/* Allow only IPv4 connections. */
	hints.ai_socktype = SOCK_STREAM; 	/* Use a TCP socket. */
	hints.ai_flags = AI_PASSIVE;		/* Use this process's host's IP address. */
	
	/* Get linked list of address information, printing error and exiting if failure. */
	int gai_status = getaddrinfo(NULL, serverPort, &hints, &addrList);
	if (gai_status != 0)
	{
		fprintf(stderr, "GETADDRINFO ERROR: %s\n", gai_strerror(gai_status));
		exit(2);
	}

	/* Loop through addrList, connecting to the first process which will accept the connection. */
	int listeningSocketFD = -5;			/* Will store connected socket to return to caller. */
	int socketBound = 0;				/* Flag to track if socket successfully bound to port. */
	struct addrinfo* currentNode = addrList;	/* Address of current node in addrList. */
	while(currentNode != NULL && !socketBound)
	{
		/* Attempt to establish socket using current node's info. */
		listeningSocketFD = socket(currentNode->ai_family, currentNode->ai_socktype, currentNode->ai_protocol);
		
		/* If a socket was established successfully, attempt to bind the socket to serverPort. */
		if (listeningSocketFD != -1)
		{
			/* If binding socket is successful, set flag to true. */
			if (bind(listeningSocketFD, currentNode->ai_addr, currentNode->ai_addrlen) != -1)
			{
				socketBound = 1;
			}

			/* Otherwise, close socket in preparation for next iteration. */
			else
			{
				close(listeningSocketFD);
			}
		}
		
		/* Get next node in preparation for next iteration. */
		currentNode = currentNode->ai_next;
	}

	/* Now that the loop examining the address info list has exited,
	 * free addrList since it is no longer needed. */ 
	freeaddrinfo(addrList);
	addrList = NULL;

	/* If the socket was not bound successfully, print error message and exit. */
	if (!socketBound)
	{
		fprintf(stderr, "BIND ERROR: could not bind listening socket to port %s: ", serverPort);
		perror("");
		exit(2);
	}

	/* Activate listening socket so it will listen for incoming connections, reporting error and exiting
	 * upon failure. */
	if(listen(listeningSocketFD, MAX_BACKLOG) == -1)
	{
		perror("ERROR ENABLING SOCKET FOR LISTENING");
		exit(2);
	}

	/* Return listening socket file descriptor to calling function. */
	return listeningSocketFD;
}


/***********************************************************************************************
 * Function Name:	acceptClientConnection
 * Description:		Accepts an incoming client connection on the listening socket. 
 * 			Creates a new pointer to a struct FTInfo, filling in values of
 * 			controlSocketFD and clientHost.
 * Receives: 		The file descriptor of a listening socket.
 * Returns: 		A newly-allocated struct FTInfo pointer on success or NULL upon error.
 * Pre-Conditions: 	The integer passed in represents a valid listening socket that has
 * 			already been bound to a specific port and activated for listening.
 * Post-Conditions: 	Unless NULL is returned due to an error, the returned struct will
 * 			contain the file descriptor of the control connection to a newly
 * 			connected client and the address of that client's host.
 * ** CITATIONS **	- Info on how to implement accept() call learned from 
 * 			  CS-344-400 Fall 2019 Block 4 content and my implementation of a
 * 			  similar function in the Block 4 project in that same class.
 * 			- Info on how to look up client host name upon accepting connection
 * 			  from client learned from
 *			  Hall, Brian, "Beej’s Guide to Network Programming: Using Internet Sockets."
 * 			  Retrieved 02/08/2020 from http://beej.us/guide/bgnet
**********************************************************************************************/

struct FTInfo* acceptClientConnection(int listeningSocketFD)
{
	int controlSocketFD;			/* File descriptor of newly-accepted client connection. */
	socklen_t sizeOfClientInfo;		/* Size of clientInfo struct. */
	struct sockaddr_in clientInfo;		/* Struct to hold info about client's address. */
	
	/* Get the size of the address for the client that will connect.
	 * Then, accept a connection, blocking if one is not available until one connects */
	sizeOfClientInfo = sizeof(clientInfo); 
	controlSocketFD = accept(listeningSocketFD, (struct sockaddr *)&clientInfo, &sizeOfClientInfo);
	
	/* If the accept call failed, print error message and return NULL to calling function. */
	if (controlSocketFD < 0)
	{
		perror("ACCEPT CONNECTION ERROR");
		return NULL;
	}
	
	/* Otherwise, get IPv4 address of client on other end of newly-accepted connection, allocating
	 * buffer for and storing it. */
	char* clientHost = (char*)malloc(INET_ADDRSTRLEN * sizeof(char));
	memset(clientHost, '\0', INET_ADDRSTRLEN);
	inet_ntop(AF_INET, &(clientInfo.sin_addr), clientHost, INET_ADDRSTRLEN);
	
	/* Allocate and return a new struct FTInfo pointer initialized with controlSocketFD and clientHost. */
	return newFTInfo(controlSocketFD, clientHost);
}


/***********************************************************************************************
 * Function Name:  	establishDataSocket
 * Description:		Creates a new socket for data transfer, connecting to the client at the
 * 			requested port.
 * Receives: 		Strings representing the client's host address and desired port number
 * 			for the new data port.
 * Returns: 		The file descriptor of the newly-established socket upon success
 * 			or -1 upon failure.
 * Pre-Conditions: 	clientHost represents the address of the client with which this process
 * 			has already connected, and dataPort represents the port number requested
 * 			by the client.
 * Post-Conditions: 	Unless -1 is returned to report a connection error,
 * 			the returned value represents the socket file descriptor
 * 			of the socket for data transfers that has been successfully
 * 			connected to the client.
 * **CITATION**		- Code for working with socket api adapted from:
 * 			  Hall, Brian, "Beej’s Guide to Network Programming: Using Internet Sockets."
 * 			  Retrieved 02/08/2020 from http://beej.us/guide/bgnet
 * 			- Adapted from my implementation of a similar function in CS-372
 * 			  Programming Assignment 1.
**********************************************************************************************/

int establishDataSocket(char* clientHost, char* dataPort)
{
	/* Declare addrinfo structs for establishing connection. */
	struct addrinfo hints;		/* Contains info about desired connection. */
	struct addrinfo* addrList;	/* Will contain linked list of possible matches to client info. */
	
	/* Empty and initialize hints. */
	memset(&hints, 0, sizeof hints); 
	hints.ai_family = AF_INET; 		/* Allow only IPv4 connections. */
	hints.ai_socktype = SOCK_STREAM; 	/* Use a TCP socket. */
	
	/* Get linked list of address information, printing error, and returning -1 to calling function if failure. */
	int gai_status = getaddrinfo(clientHost, dataPort, &hints, &addrList);
	if (gai_status != 0)
	{
		fprintf(stderr, "GETADDRINFO ERROR: %s\n", gai_strerror(gai_status));
		return -1;
	}

	/* Loop through addrList, connecting to the first process which will accept the connection. */
	int dataSocketFD = -5;				/* Will store connected socket to return to caller. */
	int socketConnected = 0;			/* Flag to track if socket connected. */
	struct addrinfo* currentNode = addrList;	/* Address of current node in addrList. */
	while(currentNode != NULL && !socketConnected)
	{
		/* Attempt to establish socket using current node's info. */
		dataSocketFD = socket(currentNode->ai_family, currentNode->ai_socktype, currentNode->ai_protocol);
		
		/* If a socket was established successfully, attempt to connect to client
		 * using current node's info. */
		if (dataSocketFD != -1)
		{
			/* If connecting socket is successful, set flag to true. */
			if (connect(dataSocketFD, currentNode->ai_addr, currentNode->ai_addrlen) != -1)
			{
				socketConnected = 1;
			}

			/* Otherwise, close socket in preparation for next iteration. */
			else
			{
				close(dataSocketFD);
			}
		}
		
		/* Get next node in preparation for next iteration. */
		currentNode = currentNode->ai_next;
	}
	
	/* Now that the loop examining the address info list has exited,
	 * free addrList since it is no longer needed. */ 
	freeaddrinfo(addrList);
	addrList = NULL;

	/* If the socket is not connected, print error message, and return -1 to calling function. */
	if (!socketConnected)
	{
		fprintf(stderr, "CONNECTION ERROR: could not connect to client at %s:%s: ", clientHost, dataPort);
		perror("");
		return -1;
	}

	/* Return socket file descriptor to be used for data transfers to calling function. */
	return dataSocketFD;
}


/***********************************************************************************************
 * Function Name:  	sendMessage
 * Description:		Sends the length of the message passed in to the client followed by 
 * 			the message itself.
 * Receives: 		The file descriptor of a socket connected to the client 
 * 			and the message entered by the user for sending.
 * Returns: 		0 on success; -1 on failure.
 * Pre-Conditions: 	socketFD refers to a socket which has successfully been connected
 * 			to the client, and the message is a non-null string.
 * Post-Conditions: 	If 0 is returned to indicate success, all bytes of the message have
 * 			succesfully been sent out to the transport layer.
 * ** CITATIONS **:	Concept of sending message length before sending message itself using
 * 			function that verifies sending of full strings learned from combination
 * 			of the following:
 * 			- CS 344-400 Fall 2019 Lectures.
 * 			- My implementation of the Block 4 program in that same class.
 * 			Function adapted from my implementation of a similar function 
 * 			in CS 372 Programming Assignment 1.
**********************************************************************************************/

int sendMessage(int socketFD, char* message)
{
	/* Send client message length so it knows how many characters to expect to receive.
	 * Declare buffer to hold message length + terminating '@' character
	 * to indicate end of message length string. */ 
	char messageLenStr[50];		/* Relatively small buffer needed to represent message length. */
	memset(messageLenStr, '\0', sizeof(messageLenStr));
	 
	/* Store message length + terminating '@' character in messageLenStr. */
	sprintf(messageLenStr, "%d@", (int)strlen(message));

	/* Send messageLenStr to server, returning -1 to calling function if error. */
	if (sendCompleteString(socketFD, messageLenStr) == -1)
	{
		return -1;
	}

	/* Send message to server, returning -1 to calling function if error. */
	if (sendCompleteString(socketFD, message) == -1)
	{
		return -1;
	}
	
	/* If no error occurred above, return 0 to calling function to indicate success. */
	return 0;
}


/***********************************************************************************************
 * Function Name:  	sendCompleteString
 * Description:		Sends string passed in to client, looping until full message has been
 * 			sent out on transport layer or send error has occurred.
 * Receives: 		The file descriptor of a messaging socket connected to the client
 * 			and the message to be sent.
 * Returns: 		0 on success, -1 on send error.
 * Pre-Conditions: 	socketFD refers to a socket which has successfully been connected
 * 			to the client, and the message is a non-null string.
 * Post-Conditions: 	If 0 is returned to indicate success, all bytes of the message have
 * 			succesfully been sent out to the transport layer.
 * ** CITATIONS **:	Function adapted from my implementation of a function with an
 * 			identical purpose in the Block 4 Project in CS 344-400 Fall 2019
 * 			and from my implementation of a function with identical purpose
 * 			in CS 372 Programming Assignment 1.
**********************************************************************************************/

int sendCompleteString(int messagingSocket, char* message)
{
	/* Loop until full message is sent. */
	int charsRemaining = strlen(message);		/* Number of chars remaining to be sent. */
	char* posInMessage = message;			/* Position of next character to send. */
	while (charsRemaining > 0)
	{
		/* Attempt to send up to charsRemaining bytes of message. */
		int charsSent = send(messagingSocket, posInMessage, charsRemaining, 0);
				
		/* If an error occurred, print error message and return -1 to calling function. */
		if (charsSent < 0)
		{
			perror("SEND ERROR");
			fprintf(stderr, "Disconnecting from client.\n");
			return -1;
		}

		/* Otherwise, update charsRemaining and address of next char to send
		 * in preparation for next iteration. Loop will terminate when charsRemaining hits 0. */
		charsRemaining -= charsSent;
		posInMessage += charsSent;
	}

	/* Return 0 to indicate successful sending. */	
	return 0;
}


/***********************************************************************************************
 * Function Name:  	recvMessage
 * Description:		Receives and returns a message from the client, ensuring that all bytes
 * 			of full message are read from transport layer.
 * Receives:		The file descriptor of a socket connected to the client.
 * Returns: 		The message received from the client as a string
 * 			(or NULL if error occurs).
 * Pre-Conditions: 	socketFD represents a socket previously successfully connected
 * 			to the client.
 * Post-Conditions: 	Unless an error occurs while calling recv and NULL is returned,
 * 			the full message has been read from the transport layer and returned.
 * ** CITATIONS **:	Function adapted from my implementation of functions with similar
 * 			purpose in the Block 4 Project in CS 344-400 Fall 2019 and from
 * 			my implementation of function with similar purpose in CS 372
 * 			Programming Assignment 1.
**********************************************************************************************/

char* recvMessage(int socketFD)
{
	/* Receive message length. Declare buffer of 12 characters to hold length of message. */
	char messageLenStr[12];
	memset(messageLenStr, '\0', sizeof(messageLenStr));

	/* Receive length 1 byte at a time, stopping after '@' character is received. */
	char* posInStr = messageLenStr;		/* Address of index within buffer to store next char read. */
	char endingChar = '\0';			/* Current ending char of message. */
	while(endingChar != '@')
	{
		/* Read up to 1 character from the socket. */
		int charsRead = recv(socketFD, posInStr, 1, 0); 

		/* If an error occurred, return NULL to calling function. */
		if (recvError(charsRead))
		{
			return NULL;
		}

		/* Get the value of the current ending char of the message,
		 * and update the position in the message for the next iteration. */
		endingChar = posInStr[charsRead-1];
		posInStr += charsRead;
	}

	/* Strip the terminating '@' character off of the message and convert it to an int. */
	messageLenStr[(int)strlen(messageLenStr) - 1] = '\0';
	int messageLen = atoi(messageLenStr);

	/* Allocate a buffer for the message. */
	int messageBufferLen = messageLen + 1;
	char* message = (char*)malloc(messageBufferLen * sizeof(char));
	memset(message, '\0', messageBufferLen);
	
	/* Receive message. Loop until full messageLen has been read or error occurs. */
	char* posInMessage = message;		/* Address of index within buffer to store next char read. */
	int charsRemaining = messageLen;	/* Max number of chars remaining to be read. */
	while(charsRemaining > 0)
	{
		/* Read up to charsRemaining characters from the socket. */
		int charsRead = recv(socketFD, posInMessage, charsRemaining, 0); 
		
		/* Check for receive error, freeing message and returning NULL if one occurred. */
		if (recvError(charsRead))
		{
			free(message);
			message = NULL;
			return NULL;
		}

		/* Update the position in the message for the next iteration, and update
		 * the number of chars remaining to be read. */
		posInMessage += charsRead;
		charsRemaining -= charsRead;
	}

	/* Now that full message has been read without error, return it to calling function. */
	return message;
}


/***********************************************************************************************
 * Function Name:  	recvError
 * Description:		Checks to see if an error occurred receiving message from client.
 * 			If error occurred, reports error to user.
 * Receives: 		The number of chars read in the last recv call (used to determine
 * 			if error occurred).
 * Returns: 		True if error occurred; false otherwise.
 * Pre-Conditions: 	charsRead represents the return value of a previous recv call.
 * Post-Conditions: 	If error ocurred as detected by recv return value, error is reported
 * 			and function returns true. Otherwise, function returns false.
**********************************************************************************************/

int recvError(int charsRead)
{
	/* If an error occurred (charsRead <= 0),
	 * report error and return true to calling function. */
	if (charsRead <= 0)
	{
		/* If charsRead is -1, report error in errno. */
		if (charsRead == -1)
		{
			perror("RECV ERROR");
		}

		else if (charsRead == 0)
		{
			fprintf(stderr, "RECV ERROR: Client disconnected\n");
		}

		/* Return 1 to indicate error. */
		return 1;
	}

	/* Otherwise, return false to calling function to indicate no error. */
	else
	{
		return 0;
	}
}
