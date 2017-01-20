#ifndef _upcon_h
    #define _upcon_h

#include <iostream>
#include <sstream>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <vector>
#include <iterator>
#include <algorithm>
#include "main.h"
#include "linux.h"
#include "utils.h"

#include "MQTTClient.h"

#define DEFAULT_STACK_SIZE -1
#define _pub_buf_max 100000

typedef std::vector<std::pair<std::string, std::string> > pubdata;

class upcon: public cproperties {
    IPStack                             m_ipstack;
    MQTT::Client<IPStack, Countdown>    *m_client;
    pubdata pubs;
    int                 m_status;

    public:
        int32_t m_id; 
       
        upcon();
        ~upcon();
        int16_t connect();                                   // connect to broker
        int16_t publish(cparam &);
        int16_t pubdataproc();              // publication of data from buffer
        int16_t getStatus() { return m_status; };
        int16_t terminate() { m_status = TERMINATE; return EXIT_SUCCESS; }
};

typedef std::vector< upcon * > upconnections;
extern upconnections upc;

void messageArrived(MQTT::MessageData& md);
void* upProcessing(void *args); // поток обработки обмена с верхним уровнем


#endif
