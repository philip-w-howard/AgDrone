OPTS= -Wall -c -pthread
LIBS= -lmraa

OBJS=  AgDrone.o \
       Heartbeat.o \
       connection.o \
       mavlinkif.o \
       pixhawk.o \
       wificlient.o \
       wifiserver.o \
       agdronecmd.o \
       queue.o \

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
