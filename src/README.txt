#######################################################################################################
# Programmer Name: 	Alexander Densmore
# Program Names: 	ftclient / ftserver
# Program Descriptions:	Implementation of a client-server file transfer protocol accross two programs,
#			ftclient and ftserver. ftserver listens on port specified on command line
#			for connections. ftclient allows user to specify the server and port to which
#			to connect, the desired command, the filename (if command is -g for get file),
#			and the data port on which to receive data. 
# *** EXTRA CREDIT ***	In addition to the required commands of -l (list all files in current directory)
#			and -g (get file with filename), I have implemented an option -ltxt (list all files
#			in current directory with .txt extension). Upon receiving -ltxt command,
#			server filters current directory listing for only files with .txt extension,
#			either sending list of such files to client or reporting that there
#			are no files with .txt extension in the current directory.
# Course Name: 		CS 372-400: Introduction to Computer Networks
# Last Modified:	03/09/2020
######################################################################################################

*** FTServer Instructions ***

To Compile: On the command line, type: make
To Run: On the command line, type: ftserver SERVER_PORT
To Remove Executable: On the command line, type: make clean
Notes:		Once the process begins execution upon inputting command-line arguments, no further input to the process
		is accepted. If the SERVER_PORT is invalid or there is an error binding it for listening,
		an error will be printed to the screen, and the process will exit. Otherwise,
		a message will be printed to indicate that the process is listening on the
		specified SERVER_PORT, and the process will loop between listening for connections
		and accepting / responding to connections until a SIGINT is received by the user typing ctrl + c.
		While the server is running, informative messages will be printed every time a new connection is accepted,
		including what was requested as well as a message indicating the request
		is being fulfilled and/or any pertinent error messages. The process then reports when it is once again
		awaiting a new connection.

*** FTClient Instructions ***

To Run: On the command line, type: python3 ftclient.py SERVER_HOST SERVER_PORT COMMAND [FILE_NAME] DATA_PORT
To Remove Pycache: On the command line, type: make cleanPycache
Notes:		SERVER_HOST may be either a flip nickname ("flip1", "flip2", or "flip3") or the full URL / IPv4 address
		of the desired server with which to connect.

		Available commands for the command argument are listed below:
		
		SYNTAX: DESCRIPTION:
		-g      Get file with [filename]
		-l      List all files in the current directory
		-ltxt   List only files with .txt extension

		(These commands and descriptions can also be viewed by typing the following on the command line:
		python3 chatclient.py -h). Note that the filename is required with the -g command but should be
		omitted after other commands.

		Once the process begins execution upon inputting command-line arguments, no further input to the process
		is accepted. The process first validates all command-line arguments, ensuring that the port numbers are 
		in a valid format, there are the correct number of arguments (4 if no filename, 5 if filename),
		the command provided is valid, and a filename is included (if command is -g) or omitted
		(if command is -l or -ltxt). In addition, before attempting to connect to the server,
		the client tries binding the listening socket to the data port to listen
		for an incoming data connection from the server, exiting without trying to connect
		to the server on failure and reporting an error. (The server also validates all
		arguments received from the client for security reasons, but client-end validation ensures that a 
		connection is not needlessly made just to send invalid commands).

		The client process prints an informative message when it is about to receive
		data from the server. Upon receiving a directory listing,
		the client prints that listing to the console. Upon receiving file contents,
		the client writes the data received to a file, ensuring it has a unique name
		if the name of the file requested matches the name of a file
		already in the current directory. Once the file contents have finished being received,
		the client prints an informative message to the console with the name of the file containing the results.
		The client also prints any error messages received from the server. The client exits automatically
		upon command fulfillment or first error encountered.
