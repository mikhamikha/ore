CC=arm-9tdmi-linux-gnueabi-gcc
TARGET=bu
PREFIX ?= /usr/local
CFLAGS=-I ../src/ -I ../ 
SRCS=unit-test-server.c ../src/modbus.c ../src/modbus-rtu.c ../src/modbus-tcp.c ../src/modbus-data.c
#OBJS = $(SRCS:.cpp=.o) $(SRCS:.c=.o) 
OBJS = $(SRCS:.c=.o) 

.PHONY: all clean install uninstall

all: $(TARGET)
$(TARGET): $(OBJS)
	$(CC) -o $(TARGET) $(OBJS) $(CFLAGS)
.c.o:
	$(CC) $(CFLAGS)  -c $< -o $@
clean:
	rm -rf $(TARGET) $(OBJS)
install:
	install $(TARGET) $(PREFIX)/bin
uninstall:
	rm -rf $(PREFIX)/bin/$(TARGET)


