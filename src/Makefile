OPTS= -Wall -c -pthread -O2 -g
LIBS= -lmraa -lssl -lgnutls-openssl

OBJS=  AgDrone.o \
       Heartbeat.o \
       connection.o \
       mavlinkif.o \
       pixhawk.o \
       wificlient.o \
       wifiserver.o \
       datalist.o \
       agdronecmd.o \
       loglistcmd.o \
       dataflashcmd.o \
       getfilecmd.o \
       gettimecmd.o \
       gettlogscmd.o \
       getimagescmd.o \
       sendfile.o \
       queue.o \
       log.o \

all: AgDrone

clean:
	rm -f AgDrone
	rm -f $(OBJS)

.c.o:
	g++ $(OPTS) $? -o $@

.cpp.o:
	g++ $(OPTS) $? -o $@

AgDrone: $(OBJS)
	g++ $(OBJS) $(LIBS) -pthread -o AgDrone
