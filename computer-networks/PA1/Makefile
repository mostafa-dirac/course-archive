#################################################
##              In the name of GOD
##
## Copyright © 2009-2015 Behnam Momeni.
## All rights reserved.
##
## This file is part of the Partov project.
##
#################################################

CXX = g++
FLAGS = -Wall -g -Ibase
LIBS = -lpthread

TARGET_USER = machine.out
TARGET = ${TARGET_USER}

BASE_SOURCES = base/main.cpp \
	base/cf.cpp \
	base/interface.cpp \
	base/machine.cpp \
	base/frame.cpp

SUB_NAME = PA1_CPP

# DO NOT LIST .H FILES HERE! JUST .C AND .CPP FILES
USER_SOURCES = user/sm.cpp \
	user/facade.cpp \
	user/client_machine.cpp \
	user/simpleMachine.cpp \
	user/server_machine.cpp
	

BASE_OBJ = ${BASE_SOURCES:.cpp=.o}
BASE_OBJECTS = ${BASE_OBJ:.c=.o}

USER_OBJ = ${USER_SOURCES:.cpp=.o}
USER_OBJECTS = ${USER_OBJ:.c=.o}

OBJECTS_USER = ${BASE_OBJECTS} ${USER_OBJECTS}

OBJECTS = ${BASE_OBJECTS} ${USER_OBJECTS}

all: ${TARGET_USER}

${TARGET_USER}: ${OBJECTS_USER}
	${CXX} ${FLAGS} -o ${TARGET_USER} ${OBJECTS_USER} ${LIBS}

clean:
	tmpfolder=`echo /tmp/CF-clean-$$$$.removed` && mkdir -p $$tmpfolder && date >a && files="a `for a in ${OBJECTS} ${TARGET} *~ */*~; do if [ -e $$a ]; then echo $$a; fi; done`" && tar cf $$tmpfolder/backup.tar $$files && rm -f $$files

archive:
	zip -r ${SUB_NAME}_`date +%s`.zip user*/*.c user*/*.cpp user*/*.h base/*.cpp base/*.h base/*.c Makefile run.sh free.sh new.sh

.cpp.o:
	${CXX} ${FLAGS} -c $< -o $@
.c.o:
	${CXX} ${FLAGS} -c $< -o $@

