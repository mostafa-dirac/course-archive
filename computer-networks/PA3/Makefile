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

TARGET = node.out

BASE_SOURCES = base/main.cpp \
	base/cf.cpp \
	base/interface.cpp \
	base/machine.cpp \
	base/frame.cpp

ARCHIVE_PREFIX = CF

# DO NOT LIST .H FILES HERE! JUST .C AND .CPP FILES
USER_SOURCES = user/sm.cpp \
	user/facade.cpp

BASE_OBJ = ${BASE_SOURCES:.cpp=.o}
BASE_OBJECTS = ${BASE_OBJ:.c=.o}

USER_OBJ = ${USER_SOURCES:.cpp=.o}
USER_OBJECTS = ${USER_OBJ:.c=.o}

OBJECTS = ${BASE_OBJECTS} ${USER_OBJECTS}

all: ${TARGET}

${TARGET}: ${OBJECTS}
	${CXX} ${FLAGS} -o ${TARGET} ${OBJECTS} ${LIBS}

clean:
	tmpfolder=`echo /tmp/CF-clean-$$$$.removed` && mkdir -p $$tmpfolder && touch a && mv -t $$tmpfolder/ a `for a in ${OBJECTS} ${TARGET} *~ base/*~ user/*~ ; do if [ -e $$a ]; then echo $$a; fi; done`

archive:
	zip -r ${ARCHIVE_PREFIX}_`date +%s`.zip base/*.cpp base/*.h user/*.cpp user/*.h Makefile node.sh new.sh free.sh

.cpp.o:
	${CXX} ${FLAGS} -c $< -o $@
.c.o:
	${CXX} ${FLAGS} -c $< -o $@
