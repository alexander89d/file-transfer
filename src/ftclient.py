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
# File Name:		ftclient.py
# File Description: 	File containing top-level code for reading in command-line arguments,
#			printing help message if command "-h" is entered, otherwise printing error
#			message if invalid number of command-line arguments are entered, and then
#			instantiating FTInfo object to faciliate communications between ftclient
#			and ftserver.
# Course Name: 		CS 372-400: Introduction to Computer Networks
# Last Modified:	03/09/2020
######################################################################################################

import sys
import FTInfo

# If "-h" was entered on command line, display usage instructions + list of available commands to stdout
# and exit with exit code 0 to indicate no error.
if len(sys.argv) == 2 and sys.argv[1] == "-h":
	print(FTInfo.USAGE_MESSAGE)
	print("Accepted Commands:\n")
	print(FTInfo.ACCEPTED_COMMANDS)
	sys.exit(0)

# If an invalid number of command-line arguments were entered, print usage message and exit.
if len(sys.argv) < FTInfo.MIN_ARGS or len(sys.argv) > FTInfo.MAX_ARGS:
	print(FTInfo.USAGE_MESSAGE, file=sys.stderr)
	print(FTInfo.COMMAND_HELP_MESSAGE, file=sys.stderr)
	sys.exit(1)

# Otherwise, instantiate FTInfo, passing it the command-line arguments for initialization.
myFT = FTInfo.FTInfo(sys.argv)

# Initiate contact with the server.
myFT.initiateContact()

# Send command (and filename, if applicable) to the server.
myFT.makeRequest()

# Receive response from the server.
myFT.receiveData()
