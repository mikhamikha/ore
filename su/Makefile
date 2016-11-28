SS = arm-9tdmi-linux-gnueabi-gcc
GG = arm-9tdmi-linux-gnueabi-g++
TARGET = bu
PREFIX ?= /usr/local
S_FLAGS = -I ../libmodbus/src/ -I ../libmodbus/ -I ../mqtt/MQTTClient/src -I ../mqtt/MQTTClient/src/linux -I ../mqtt/MQTTPacket/src -I ../noritake/src
S_SRCS=../libmodbus/src/modbus.c ../libmodbus/src/modbus-tcp.c ../libmodbus/src/modbus-rtu.c ../libmodbus/src/modbus-data.c  
G_SRCS=hello.cpp param.cpp ../noritake/src/Noritake_VFD_GU3000.cpp  ../noritake/src/linux_serial.cpp \
../mqtt/MQTTPacket/src/MQTTPacket.cpp ../mqtt/MQTTPacket/src/MQTTDeserializePublish.cpp ../mqtt/MQTTPacket/src/MQTTConnectClient.cpp \
../mqtt/MQTTPacket/src/MQTTSubscribeClient.cpp ../mqtt/MQTTPacket/src/MQTTSerializePublish.cpp ../mqtt/MQTTPacket/src/MQTTUnsubscribeClient.cpp   
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


