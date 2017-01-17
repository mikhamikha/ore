#ifndef _linux_h
    #define _linux_h

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

#include <stdlib.h>
#include <string.h>
#include <signal.h>


class IPStack 
{
public:    
    IPStack()
    {

    }
    
	int Socket_error(const char* aString);
    int connect(const char* hostname, int port);
    int read(unsigned char* buffer, int len, int timeout_ms);
    int write(unsigned char* buffer, int len, int timeout);
	int disconnect();
    
private:

    int mysock; 
    
};


class Countdown
{
public:
    Countdown()
    { 
	
    }

    Countdown(int ms);
    bool expired();
    void countdown_ms(int ms);  
    void countdown(int seconds);
    int left_ms();
    
private:

	struct timeval end_time;
};

#endif

