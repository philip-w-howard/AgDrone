OPTS= -Wall -c -pthread

OBJS=  AgDrone.o \
       Heartbeat.o \
       connection.o \
       mavlinkif.o \
       pixhawk.o \
       wificlient.o \
       wifiserver.o \

all: AgDrone

.cpp.o:
	g++ $(OPTS) $? -o $@

AgDrone: $(OBJS)
	g++ $(OBJS) -pthread -o AgDrone
