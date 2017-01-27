#define MQTTCLIENT_QOS2 1

#include <termios.h>
#include <unistd.h>
#include <modbus.h>
#include <pthread.h>
#include <errno.h>
#include "main.h"

using namespace std;
 
int main(int argc, char* argv[])
{
    int         nResult;
    uint32_t    i;
    pthread_t   *thMBX;
    
    setDT();
    if (readCfg()==EXIT_SUCCESS) {
        cout << "readCFG OK!" << endl;
        thMBX = new pthread_t[conn.size() + upc.size() + 2]; 
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
        for(upconnections::iterator up=upc.begin(); up != upc.end(); ++up) {
            if( (*up)->connect()==EXIT_SUCCESS )
                nResult = pthread_create(thMBX+i, NULL, upProcessing, (void *)(*up));
            if (nResult != 0) {
//              perror("Создание первого потока!");
//              return EXIT_FAILURE;
                break;
            }            
            ++i;
        }
        nResult = pthread_create(thMBX+i++, NULL,  paramProcessing, (void *)NULL);
        nResult = pthread_create(thMBX+i, NULL,  viewProcessing, (void *)NULL);
        //          printf("А=%d Я=%d Ё=%d | а=%d п=%d р=%d я=%d ё=%d\n",'А', 'Я', 'Ё', 'а', 'п', 'р', 'я', 'ё');
        //          printf("А=%d Я=%d Ё=%d | а=%d п=%d р=%d я=%d ё=%d\n",wchar_t(L'А'), wchar_t(L'Я'), wchar_t(L'Ё'), wchar_t(L'а'), wchar_t(L'п'), wchar_t(L'р'), wchar_t(L'я'), wchar_t(L'ё'));
//        cout << "param thread " << i <<endl;
       
// ----------- terminate block -------------
        struct termios oldt, newt;
        int ch;
        tcgetattr( STDIN_FILENO, &oldt );
        newt = oldt;
        newt.c_lflag &= ~( ICANON | ECHO );
        tcsetattr( STDIN_FILENO, TCSANOW, &newt );
        while (ch!='q' && ch!='Q') {
            ch = getchar();
            cout << "Pressed " << ch<<endl;;
            if(isdigit(ch) && ch<522) {
                conn[0]->m_pWriteData[ch-48]=(conn[0]->m_pWriteData[ch-48]==0);
                cout << " == " << ch-48 << " | " << conn[0]->m_pWriteData[ch-48];
            }
            else if(ch=='d' || ch=='D') {
                cout << " lastdata 0-9: ";
                for(int j=0; j<10; j++) {
                    cout<<conn[0]->m_pLastWriteData[j]<<" ";
                }            
                cout << endl;
                cout << "writedata 0-9: ";
                for(int j=0; j<10; j++) {
                    cout<<conn[0]->m_pWriteData[j]<<" ";
                }            
                cout << endl;
                cout << "read  400-409: ";
                for(int j=400; j<410; j++) {
                    cout<<conn[0]->m_pReadData[j]<<" ";
                }            
                cout << endl;
            }

            cout << endl;
        }
        tcsetattr( STDIN_FILENO, TCSANOW, &oldt );
// ---------- end terminate block ------------
        
        cout << "end param thread " << i <<endl;
        fParamThreadInitialized=0;
        pthread_join(thMBX[i--], NULL);
        pthread_join(thMBX[i], NULL);
        
        for(coni=conn.begin(), i=0; coni != conn.end(); ++coni) { 
            (*coni)->terminate();
            pthread_join(thMBX[i], NULL);
            delete *coni;
            ++i;
        }
        for(upconnections::iterator up=upc.begin(); up != upc.end(); ++up) { 
            (*up)->terminate();
            pthread_join(thMBX[i], NULL);
            delete *up;
            ++i;
        }
        delete []thMBX;
    }
}



