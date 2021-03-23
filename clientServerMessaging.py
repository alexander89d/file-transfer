#######################################################################################################
# Programmer Name: 	Alexander Densmore
# Program Name: 	ftclient
# Program Description:	Implementation of the client side of a client-server file transfer protocol. 
#			Client receives SERVER_HOST, SERVER_PORT, COMMAND, FILENAME (if applicable),
#			and DATA_PORT from the command line. Once command-line arguments are validated,
#			attempts to establish a control connection at SERVER_HOST:SERVER_PORT. Then,
#			client awaits response at DATA_PORT, printing the response it receives, 
#			or client receives an error message at SERVER_PORT if the server could not 
#			fulfill the requested command, printing the error message.
# *** EXTRA CREDIT ***	In addition to the required commands of -l (list all files in current directory)
#			and -g (get file with filename), I have implemented an option -ltxt (list all files
#			in current directory with .txt extension). Upon receiving -ltxt command,
#			server filters current directory listing for only files with .txt extension,
#			either sending list of such files to client or reporting that there
#			are no files with .txt extension in the current directory.
# File Name:		clientServerMessaging.py
# File Description: 	File containing functions for sending and receiving data between client
#			and server. Invoked by various methods of FTInfo class.
# Course Name: 		CS 372-400: Introduction to Computer Networks
# Last Modified:	03/09/2020
######################################################################################################


import socket
import sys
import FTInfo

#######################################################################################################
# Function Name:  	sendCompleteString
# Description:		Sends string passed in to client, looping until full message has been
# 			sent out on transport layer or send error has occurred.
# Receives: 		A socket connected to the server, the message to be sent, and an FTInfo
#			object to be used for closing open sockets if error occurs before exiting.
#			Note that, although messagingSocket represents one of the sockets
#			stored in FTInfo, this parameter clarifies which of those sockets
#			to use for sending this message.
# Returns: 		Nothing
# Pre-Conditions: 	messagingSocket has successfully connected to the server, message is
#			a non-empty string, and myFT represents an instantiated FTInfo object.
# Post-Conditions: 	Either full message has been sent out to transport layer or send error
# 			has been reported to user (in which case program exits).
# ** CITATIONS: **:	- send() call based off example send() call in Lecture 15, Slide 9.
#			- Use of str.encode() learned from Python 3 documentation:
#			  https://docs.python.org/3.6/library/stdtypes.html#str.encode
#			- Concept of looping until full message is sent based on information
#			  provided in the following article: 
#			  McMillan, G. Socket Programming HOWTO. Accessed 02/09/2020 from:
#			  https://docs.python.org/3.6/howto/sockets.html
# 			- Function adapted from my implementation of a function with an
# 			  identical purpose in CS 372 Programming Assignment 1.
#######################################################################################################

def sendCompleteString(messagingSocket, message, myFT):
	# Convert message string to sequence of bytes for sending to client.
	messageAsBytes = message.encode()
	
	# Loop until full message is sent. */
	totalCharsSent = 0
	chunkLen = 0
	
	while totalCharsSent < len(message):
		# Attempt to send remainder of message to client.
		try:
			chunkLen = messagingSocket.send(messageAsBytes[totalCharsSent:])
			
		# If an error occurred, print error message and exit.
		except OSError as socketError:
			print("SEND ERROR:", socketError, file=sys.stderr)
			print("Exiting.", file=sys.stderr)
			myFT.closeSockets()
			sys.exit(2)
		
		# Otherwise, update totalCharsSent in preparation for next iteration.
		totalCharsSent += chunkLen
	
#######################################################################################################
# Function Name:  	sendMessage
# Description:		Sends the length of the message passed in to the server followed by 
# 			the message itself.
# Receives:		A socket connected to the server, the message to be sent, and an FTInfo
#			object to be used for closing open sockets if error occurs before exiting.
#			Note that, although messagingSocket represents one of the sockets
#			stored in FTInfo, this parameter clarifies which of those sockets
#			to use for sending this message.
# Returns: 		Nothing
# Pre-Conditions:	messagingSocket has successfully connected to the server, message is
#			a non-empty string, and myFT represents an instantiated FTInfo object.
# Post-Conditions: 	If there is no error in sending the message, all bytes of the message
#			have succesfully been sent out to the transport layer.
# ** CITATION **:	Concept of sending message length before sending message itself using
# 			function that verifies sending of full strings learned from combination
# 			of the following:
# 			- CS 344-400 Fall 2019 Lectures.
# 			- My C implementation of the Block 4 program in that same course.
#			Adapted from similar function that I implemented for CS 372 Program 1.
#######################################################################################################

def sendMessage(messagingSocket, message, myFT):	
	# Convert message length to a string and append terminating "@" character to signal end of message
	# length string. Then send messageLenStr followed by message itself to server.
	messageLenStr = str(len(message)) + "@"
	sendCompleteString(messagingSocket, messageLenStr, myFT)
	sendCompleteString(messagingSocket, message, myFT)

#######################################################################################################
# Function Name:  	recvMessage
# Description:		Receives and returns a message from the server, ensuring that all bytes of full
#  			message are read from transport layer.
# Receives:		A socket connected to the server and an FTInfo object to be used
#			for closing open sockets if error occurs before exiting.
#			Note that, although messagingSocket represents one of the sockets
#			stored in FTInfo, this parameter clarifies which of those sockets
#			to use for sending this message.
# Returns: 		The message received from the server, decoded as a string.
# Pre-Conditions: 	The messagingSocket is connected to the server and myFT represents
#			an instantiated FTInfo object.
# Post-Conditions: 	Unless an error occurs while calling recv and the connection is closed,
# 			the full message has been read from the transport layer and returned.
# ** CITATIONS **:	- recv() calls based off example recv() call in Lecture 15, Slide 9.
#			- Use of bytes.decode() learned from Python 3 documentation:
#			  https://docs.python.org/3.6/library/stdtypes.html#bytes.decode
#			- Concept of looping until full message is received based on information
#			  provided in the following article: 
#			  McMillan, G. Socket Programming HOWTO. Accessed 02/09/2020 from:
#			  https://docs.python.org/3.6/howto/sockets.html
#			- Function adapted from my C implementation of functions with similar
# 			  purpose in the Block 4 Project in CS 344-400 Fall 2019.
#			- Function adapted from my Python implementation of similar function in
#			  CS-372 Programming Assignment 1.
#######################################################################################################

def recvMessage(messagingSocket, myFT):
	# Receive message length. Declare string to hold length of message.
	messageLenStr = ""

	# Receive length 1 byte at a time, stopping after '@' character is received.
	while not messageLenStr.endswith("@"):
		# Read up to 1 character from the socket and check for error.
		try:
			messageByte = messagingSocket.recv(1)
			byteAsStr = messageByte.decode()
			
			# If 0 bytes were received, report error to user and exit.
			if len(byteAsStr) == 0:
				print("RECV ERROR: Connection closed by server.", file=sys.stderr)
				myFT.closeSockets()
				sys.exit(2)
			
			# Otherwise, add byteAsStr to end of messageLenStr.
			messageLenStr += byteAsStr
		
		# If an OSError was raised during the recv call, catch and report it before exiting.
		except OSError as socketError:
			print("RECV ERROR:", socketError, file=sys.stderr)
			myFT.closeSockets()
			sys.exit(2)
	
	# Strip the terminating '@' character off of the message and convert it to an int. */
	messageLenStr = messageLenStr.strip("@")
	messageLenReported = int(messageLenStr)
	
	# Allocate an empty string into which to store message. */
	message = ""
	
	# Loop until full messageLenReported has been read or error occurs. */
	charsRemaining = messageLenReported
	
	while charsRemaining > 0:
		# Read up to charsRemaining characters from the socket and check for recv error. */
		try:
			chunkAsBytes = messagingSocket.recv(charsRemaining)
			chunkAsStr = chunkAsBytes.decode()
			
			# If 0 bytes were received, report error to user and exit.
			if len(chunkAsStr) == 0:
				print("RECV ERROR: Connection closed by server.", file=sys.stderr)
				myFT.closeSockets()
				sys.exit(2)
			
			# Update the number of chars remaining for the next iteration, and concatenate
			# messageChunkStr to end of message.
			charsRemaining -= len(chunkAsStr)
			message += chunkAsStr
		
		# If an OSError was raised during the recv call, catch and report it before exiting.
		except OSError as socketError:
			print("RECV ERROR:", socketError, file=sys.stderr)
			myFT.closeSockets()
			sys.exit(2)
	
	# Return message to calling function now that full message has been received.
	return message

#######################################################################################################
# Function Name:  	recvBytes
# Description:		Receives and returns a chunk of bytes from the server, ensuring that complete
#			chunk of data sent by the server with length reported by the server is
#			received before returning it.
# Receives:		A socket connected to the server and an FTInfo object to be used
#			for closing open sockets if error occurs before exiting.
#			Note that, although messagingSocket represents one of the sockets
#			stored in FTInfo, this parameter clarifies which of those sockets
#			to use for sending this message.
# Returns: 		A chunk of bytes received from the server as a bytes object.
# Pre-Conditions: 	The messagingSocket is connected to the server and myFT represents
#			an instantiated FTInfo object.
# Post-Conditions: 	Unless an error occurs while calling recv and the connection is closed,
# 			the full chunk of bytes with length reported by the server has
#			been read from the transport layer and returned.
# ** CITATIONS **:	- recv() calls based off example recv() call in Lecture 15, Slide 9.
#			- Use of bytes.decode() learned from Python 3 documentation:
#			  https://docs.python.org/3.6/library/stdtypes.html#bytes.decode
#			- Concept of looping until full message is received based on information
#			  provided in the following article: 
#			  McMillan, G. Socket Programming HOWTO. Accessed 02/09/2020 from:
#			  https://docs.python.org/3.6/howto/sockets.html
#			- Function adapted from my C implementation of functions with similar
# 			  purpose in the Block 4 Project in CS 344-400 Fall 2019.
#			- Function adapted from my Python implementation of similar function in
#			  CS-372 Programming Assignment 1.
#######################################################################################################

def recvBytes(messagingSocket, myFT):
	# Receive message length. Declare string to hold length of message.
	messageLenStr = ""

	# Receive length 1 byte at a time, stopping after '@' character is received.
	while not messageLenStr.endswith("@"):
		# Read up to 1 character from the socket and check for error.
		try:
			messageByte = messagingSocket.recv(1)
			byteAsStr = messageByte.decode()
			
			# If 0 bytes were received, report error to user and exit.
			if len(byteAsStr) == 0:
				print("RECV ERROR: Connection closed by server.", file=sys.stderr)
				myFT.closeSockets()
				sys.exit(2)
			
			# Otherwise, add byteAsStr to end of messageLenStr.
			messageLenStr += byteAsStr
		
		# If an OSError was raised during the recv call, catch and report it before exiting.
		except OSError as socketError:
			print("RECV ERROR:", socketError, file=sys.stderr)
			myFT.closeSockets()
			sys.exit(2)
	
	# Strip the terminating '@' character off of the message and convert it to an int. */
	messageLenStr = messageLenStr.strip("@")
	messageLenReported = int(messageLenStr)
	
	# Allocate an empty bytes object into which to store message. */
	data = b""
	
	# Loop until full messageLenReported has been read or error occurs. */
	bytesRemaining = messageLenReported
	
	while bytesRemaining > 0:
		# Read up to charsRemaining characters from the socket and check for recv error. */
		try:
			chunkOfBytes = messagingSocket.recv(bytesRemaining)
			
			# If 0 bytes were received, report error to user and exit.
			if len(chunkOfBytes) == 0:
				print("RECV ERROR: Connection closed by server.", file=sys.stderr)
				myFT.closeSockets()
				sys.exit(2)
			
			# Update the number of bytes remaining for the next iteration, and concatenate
			# chunkOfBytes to end of data.
			bytesRemaining -= len(chunkOfBytes)
			data += chunkOfBytes
		
		# If an OSError was raised during the recv call, catch and report it before exiting.
		except OSError as socketError:
			print("RECV ERROR:", socketError, file=sys.stderr)
			myFT.closeSockets()
			sys.exit(2)
	
	# Return data to calling function now that full message has been received.
	return data
