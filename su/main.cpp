#define MQTTCLIENT_QOS2 1

#include <termios.h>
#include <unistd.h>
#include <modbus.h>
#include <MQTTClient.h>
#include <pthread.h>
#include <errno.h>
#include "param.h"
#include "main.h"
#include "mbxchg.h"

#define DEFAULT_STACK_SIZE -1

#include "linux.cpp"

using namespace std;
 
int arrivedcount = 0;

void outtext(std::string tx) {
  time_t      rawtime;
//    timespec    rawtime;
    struct tm   *ptm;
    
      time(&rawtime);
//    clock_gettime(CLOCK_MONOTONIC, &rawtime);
//    ptm = localtime(&(rawtime.tv_sec));
    ptm = localtime(&rawtime);
    cout<<setfill('0')<<setw(2)<<ptm->tm_hour<<":"<<\
          setfill('0')<<setw(2)<<ptm->tm_min<<":"<< \
          setfill('0')<<setw(2)<<ptm->tm_sec<<"\t"<<tx<<endl;
}

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
    
    if (readCfg()==EXIT_SUCCESS) {
        thMBX = new pthread_t[conn.size()+1]; 
        fieldconnections::iterator coni;
        fParamThreadInitialized=1;
        for(coni=conn.begin(), i=0; coni != conn.end(); ++coni) { 
            nResult = pthread_create(thMBX+i, NULL, fieldXChange, (void *)(*coni));
            if (nResult != 0) {
//              perror("Создание первого потока!");
//              return EXIT_FAILURE;
                break;
            }
            ++i;
        }            
        cout << "param thread " << i <<endl;
        nResult = pthread_create(thMBX+i, NULL,  paramProcessing, (void *)NULL);

       
// ----------- terminate block -------------
        struct termios oldt, newt;
        int ch;
        tcgetattr( STDIN_FILENO, &oldt );
        newt = oldt;
        newt.c_lflag &= ~( ICANON | ECHO );
        tcsetattr( STDIN_FILENO, TCSANOW, &newt );
        while (ch!='q' && ch!='Q') {
            ch = getchar();
            cout << ch << endl;
        }
        tcsetattr( STDIN_FILENO, TCSANOW, &oldt );
// ---------- end terminate block ------------
        
        cout << "end param thread " << i <<endl;
        fParamThreadInitialized=0;
        pthread_join(thMBX[i], NULL);

        for(coni=conn.begin(), i=0; coni != conn.end(); ++coni) { 
            (*coni)->terminate();
            pthread_join(thMBX[i], NULL);
            delete *coni;
            ++i;
        }
        delete []thMBX;
    }
}



