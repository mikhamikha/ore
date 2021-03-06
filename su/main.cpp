#define MQTTCLIENT_QOS2 1

#include <time.h>
#include <stdint.h>
#include <iostream>
#include <iomanip>
#include <string>
#include <ctype.h>
#include <termios.h>
#include <unistd.h>
#include <modbus.h>
#include <pthread.h>
#include <errno.h>
#include <sys/mman.h>
#include <fcntl.h>

#include "main.h"
#include "upcon.h"
#include "unitdirector.h"
#include "tagdirector.h"
#include "algo.h"
#include "mbxchg.h"
#include "display.h"
#include "hist.h"

using namespace std;

int main(int argc, char* argv[]) {
    
    if( argc>1 ) {
        string _s = argv[1];
        _s = trim( _s );
        if( !_s.compare("-c") || !_s.compare("--clear") ) {
            hist.clear();
            cout<<"History on SRAM was cleared\n";
        }
        else {
            if( !_s.compare("-v") || !_s.compare("--version") ) {
                cout<<"bu version "<<g_sVersion<<endl;;
            }
            else {
                cout<<"Typically usage of this program:\n";
                cout<<"bu [-c]\n";
                cout<<"where:\n";
                cout<<"\t-c, --clear \t\tclean SRAM history and exit\n";
                cout<<"\t-h, --help \t\tShow this help and exit\n";
                cout<<"\t-v, --version \t\tshow program version and exit\n";
            }
        }
        return 0;
    }
    hist.init();

    setDT();
    if (readCfg()==EXIT_SUCCESS) {
        int ret=0;
        ret = pthread_mutexattr_settype(&mutex_tag_attr, PTHREAD_MUTEX_ERRORCHECK_NP); 
        // PTHREAD_MUTEX_ERRORCHECK_NP avoids double locking on same thread.
        
        if(ret != 0) {
            printf("Mutex attribute not initialized!!\n");
        }
        ret = pthread_mutex_init(&mutex_tag, &mutex_tag_attr);
        if(ret != 0) {
            printf("Mutex not initialized!!\n");
        }
        cout << "readCFG OK!" << endl;

        ftagThreadInitialized=1;
        
        fieldconnections::iterator coni;
        for(coni=conn.begin(); coni != conn.end(); ++coni) (*coni)->start();
        
        upconnections::iterator up;
        for(up=upc.begin(); up != upc.end(); ++up) (*up)->start();
        
        tagdir.start();     
//        unitdir.start();
        
        dsp.start();

// ----------- terminate block -------------
        struct termios oldt, newt;
        int ch=0;
        tcgetattr( STDIN_FILENO, &oldt );
        newt = oldt;
        newt.c_lflag &= ~( ICANON | ECHO );
        tcsetattr( STDIN_FILENO, TCSANOW, &newt );
        while (ch!='q' && ch!='Q') {
            ch = getchar();
            cout << dec<<"Pressed " << ch<<" conn="<<int(conn[0])<<endl;
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
                cout << "read  400-419: ";
                for(int j=400; j<420; j++) {
                    cout<<conn[0]->m_pReadData[j]<<" ";
                }            
                cout << endl;
                cout << "read counters  051-054: ";
                for(int j=51; j<55; j++) {
                    cout<<conn[0]->m_pReadData[j]<<" ";
                }            
                cout << endl;
                cout << "read  000-019: \n";
                for(int j=0; j<20; j++) {
                    cout<< setfill(' ')<<setw(6)<<j;
                }            
                cout << endl;
                for(int j=0; j<20; j++) {
                    cout<< setfill(' ')<<setw(6)<<conn[0]->m_pReadData[j];
                }            
                cout << endl;
                cout << "read  060-075: \n";
                for(int j=60; j<76; j++) {
                    cout<< setfill(' ')<<setw(6)<<j;
                }            
                cout << endl;
                for(int j=60; j<76; j++) {
                    cout<< setfill(' ')<<setw(6)<<conn[0]->m_pReadData[j];
                }            
                cout << endl;
           }
           else if(ch=='m' || ch=='M') {
               histCellBody _hc;
               string       _hn;
//               int          _rc;
               while( hist.pull( _hn, _hc )==_exOK )
                   cout<<"tag="<<_hn<<"\tpv="<<_hc.m_value<<"\tq=0x"<<hex
                       <<uint16_t(_hc.m_qual)<<dec<<"\tts="<<_hc.m_ts<<endl;
           } 
            cout << endl;
        }
        tcsetattr( STDIN_FILENO, TCSANOW, &oldt );
// ---------- end terminate block ------------
        
        ftagThreadInitialized=0;
        dsp.join();
//        unitdir.join();
        tagdir.join();

        cout << "end mqtt thread ";
        for(upconnections::reverse_iterator up=upc.rbegin(); up != upc.rend(); ++up) { 
            (*up)->terminate();
            (*up)->join();
            delete *up;
        }
      
        fieldconnections::reverse_iterator rconi;          
        for(rconi=conn.rbegin(); rconi != conn.rend(); ++rconi) { 
            (*rconi)->terminate();
            (*rconi)->join();
            delete *rconi;
        }

        cout<<endl;
        pthread_mutexattr_destroy(&mutex_tag_attr);   // clean up the mutex attribute
        pthread_mutex_destroy(&mutex_tag);            // clean up the mutex itself
    }
}



