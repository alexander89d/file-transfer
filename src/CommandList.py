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
# File Name:		CommandList.py
# File Description: 	File containing class definitions for Command and CommandList classes. Classes
#			manage command syntax and descriptions.
# Course Name: 		CS 372-400: Introduction to Computer Networks
# Last Modified:	03/09/2020
#######################################################################################################


#######################################################################################################
# Class Name: 		Command
# Class Description:	Simple object storing the syntax and description of a command.
# Data Members:		syntax: how the command should be entered on the command-line
#			description: explanation of what the command does
# Member Functions:	__init__ (constructor)
#######################################################################################################

class Command:
	
	#######################################################################################################
	# Function Name:	__init__
	# Description:		Instantiates a Command object.
	# Receives: 		Self-reference and a command's syntax and description, represented as strings.
	# Returns: 		The instantiated Command.
	# Pre-Conditions:	The received syntax and description are non-null strings.
	# Post-Conditions: 	The Command object has been instantiated.
	######################################################################################################
	
	def __init__(self, syntax, description):
		# Instantiate object's data members with parameters passed in.
		self.syntax = syntax
		self.description = description


#######################################################################################################
# Class Name: 		CommandList
# Class Description:	Class which instantiates object containing a list of Commands and implements
#			related methods.
# Data Members:		commands: a list of Command objects
# Member Functions:	__init__ (constructor)
#			__str__ (returns string representation)
#			validate (returns True if command valid, False otherwise)
#######################################################################################################

class CommandList:
	
	#######################################################################################################
	# Function Name:	__init__
	# Description:		Instantiates a CommandList object.
	# Receives: 		A self-reference and a list of Commands.
	# Returns: 		The instantiated CommandList on which class methods are now callable.
	# Pre-Conditions:	The received parameter is a list of Commands.
	# Post-Conditions: 	The CommandList object has been instantiated.
	######################################################################################################

	def __init__(self, commands):
		# Instantiate list to that passed in.
		self.commands = commands
	
	
	#######################################################################################################
	# Function Name:	__str__
	# Description:		Creates a string representation of the CommandList, formatted for easy printing.
	# Receives: 		A self-reference.
	# Returns: 		A nicely formatted human-readable string representation of the CommandList
	#			formatted as a tab-separated table.
	# Pre-Conditions:	The Commands in the CommandList contain valid, non-null strings representing
	#			a Command's syntax and description.
	# Post-Conditions:	The string representation of the CommandList is returned.
	######################################################################################################

	def __str__(self):
		# Declare and initialize CommandList string as column headings.
		commandStr = "SYNTAX:\tDESCRIPTION:\n"

		# Iterate through list of commands, adding new line for each command.
		for command in self.commands:
			commandStr += command.syntax + "\t" + command.description + "\n"

		# Return commandStr to calling function.
		return commandStr
	
	#######################################################################################################
	# Function Name:	validate
	# Description:		Validates whether CommandList contains Command with provided syntax.
	# Receives: 		A self-reference and a string representing a command's syntax.
	# Returns: 		True if CommandList contains command with that syntax; false otherwise.
	# Pre-Conditions:	The syntax passed in is a non-null string.
	# Post-Conditions:	If True is returned, the Command with syntax passed in is in CommandList.
	######################################################################################################

	def validate(self, syntax):
		# Conduct simple linear search of CommandList, returning True if command with given syntax
		# is encountered (linear search used since CommandList is assumed to be relatively short).
		for command in self.commands:
			if syntax == command.syntax:
				return True

		# If command with syntax not found in loop above,
		# return False.
		return False
