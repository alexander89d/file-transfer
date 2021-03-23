PY_FILES = CommandList.py FTInfo.py clientServerMessaging.py ftclient.py
H_FILES = clientServerMessaging.h FTInfo.h manageConnections.h
C_FILES = clientServerMessaging.c FTInfo.c manageConnections.c ftserver.c
EXEC_FILE = ftserver
ZIP_FILE = densmora_cs372_prog2.zip
COMP = gcc
FLAGS = -g -Wall --std=gnu99

ftserver: ${C_FILES} ${H_FILES}
	${COMP} ${FLAGS} ${C_FILES} -o ${EXEC_FILE}

clean:
	rm -f ${EXEC_FILE}

zip:
	zip -D ${ZIP_FILE} ${C_FILES} ${H_FILES} ${PY_FILES} README.txt makefile

cleanZip:
	rm -f ${ZIP_FILE}

cleanPycache:
	rm -rf __pycache__

# Clean up any text file copies (with underscore after name)
cleanTxt:
	rm -f *_*.txt
