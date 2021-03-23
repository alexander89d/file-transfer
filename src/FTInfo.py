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
# File Name:		FTInfo.py
# File Description: 	File containing class definition of FTInfo, which validates and stores
#			the connection-related information used throughout execution of the program
#			and contains methods related to that connection.
# Course Name: 		CS 372-400: Introduction to Computer Networks
# Last Modified:	03/09/2020
######################################################################################################

import select
import socket
import sys
import clientServerMessaging
import CommandList

# Global variables to be treated as constants within FTInfo class.

# Minimum and maximum number of arguments allowed (inclusive of program name on command line).
MIN_ARGS = 5
MAX_ARGS = 6

# Usage message.
USAGE_MESSAGE = "USAGE: python3 ftclient.py SERVER_HOST SERVER_PORT COMMAND [FILE_NAME] DATA_PORT"
COMMAND_HELP_MESSAGE = "FOR COMMAND HELP: python3 ftclient.py -h"

# Macros for accepted commands.
GET_FILE = "-g"
LIST_FILES = "-l"
LIST_TXT_FILES = "-ltxt"

# List of commands accepted on command line with descriptions.
ACCEPTED_COMMANDS = CommandList.CommandList([
CommandList.Command(GET_FILE, "Get file with [filename]"), 
CommandList.Command(LIST_FILES, "List all files in the current directory"),
CommandList.Command(LIST_TXT_FILES, "List only files with .txt extension")
])

# Beginning of success message received from server over control socket
# once all bytes of requested data have been successfully sent.
SUCCESS_PREFIX = "SUCCESS!"


#######################################################################################################
# Class Name:		FTInfo
# Class Description:	Class that instantiates object with data members storing information relevant
#			to the requested interaction with ftserver and methods that help facilitate
#			client-server interaction.
# Data Members:		serverNickname (server name passed in on command line)
#			serverHost (full server address)
#			serverPort (port number at which to contact server, represented as int)
#			command (the command to be executed by ftserver; must be in ACCEPTED_COMMANDS)
#			filename (the name of the file to be retrieved from server or None)
#			dataPort (the port on which to listen for data connection from server, represented as int)
#			controlSocket (socket used for control connection to server)
#			listeningSocket (socket on which to listen for connection from server)
#			dataSocket (socket used for data connection to server)
#			messagingPoll (poll object registered to poll for when control or data sockets ready to recv)
# Member Functions:	(see below for definitions and descriptions)
#######################################################################################################

class FTInfo:
	
	#######################################################################################################
	# Function Name:	_getFullHostname
	# Description:		Internal function called by __init__. 
	#			If "flip1", "flip2", or "flip3" is provided as SERVER_HOST, expands to full
	#			hostname. Otherwise, assumes hostname received is full hostname and returns as
	#			is.
	# Receives: 		Self-reference and string representing SERVER_HOST, either abbreviated as
	#			"flip1", "flip2", or "flip3", or fully written out.
	# Returns: 		The full hostname.
	# Pre-Conditions:	SERVER_HOST is non-null string and either one of the abbreviations mentioned
	#			above or the full host name.
	# Post-Conditions: 	Full hostname is returned and assigned to self.serverHost.
	######################################################################################################
	
	def _getFullHostname(self, hostname):
		# If simply "flip1," "flip2," or "flip3" was entered as the hostname, expand it to full hostname.
		if hostname == "flip1" or hostname == "flip2" or hostname == "flip3":
			return hostname + ".engr.oregonstate.edu"
		
		# Otherwise, return the hostname exactly as entered.
		else:
			return hostname
	
	#######################################################################################################
	# Function Name:	_validatePornum
	# Description:		Determines whether string received is a valid non-negative integer.
	# Receives: 		A self-reference and a string representing a port number.
	# Returns: 		An integer representation of the port number if portnum string received
	#			represents a non-negative ingeger. Otherwise, returns None.
	# Pre-Conditions:	Portnum is a non-null string.
	# Post-Conditions: 	Either portnum has been converted to an integer and returned, or None
	#			has been returned to indicate error.
	######################################################################################################
	
	def _validatePortnum(self, portnum):
		# Ensure the entered port is a valid non-negative integer.
		# Call to isdigit ensures every character is a digit.
		if not portnum.isdigit():
			return None
		
		# If portnum is valid integer, return integer representing it.
		else:
			return int(portnum)
	
	#######################################################################################################
	# Function Name:	_handleInitErrs
	# Description:		Internal function called by __init__ to print any error passed in by calling
	#			function.
	# Receives: 		Self-reference and list of errors as strings.
	# Returns: 		Nothing (exits if error)
	# Pre-Conditions:	errList is either empty list or list of non-null strings.
	# Post-Conditions: 	Control either returns to calling function or process exits with error code 1.
	######################################################################################################
	
	def _handleInitErrs(self, errList):
		# If there are any errors, print them.
		if len(errList) > 0:
			print(USAGE_MESSAGE, file=sys.stderr)
			print("The command-line arguments entered contained the following error(s):", file=sys.stderr)
			for err in errList:
				print("\t", err, file=sys.stderr)
			
			# Exit with error code 1 to indicate command-line argument errors.
			sys.exit(1)
	
	#######################################################################################################
	# Function Name:	closeSockets
	# Description:		Closes the controlSocket and, if open, the listeningSocket. Server is
	#			responsible for closing dataSocket (client only closes dataSocket if it
	#			unintentionally accepts a connection from source other than the server).
	# Receives: 		A self-reference.
	# Returns: 		nothing
	# Pre-Conditions:	The controlSocket has been initialized as a socket object
	#			(even if it is not connected to the server yet). The listeningSocket
	#			is either a socket object or None (e.g. if it has already been closed).
	# Post-Conditions: 	The controlSocket and listeningSocket (if not None) have been closed.
	######################################################################################################
	
	def closeSockets(self):
		# Close control socket.
		self.controlSocket.close()
		
		# If listening socket is not None, close it.
		if self.listeningSocket != None:
			self.listeningSocket.close()
	
	#######################################################################################################
	# Function Name:	__init__
	# Description:		Constructs new FTInfo object based on parameters received.
	# Receives: 		Self-reference and argv, a list of strings representing command-line arguments
	#			passed into code which called this function.
	# Returns: 		An instantiated FTInfo object.
	# Pre-Conditions:	argv is a valid list of non-null strings which follows usage instructions
	#			detailed in USAGE_MESSAGE declared above.
	# Post-Conditions: 	All data members have been initialized. Sockets have been established
	#			and are ready to connect to server or listen for connection from server.
	######################################################################################################
	
	def __init__(self, argv):
		# List of error messages to be printed in case of errors before exiting.
		initErrList = []
		
		# Initialize serverNickname to that passed in on the command line.
		self.serverNickname = argv[1]
		
		# Initialize serverHost, expanding serverNickname if it is "flip1," "flip2," or "flip3." 
		self.serverHost = self._getFullHostname(self.serverNickname)
		
		# Initialize serverPort, adding error message if invalid.
		self.serverPort = self._validatePortnum(argv[2])
		if self.serverPort == None:
			initErrList.append("SERVER_PORT invalid. You entered: " + argv[2])
		
		# Initialize command, adding error message if invalid.
		self.command = argv[3]
		if not ACCEPTED_COMMANDS.validate(argv[3]):
			initErrList.append("COMMAND invalid. You entered: " + argv[3])
			initErrList.append("\t" + COMMAND_HELP_MESSAGE)
		
		# If command GET_FILE was entered, then argv[4] is the filename and argv[5] is the data port.
		# Set these accordingly.
		if self.command == GET_FILE:
			# If there are not a total of 6 command-line arguments,
			# assume user did not enter filename and only provided portnum
			# after command. Add error to errList and set dataPortIn to argv[4].
			if len(argv) != MAX_ARGS:
				initErrList.append("COMMAND ERROR: FILENAME required after -g command before DATA_PORT.")
				self.filename = None
				dataPortIn = argv[4]
			
			# Otherwise, since correct number of arguments for -g command were passed in,
			# assign values accordingly.
			else:
				self.filename = argv[4]
				dataPortIn = argv[5]
		
		# Otherwise, if the maximum number of arguments were entered,
		# report error since -l and -ltxt should be followed only by dataPort,
		# and set dataPortIn to the last argument received.
		elif len(argv) == MAX_ARGS:
			tooManyArgs = "COMMAND ERROR: Only DATA_PORT should appear after \""
			tooManyArgs += self.command + "\" command"
			initErrList.append(tooManyArgs)
			dataPortIn = argv[5]
		
		# Otherwise, the command is valid and argv[4] is the data port. Set accordingly.
		else:
			self.filename = None
			dataPortIn = argv[4]
		
		# Initialize self.dataPort, adding error message if invalid.
		self.dataPort = self._validatePortnum(dataPortIn)
		if self.dataPort == None:
			initErrList.append("DATA_PORT invalid. You entered: " + dataPortIn)
		
		# Handle any error messages.
		self._handleInitErrs(initErrList)
		
		# Establish TCP socket which will be connected to server as control socket.
		self.controlSocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM) 
		
		# Establish TCP socket which will listen for incoming connection from server
		# (incoming connection will be data socket).
		self.listeningSocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		
		# Bind listeningSocket to dataPort, reporting error and exiting if one occurs.
		try:
			self.listeningSocket.bind(('', self.dataPort))
		except OSError as socketErr:
			print("LISTENING SOCKET DATA PORT BIND ERROR:", socketErr, file=sys.stderr)
			self.closeSockets()
			sys.exit(2)
		
		# Enable listeningSocket to listen for incoming connections, only allowing
		# 1 queued connection since only 1 incoming connection expected (the one received
		# from the server). If error occurs, report it and exit.
		try:
			self.listeningSocket.listen(1)
		except OSError as socketErr:
			print("LISTEN ERROR:", socketErr, file=sys.stderr)
			self.closeSockets()
			sys.exit(2)
		
		# Initialize dataSocket to None as a placeholder for when
		# a data connection is accepted from the server later.
		self.dataSocket = None
		
		# Initialize messagingPoll to None as a placeholder for when poll is registered
		# for polling later on for when controlSocket or dataSocket have data ready to receive.
		self.messagingPoll = None
	
	#######################################################################################################
	# Function Name:	initiateContact
	# Description:		Initiates a control connection with server at serverHost:serverPort,
	#			sending server port number on which dataPort has been established, receiving
	#			server response, and validating that expected response was received.
	# Receives: 		A self-reference.
	# Returns: 		Nothing
	# Pre-Conditions:	The server at serverHost:serverPort exists and is listening for connections.
	# Post-Conditions: 	If no errors occur with sending or receiving and the expected server response
	#			is received, a TCP control connection is established between the client and server.
	######################################################################################################
	
	def initiateContact(self):
		# Connect to server at serverHost:serverPort, reporting error and exiting if one occurs.
		try:
			self.controlSocket.connect((self.serverHost, self.serverPort))
		
		except OSError as socketErr:
			print("ERROR CONNECTING TO SERVER:", socketErr, file=sys.stderr)
			self.closeSockets()
			sys.exit(2)
		
		# Send server dataPort via controlSocket
		dataPortMessage = "DATA_PORT: " + str(self.dataPort)
		clientServerMessaging.sendMessage(self.controlSocket, dataPortMessage, self)

		# Receive initial response from server, validating that it is
		# expected response of "FTSERVER CONNECTION ESTABLISHED"
		serverResponse = clientServerMessaging.recvMessage(self.controlSocket, self)
		expectedResponse = "FTSERVER CONNECTION ESTABLISHED"
		if serverResponse != expectedResponse:
			print("SERVER VALIDATION ERROR:", file=sys.stderr)
			print("Expected response from server was:", expectedResponse, file=sys.stderr)
			print("Response received was:", serverResponse, file=sys.stderr)
			self.closeSockets()
			sys.exit(2)
	
	#######################################################################################################
	# Function Name:	makeRequest
	# Description:		Sends the command and filename (if applicable) to the server.
	# Receives: 		A self-reference.
	# Returns: 		nothing
	# Pre-Conditions:	The controlSocket has been successfully connected to the server,
	#			and the server has already been sent the desired dataPort number.
	# Post-Conditions: 	The request has been sent to the server on the control connection.
	######################################################################################################
	
	def makeRequest(self):
		# Initialize serverRequest to being command.
		serverRequest = self.command
		
		# If filename is not None, append a space and the filename to serverRequest
		if self.filename != None:
			serverRequest += " " + self.filename
		
		# Send the request to the server.
		clientServerMessaging.sendMessage(self.controlSocket, serverRequest, self)
	
	#######################################################################################################
	# Function Name:	_conectionReadyToAccept
	# Description:		Internal function that polls controlSocket and listeningSocket
	#			until at least one of them has data to receive and then indicates whether
	#			listeningSocket is the only one ready.
	# Receives: 		A self-reference.
	# Returns: 		True if listening socket is the only one with data ready to receive,
	#			indicating connection can be accepted from it. Otherwise,
	#			returns False to indicate that error message from server
	#			should be checked for and printed on controlSOcket.
	# Pre-Conditions:	The controlSocket has been successfully connected to the server
	#			and the listeningSocket has been boud to the desired port and activated
	#			for listening.
	# Post-Conditions: 	If True is returned, then dataConnection is ready to be accepted and validated.
	######################################################################################################
	
	def _connectionReadyToAccept(self):
		# Define a new polling object, registering POLLIN listeners for listeningSocket and controlSocket
		recvPoll = select.poll()
		recvPoll.register(self.controlSocket.fileno(), select.POLLIN)
		recvPoll.register(self.listeningSocket.fileno(), select.POLLIN)

		# Wait for poll results. Do not specify a timeout so that
		# system call blocks until results are received.
		pollResults = recvPoll.poll()
		
		# If there is more than one result or the only result's fileno does not match
		# that of the listeningSocket, there must be data to read on the controlSocket,
		# indicating that the server sent an error. Return False to indicate that
		# a new connection should not be accepted and that the controlSOcket should be
		# read from instead.
		if len(pollResults) != 1 or pollResults[0][0] != self.listeningSocket.fileno():
			return False
		
		# Otherwise, return True to indicate that a connection for the dataPort
		# can be accepted.
		else:
			return True
	
	#######################################################################################################
	# Function Name:	_validateDataConnection
	# Description:		Internal function which accepts an incoming connection on the listening socket,
	#			validates that dataSocket is connected to the same remote IPv4 address as
	#			controlSocket, receives and validates initial dataSocket
	#			message from server, and sends acknowledgement message to server.
	# Receives: 		A self-reference.
	# Returns: 		nothing
	# Pre-Conditions:	The control socket has previously been connected to the server, and the
	#			listening socket has been bound to the desired port and activated
	#			for listening.
	# Post-Conditions: 	Unless error occurs or invalid connection detected (causing process to exit),
	#			dataSocket is connected to the same remote IPv4 address
	#			as controlSocket, listeningSocket has been closed now that it is no longer
	#			needed, expected initial message has been received from server, and
	#			reply has been sent to server. dataSocket is now ready to receive
	#			the data requested from the server.
	######################################################################################################
	
	def _validateDataConnection(self):
		# Accept data connection from listening socket, reporting error if one occurs.
		try:
			self.dataSocket, dataSocketAddr = self.listeningSocket.accept()
		except OSError as socketErr:
			print("SOCKET ACCEPTANCE ERROR:", socketErr, file=sys.stderr)
			self.closeSockets()
			sys.exit(2)
		
		# Close the listening socket now that it is no longer needed and
		# set its value to None to indicate it is no longer in use.
		self.listeningSocket.close()
		self.listeningSocket = None
		
		# Get the remote IP address to which each socket is connected (at the first index
		# in the 2-tuple representing IPv4 address).
		dataSocketIP = dataSocketAddr[0]
		controlSocketIP = (self.controlSocket.getpeername())[0]
		
		# If the dataSocket and listeningSocket are not connected to the same remote address, report error,
		# close data socket since invalid connection, and exit.
		if controlSocketIP != dataSocketIP:
			print("INVALID DATA CONNECTION: Connection expected from", self.clientNickname, file=sys.stderr)
			print("at address", controlSocketIP, file=sys.stderr)
			print("Connection received instead from address", dataSocketIP, file=sys.stderr)
			self.dataSocket.close()
			self.closeSockets()
			sys.exit(2)
		
		# Now, receive initial message from server on data socket, ensuring it is expected message.
		serverMessage = clientServerMessaging.recvMessage(self.dataSocket, self)
		expectedMessage = "FTSERVER DATA CONNECTION INITIALIZATION"

		# If message received is not what was expected, report error, close invalid data socket,
		# and exit.
		if serverMessage != expectedMessage:
			print("INVALID DATA CONNECTION", file=sys.stderr)
			print("Expected message on data port from server was:", expectedMessage, file=sys.stderr)
			print("Message received was:", serverMessage, file=sys.stderr)
			self.dataSocket.close()
			self.closeSockets()
			sys.exit(2)
		
		# Send response to server over data connection.
		dataConnectionAckMessage = "FTSERVER DATA CONNECTION ACCEPTED"
		clientServerMessaging.sendMessage(self.dataSocket, dataConnectionAckMessage, self)
	
	#######################################################################################################
	# Function Name:	_registerMessagingPoll()
	# Description:		Instantiates a polling object to poll for when data is ready to receive
	#			on controlSocket and/or dataSocket. Registers controlSOcket
	#			and dataSocket with polling object. Stores this polling object
	#			in self.messagingPoll
	# Receives: 		A self-reference.
	# Returns: 		nothing
	# Pre-Conditions:	The controlSocket and dataSocket have been successfully connected
	#			to the server.
	# Post-Conditions: 	self.messagingPoll has been instantiated and registered to poll for data
	#			ready to receive on controlSocket or dataSocket.
	######################################################################################################
	
	def _registerMessagingPoll(self):
		# Define a new polling object and store it in self.messagingPoll,
		# registering POLLIN listeners for listeningSocket and controlSocket
		self.messagingPoll = select.poll()
		self.messagingPoll.register(self.controlSocket.fileno(), select.POLLIN)
		self.messagingPoll.register(self.dataSocket.fileno(), select.POLLIN)
	
	#######################################################################################################
	# Function Name:	_pollMessagingSockets
	# Description:		Polls the messagingPoll object until controlSocket and/or dataSocket has
	#			data ready to receive. Once poll call returns, receives data from whichever
	#			socket(s) have data available.
	# Receives: 		A self-reference and a flag indicating whether to decode dataMessage to string
	#			(if set) or leave dataMessage as bytes object (if cleared).
	# Returns: 		A 2-tuple containing (controlMessage, dataMessage). Control message
	#			(if present) is already decoded to string. Data message is decoded to
	#			string if decodeDataMessage flag is set; otherwise, is left encoded as bytes.
	# Pre-Conditions:	messagingPoll has been initialized and registered for polling controlSocket
	#			and dataSocket for data available to receive. Data socket and control socket
	#			have been connected to server.
	# Post-Conditions: 	Messages received are placed in 2-tuple returned, with None in an index
	#			indicating that a message was not available on that socket.
	######################################################################################################
	
	def _pollMessagingSockets(self, decodeDataMessage):
		# Initialize controlMessage and dataMessage to None.
		controlMessage = None
		dataMessage = None
		
		# Poll messagingPoll and process results. Call will block until at least 1
		# socket has data ready (or error occurs with a socket).
		pollResults = self.messagingPoll.poll()
		
		# Loop through results list of 2-tuples containing file descriptors and events.
		for fd, event in pollResults:
			# If fd represents the control socket, receive controlMessage.
			if fd == self.controlSocket.fileno():
				controlMessage = clientServerMessaging.recvMessage(self.controlSocket, self)
			
			# Otherwise, fd represents data socket. If decodeDataMessage flag is set,
			# receive dataMessage as string.
			elif decodeDataMessage == True:
				dataMessage = clientServerMessaging.recvMessage(self.dataSocket, self)

			# Otherwise, receive dataMessage as bytes since decodeDataMessage flag not set.
			else:
				dataMessage = clientServerMessaging.recvBytes(self.dataSocket, self)
			
		# Return controlMessage and dataMessage in 2-tuple to calling function.
		return (controlMessage, dataMessage)
	
	#######################################################################################################
	# Function Name:	_handleFinalControlMessage
	# Description:		Processes the final control message received from the control socket.
	#			If it is a success message, parses it for the total number of bytes read and
	#			returns bytes read (as int) to calling function. Otherwise, since it is an error
	#			message, prints it before closing sockets and exiting.
	# Receives: 		A self-reference and a control message (as a string).
	# Returns: 		The total number of bytes of requested data sent by the server (as reported
	#			in success message) if control message is a success message. Otherwise,
	#			prints error message and exits.
	# Pre-Conditions:	controlMessage is a non-null string representing the final message received over the
	#			control socket from the server (once the server establishes data connection,
	#			only one more message total is sent over the control connection).
	# Post-Conditions: 	The total number of bytes reported in data in the success message is 
	#			extracted and returned (if the server reports success), or the error message
	#			sent by the server is printed before process exits (if the server reports
	#			error).
	######################################################################################################
	
	def _handleFinalControlMessage(self, controlMessage):
		# Convert controlMessage to a list of chunks of consecutive non-whitespace characters.
		controlChunks = controlMessage.split()

		# If there are at least two chunks in controlChunks and the first is the successPrefix,
		# return the number of bytes of data sent to the calling function.
		if len(controlChunks) >=2 and controlChunks[0] == SUCCESS_PREFIX:
			# controlChunks[1] is the total number of bytes of data sent by the server.
			# Conver it to an int and return it.
			return int(controlChunks[1])
		
		# Otherwise, an error message was received. Print the error message, close sockets, and exit.
		else:
			print(self.serverNickname + ":" + str(self.serverPort), "says:", controlMessage, file=sys.stderr)
			self.closeSockets()
			sys.exit(2)
	
	#######################################################################################################
	# Function Name:	_splitFilename
	# Description:		Splits a filename into a prefix (everything before the extension) and extension.
	# Receives: 		A self-reference.
	# Returns: 		A 2-tuple containing the prefix and the extension. If the file has no extension
	#			(i.e. there is no period in the filename), an empty string is returned
	#			for the extension.
	# Pre-Conditions:	A GET_FILE request has been sent to the server, and a filename was entered
	#			after the GET_FILE command on the command line.
	# Post-Conditions: 	The returned 2-tuple contains a filename having been split into the prefix
	#			and extension, with the extension being an empty string if there is no extension.
	######################################################################################################
	
	def _splitFilename(self):
		# If the filename contains no periods, assume it does not have an extension.
		# Return 2-tuple with filename as the prefix and empty string as the extension.
		if "." not in self.filename:
			return (self.filename, "")
		
		# Otherwise, split the filename every place there is a period.
		filenameChunks = self.filename.split(".")

		# Initialize the prefix to an empty string, and iterate through all but the last
		# index of filenameChunks, adding the chunks to each other with periods between them
		# (if there are multiple chunks before the last chunk).
		prefix = ""
		for index in range(len(filenameChunks)-1):
			# Append this chunk to the prefix.
			prefix += filenameChunks[index]
			
			# If this is not the last chunk of the prefix, add a period to the prefix
			# (where a period initially was between this chunk and the next chunk).
			if index < len(filenameChunks) - 2:
				prefix += "."
		
		# Now that the prefix has been extracted from the original filename,
		# extract the extension (the last chunk of filenameChunks with a period before it).
		extension = "." + filenameChunks[-1]
		
		# Return prefix and extension to calling function.
		return (prefix, extension)
	
	#######################################################################################################
	# Function Name:	_openOutputFile
	# Description:		Opens an output file in binary mode into which to write data received from
	#			server in response to GET_FILE request. Ensures that the output file has a
	#			unique name so as to not overwrite an existing file with the same name.
	# Receives: 		A self-reference.
	# Returns: 		A 2-tuple containing the file object of the newly opened output file and the
	#			name of that file.
	# Pre-Conditions:	self.filename is a non-null string.
	# Post-Conditions: 	A file with filename (or filename with an underscore plus copy number appended)
	#			is created, opened in binary mode, and returned to calling function.
	# ** CITATION **	Idea for opening file in binary mode so that byte objects can be directly
	#			written into it to avoid issues with UTF-8 characters that are multiple
	#			bytes (such as smart quotes) learned from CS 372-400 Winter 2020 Piazza
	#			Post @192_f3 by Michael Estorer.
	######################################################################################################
	
	def _openOutputFile(self):
		# Initialize outputFile to None as a placeholder and outputFilename to self.filename.
		outputFile = None
		outputFilename = self.filename
		
		# Attempt to open outputFile in binary mode with exclusive creation flag.
		try:
			outputFile = open(outputFilename, "xb")
		
		# If a FileExistsError is raised due to the file already existing,
		# try new file names consisting of prefix + underscore + copy number + extension
		# (e.g. myfile_1.txt, myfile_2.txt, etc.) until one that works is found.
		except FileExistsError:
			# Initialize copy number to 0, incrementing it at beginning of loop below
			# until a name that does not throw a FileExistsError is found.
			copyNumber = 0

			# Initialize validNameFound to False, setting it to True
			# upon successfully opening a file with a unique name.
			validNameFound = False
			
			# Split filename into prefix (everything before the extension) and the extension
			# (final period of filename and everything that follows it). If there is no extension,
			# extension returned is simply empty string.
			prefix, extension = self._splitFilename()
			
			# Loop until a valid name is found.
			while not validNameFound:
				# Increment the copy number, inserting an underscore and copy number
				# between the prefix and extension.
				copyNumber += 1
				outputFilename = prefix + "_" + str(copyNumber) + extension
				
				# Attempt to open output file with outputFilename in binary mode for exclusive creation,
				# catching FileExistsError if one occurs and setting validNameFound to True
				# otherwise.
				try:
					outputFile = open(outputFilename, "xb")
				
				# If the file with that name already exists,
				# simply continue to the next iteration.
				except FileExistsError:
					continue
				
				# Otherwise, set validNameFound to True.
				else:
					validNameFound = True
		
		# Now that output file with unique name has successfully been opened,
		# return outputFile and outputFilename to calling function.
		return (outputFile, outputFilename)
	
	#######################################################################################################
	# Function Name:	_recvFileFromServer
	# Description:		Internal function which receives the data from the file with specified
	#			filename from the server and writes it to a new file. Ensures new file has
	#			unique name to avoid duplicate name error.
	# Receives: 		A self-reference.
	# Returns: 		nothing
	# Pre-Conditions:	dataSocket and messagingSocket have been connected to the server successfully,
	#			and the server has been sent GET_FILE command and filename over the
	#			control connection.
	# Post-Conditions: 	Unless error occurs receiving data from server (which is reported
	#			and causes the program to exit), the file with filename printed to the console
	#			contains the transferred file.
	######################################################################################################
	
	def _recvFileFromServer(self):
		# Initialize variable dataLength (which will store the total number of bytes of data
		# the server will have sent, as reported by the server's success message) to None. This will serve as
		# placeholder for when value is received from server.
		dataLength = None
		
		# Initialize variable bytesReceived to keep count of total bytes of data
		# actually received from the data socket.
		bytesReceived = 0

		# Get next message(s) sent by server over data connection and/or control connection
		# to check for any errors in finding file to send.
		controlMessage, dataMessage = self._pollMessagingSockets(False)
		
		# If there is a controlMessage, process it, storing its return value (length of data in bytes)
		# for reference below. (If final control message is an error message,
		# process will print it to console before exiting.)
		if controlMessage != None:			
			dataLength = self._handleFinalControlMessage(controlMessage)
		
		# Inform user that file is now being received from server.
		print("Receiving \"" + self.filename + "\" from " + self.serverNickname + ":" + str(self.dataPort))
		
		# Open file and put first set of bytes read in into file (if a dataMessage has already
		# been received). 
		outputFile, outputFilename = self._openOutputFile()
		if dataMessage != None:
			outputFile.write(dataMessage)
			bytesReceived += len(dataMessage)
		
		# Loop until full file is received, continuing as long dataLength = None (the success
		# message has not been received over the control socket with total number of bytes sent)
		# or the number of bytes received is less than dataLength.
		while dataLength == None or bytesReceived < dataLength:
			# Get next message(s) sent by server over data connection and/or control connection.
			controlMessage, dataMessage = self._pollMessagingSockets(False)
			
			# If there is a controlMessage, process it, storing its return value in dataLength
			# (program will print controlMessage and exit if it is not the success message
			# with total length of data sent).
			if controlMessage != None:			
				dataLength = self._handleFinalControlMessage(controlMessage)
			
			# If there is a data message, write it into the outputFile and add its length to
			# bytesReceived.
			if dataMessage != None:
				outputFile.write(dataMessage)
				bytesReceived += len(dataMessage)
		
		# Now that full file has been received, close it, print that transfer is finished
		# and indicate output filename, and exit.
		outputFile.close()
		print("File transfer complete. Results can be found in \"" + outputFilename + "\"")
	
	#######################################################################################################
	# Function Name:	_recvListingFromServer
	# Description:		Internal function which receives and prints a list of all files in the current
	#			directory on which the server is running (if LIST_FILES command was entered)
	#			or a list of all files with .txt extension (if LIST_TXT_FILES command
	#			was entered). If error message is received from server, it is printed to
	#			the console.
	# Receives: 		A self-reference.
	# Returns: 		nothing
	# Pre-Conditions:	The dataSocket and controlSocket have already been connected to the server,
	#			and the command has been sent to the server.
	# Post-Conditions: 	Unless error occurs (in which case error message is printed),
	#			requested listing is received from server and printed to the console.
	######################################################################################################

	def _recvListingFromServer(self):
		# Initialize variable dataLength (which will store the total number of bytes of data
		# the server will have sent, as reported by the server's success message) to None. This will serve as
		# placeholder for when value is received from server.
		dataLength = None
		
		# Initialize variable bytesReceived to keep count of total bytes of data
		# actually received from the data socket.
		bytesReceived = 0

		# Print message informing user about contents about to be received and printed.
		# Determine appropriate message based on whether or not command is LIST_TXT_FILES.
		# If the command is LIST_TXT_FILES, inform user that a list of .txt
		# files in the server's current directory is about to be received.
		if self.command == LIST_TXT_FILES:
			aboutToRecvMessage = "Receiving list of .txt files in directory from "
		
		# Otherwise, since command is LIST_FILES, inform user that a list of all files
		# in the server's current directory is about to be received.
		else:
			aboutToRecvMessage = "Receiving directory structure from "

		# Print message informing user what is about to be received.
		print(aboutToRecvMessage + self.serverNickname + ":" + str(self.dataPort))
		
		# Loop until full listing is received, continuing as long dataLength = None (the success
		# message has not been received over the control socket with total number of bytes sent)
		# or the number of bytes received is less than dataLength.
		while dataLength == None or bytesReceived < dataLength:
			# Get next message(s) sent by server over data connection and/or control connection,
			# passing in decodeDataMessage argument of True since data will be printed to screen
			# upon receipt (and thus should be a decoded string).
			controlMessage, dataMessage = self._pollMessagingSockets(True)
			
			# If there is a controlMessage, process it, storing its return value in dataLength
			# (program will print controlMessage and exit if it is not the success message
			# with total length of data sent).
			if controlMessage != None:			
				dataLength = self._handleFinalControlMessage(controlMessage)
			
			# If there is a data message, print it to the screen and add its length
			# to the total number of bytes received. Set end argument of print() to empty
			# string since listing received from server will already have newline characters
			# after each filename.
			if dataMessage != None:
				print(dataMessage, end="")
				bytesReceived += len(dataMessage)	
	
	#######################################################################################################
	# Function Name:	receiveData
	# Description:		Accepts data connection from server and then calls helper functions to either
	#			receive file from server (if command is GET_FILE) or receive directory
	#			listing from server (if command is LIST_FILES or LIST_TXT_FILES). If the
	#			server sends an error message through the control connection instead of 
	#			opening data connection, prints error message before exiting.
	# Receives: 		A self-reference.
	# Returns: 		nothing
	# Pre-Conditions:	controlSocket has been successfully connected to the server, and
	#			listeningSocket has been bound to the desired DATA_PORT and activated
	#			for listening.
	# Post-Conditions: 	Either the data has been received from the server (stored in a file if
	#			GET_FILE command or printed to the console if LIST_FILES or LIST_TXT_FILES
	#			command), or an appropriate error message hasw been printed to the console.
	######################################################################################################
	
	def receiveData(self):
		# If a new connection is ready to be accepted (and no new data is ready to be read from
		# controlSocket, which would indicate an error message), establish and validate dataConnection.
		if self._connectionReadyToAccept() == True:
			self._validateDataConnection()

		# Otherwise, get and print error message from server, then exit.
		else:
			serverErrorMessage = clientServerMessaging.recvMessage(self.controlSocket, self)
			serverProcessStr = self.serverNickname + ":" + str(self.serverPort)
			print(serverProcessStr, "says:", file=sys.stderr)
			print(serverErrorMessage, file=sys.stderr)
			self.closeSockets()
			sys.exit(2)
			
		# Now that data is ready to receive, instantiate and register
		# the messagingPoll object for use with polling when
		# data is ready to receive from controlSocket or dataSocket.
		self._registerMessagingPoll()
		
		# If command is GET_FILE, call _recvFileFromServer()
		if self.command == GET_FILE:
			self._recvFileFromServer()
		
		# Otherwise, since command is validated to be one of the other two options (-l or -ltxt),
		# call _recvListingFromServer()
		else:
			self._recvListingFromServer()

		# Close sockets and exit.
		self.closeSockets()
