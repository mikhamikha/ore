#define MQTTCLIENT_QOS2 1

#include "param.h"

#include "main.h"
#include "mbxchg.h"
#include "modbus.h"
#include "MQTTClient.h"
#include <pthread.h>
#include <errno.h>

#define DEFAULT_STACK_SIZE -1

#include "linux.cpp"

using namespace std;
 
int arrivedcount = 0;

void messageArrived(MQTT::MessageData& md)
{
    MQTT::Message &message = md.message;

	printf("Message %d arrived: qos %d, retained %d, dup %d, packetid %d\n", 
		++arrivedcount, message.qos, message.retained, message.dup, message.id);
    printf("Payload %.*s\n", message.payloadlen, (char*)message.payload);
}


int main(int argc, char* argv[])
{
    int         nResult;
    uint32_t    i;
    pthread_t   *thMBX;
    
    if (readCfg()==_res_ok) {
        /*
        thMBX = new pthread_t[conn.size()]; 
        std::vector< cmbxchg, allocator<cmbxchg> >::iterator coni;
        for(coni=conn.begin(), i=0; coni != conn.end(); ++coni) { 
            nResult = pthread_create(thMBX+i, NULL, fieldXChange, (void *)(&(*coni)));
            if (nResult != 0) {
//              perror("Создание первого потока!");
//              return EXIT_FAILURE;
                break;
            }
            ++i;
        }
        sleep(10);
        for(coni=conn.begin(), i=0; coni != conn.end(); ++coni) { 
            (*coni).terminate();
            pthread_join(thMBX[i], NULL);
            ++i;
        }
        */
    }
}



