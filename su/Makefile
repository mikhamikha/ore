SS = arm-9tdmi-linux-gnueabi-gcc
GG = arm-9tdmi-linux-gnueabi-g++
TARGET = bu
PREFIX ?= /usr/local
S_FLAGS = -I ../libmodbus/src/ -I ../mqtt/paho -I ../noritake/src -I ../pugixml/src -I ../interface1/ -pthread -D_REENTRANT -Os -Wall #-ldl
S_SRCS=../libmodbus/src/modbus.c ../libmodbus/src/modbus-tcp.c ../libmodbus/src/modbus-rtu.c ../libmodbus/src/modbus-data.c \
	   ../mqtt/paho/Clients.c ../mqtt/paho/Heap.c ../mqtt/paho/LinkedList.c ../mqtt/paho/Log.c ../mqtt/paho/Messages.c ../mqtt/paho/MQTTAsync.c ../mqtt/paho/MQTTPacket.c \
	   ../mqtt/paho/MQTTPacketOut.c ../mqtt/paho/MQTTPersistence.c ../mqtt/paho/MQTTPersistenceDefault.c ../mqtt/paho/MQTTProtocolClient.c ../mqtt/paho/MQTTProtocolOut.c \
	   ../mqtt/paho/SocketBuffer.c ../mqtt/paho/Socket.c ../mqtt/paho/SSLSocket.c ../mqtt/paho/StackTrace.c ../mqtt/paho/Thread.c ../mqtt/paho/Tree.c ../mqtt/paho/utf-8.c\
	   ../interface1/interface.c 
G_SRCS=main.cpp unit.cpp unitdirector.cpp utils.cpp tag.cpp tagdirector.cpp mbxchg.cpp upcon.cpp algo.cpp display.cpp thread.cpp hist.cpp \
	   ../noritake/src/Noritake_VFD_GU3000.cpp  ../noritake/src/linux_serial.cpp ../pugixml/src/pugixml.cpp    
# temporally excluded:../sqlite/sqlite3.c -I ../sqlite -I ../libmodbus/ 

G_OBJS = $(G_SRCS:.cpp=.o) 
S_OBJS = $(S_SRCS:.c=.o) 

.PHONY: all clean install uninstall

all: $(TARGET)
$(TARGET): $(G_OBJS) $(S_OBJS)  
	$(GG) -o $(TARGET) $(S_OBJS) $(S_FLAGS) $(G_OBJS) 
.cpp.o:
	$(GG) $(S_FLAGS)  -c $< -o $@
.c.o:
	$(SS) $(S_FLAGS)  -c $< -o $@
clean:
	rm -rf $(TARGET) $(G_OBJS) $(S_OBJS)
install:
	install $(TARGET) $(PREFIX)/bin
uninstall:
	rm -rf $(PREFIX)/bin/$(TARGET)

