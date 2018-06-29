#ifndef _UPCONNECTION_HPP_
    #define _UPCONNECTION_HPP_

#include <iostream>
#include <sstream>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <vector>
#include <iterator>
#include <algorithm>
#include <signal.h>
#include <memory.h>

#include <sys/time.h>
#include <unistd.h>

#include "utils.h"
#include "thread.h"
#include "tag.h"
//
// add paho 
//
extern "C" {
    #include "MQTTAsync.h"
    #include "MQTTClientPersistence.h"
}

#define DEFAULT_STACK_SIZE -1
#define _pub_buf_max 100000

typedef struct
{
	char* clientid;
	int nodelimiter;
	char delimiter;
	int qos;
	char* username;
	char* password;
	char* host;
	char* port;
	int showtopics;
	int keepalive;
} mqtt_create_options; 
/* =
{
	"stdout-subscriber-async", 1, '\n', 2, NULL, NULL, "localhost", "1883", 0, 10
};
*/

typedef std::vector<std::pair<std::string, std::string> > pubdata;

class upcon: public cproperties<content>, public cthread, public iObserver<ctag> {

//    pubdata                     pubs;
    int                         m_status;
    MQTTAsync                   m_client;
    mqtt_create_options         m_opts;
//    MQTTAsync_connectOptions    *conn_opts; 
    bool                        m_connected;
        
    public:
        int32_t m_id; 
        upcon();
        ~upcon();
        int16_t connect();                                   // connect to broker
        int16_t disconnect();
        int16_t publish(ctag &);
        int16_t subscribe(void* );
        int16_t pubdataproc();                              // publication of data from buffer
        int16_t getstatus() { return m_status; };
        int16_t terminate() { m_status = TERMINATE; return EXIT_SUCCESS; }
        uint32_t handle() { return uint32_t(m_client); } 
        void run();
        bool connected();
        void connected(bool v);
        virtual void valueChanged( ctag& value );   
};

typedef std::vector< upcon * >  upconnections;
extern upconnections            upc;
//extern MQTTAsync_connectOptions g_conn_opts;
extern pubdata                  pubs;
  
int32_t messageArrived(void *context, char *topicName, int topicLen, MQTTAsync_message *message);
void connectionLost(void *context, char *cause);
void* upProcessing(void *args); // поток обработки обмена с верхним уровнем


#endif
