SS = arm-9tdmi-linux-gnueabi-gcc
TARGET = inter
PREFIX ?= /usr/local
S_SRCS=interface.c
S_OBJS = $(S_SRCS:.c=.o) 

.PHONY: all clean install uninstall

all: $(TARGET)
$(TARGET): $(S_OBJS)  
	$(SS) -o $(TARGET) $(S_OBJS)   
.c.o:
	$(SS) -c $< -o $@
clean:
	rm -rf $(TARGET) $(S_OBJS)
install:
	install $(TARGET) $(PREFIX)/bin
uninstall:
	rm -rf $(PREFIX)/bin/$(TARGET)

