#include "utils.h"
#include "main.h"
#include "mbxchg.h"
#include <errno.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>

//
using namespace std;
fieldconnections conn;
int16_t *cmbxchg::m_pReadData;
int16_t *cmbxchg::m_pWriteData;
int16_t *cmbxchg::m_pLastWriteData;

ccmd::ccmd(const ccmd &s)
{
    m_enable     =  s.m_enable;   
    m_intAddress =  s.m_intAddress;
    m_count      =  s.m_count;    
    m_pollInt    =  s.m_pollInt;  
    m_node       =  s.m_node;     
    m_func       =  s.m_func;     
    m_devAddr    =  s.m_devAddr;  
    m_swap       =  s.m_swap;     
    m_err.first  =  s.m_err.first;
    m_err.second =  s.m_err.second;
}
ccmd::ccmd(std::vector<int16_t> &v)
{
    m_enable     = v[0];
    m_intAddress = v[1]; 
    m_count      = v[2]; 
    m_pollInt    = v[3]; 
    m_node       = v[4]; 
    m_func       = v[5]; 
    m_devAddr    = v[6]; 
    m_swap       = v[7];
    m_err        = std::make_pair(0, "No Errors");
    m_first      = true;
    cout << ToString() << endl;
}

std::string ccmd::ToString()
{
    std::string s;
    std::stringstream out;
    out << "en="<<m_enable<<" int="<<m_intAddress<<" poll="<<m_pollInt<< \
            " node="<<m_node<<" func="<<m_func<<" addr="<<m_devAddr<<" count="<<m_count<< \
            " swap="<<m_swap<<" err="<<m_err.first<<" errDesc="<<m_err.second;
    s = out.str();
    return s;
}

content& cmbxchg::portProperty(const int i)
{
    return ps[i].second;
}

std::string cmbxchg::portProperty2Text(const int i)
{
    char buf[100];
    std::string s = ps[i].first + " = ";
    if (ps[i].second._t==0) s += to_string(ps[i].second._n);
    if (ps[i].second._t==1) s += to_string(ps[i].second._c);
    if (ps[i].second._t==2) s += ps[i].second._s;
    return s;
}

content& cmbxchg::portProperty(const char *sFind)
{
    content *ct;
    portsettings::iterator i = std::find_if(ps.begin(), ps.end(), compContent(sFind));
    if (i != ps.end()) {
        ct = &(i->second);
    }
    return *ct;
}

int32_t cmbxchg::portPropertySet(const char *sFind, content& ct)
{
    int32_t res=EXIT_FAILURE;
    portsettings::iterator i = std::find_if(ps.begin(), ps.end(), compContent(sFind));
    
    if (i != ps.end()) {
        i->second = ct;
        res = EXIT_SUCCESS;
    }
    return res;
}

cmbxchg::cmbxchg()
{
    ps.push_back( std::make_pair("path",                content("")) );///dev/ttyS2")) );
    ps.push_back( std::make_pair("enabled",             content(0)) );
    ps.push_back( std::make_pair("protocol",            content(RTU)) );    // пока работаем только RTU
    ps.push_back( std::make_pair("baudrate",            content(0)) );//9600)) );
    ps.push_back( std::make_pair("parity",              content('E')) );
    ps.push_back( std::make_pair("databits",            content(7)) );
    ps.push_back( std::make_pair("stopbits",            content(1)) );
    ps.push_back( std::make_pair("minimumcommanddelay", content(10)) );
    ps.push_back( std::make_pair("commanderrorpointer", content(500)) );
    ps.push_back( std::make_pair("responsetimeout",     content(1000)) );
    ps.push_back( std::make_pair("retrycnt",            content(0)) );
    ps.push_back( std::make_pair("errordelaycntr",      content(0)) );
    m_maxReadData=500;
    m_maxWriteData=500;
}
//
//  Modbus connection initialization
//
int16_t cmbxchg::init()
{   
    if (portProperty("proto")._n== TCP) {
        m_ctx = modbus_new_tcp("127.0.0.1", 1502);              
    } 
    else    
        if (portProperty("proto")._n == TCP_PI) {
            m_ctx = modbus_new_tcp_pi("::1", "1502");
        }   
        else {
            m_ctx = modbus_new_rtu(
                        portProperty("path")._s.c_str(), 
                        portProperty("baud")._n, 
                        easytoupper(portProperty("parity")._c), 
                        portProperty("databits")._n, 
                        portProperty("stop")._n
                        );
        }

    if (m_ctx == NULL) {
        m_status = INIT_ERR;
//        fprintf(stderr, "Unable to allocate libmodbus context\n");
//        return -1;
    } 
    else {
        m_status = INITIALIZED;
        cout << "init serial | "<<portProperty("path")._s<<" | "<<portProperty("baud")._n<<" | "<< \
            easytoupper(portProperty("parity")._c)<<" | "<<portProperty("databits")._n<<" | "<<   \
            portProperty("stop")._n<<" | "<<portProperty("response")._n<<" | "<<portProperty("errordelay")._n<<endl;
//      modbus_set_debug(m_ctx, TRUE);
//      modbus_set_error_recovery(m_ctx, MODBUS_ERROR_RECOVERY_LINK | MODBUS_ERROR_RECOVERY_PROTOCOL);
    }
    return m_status;
}
//
//  set flag for terminate thread
//
int16_t cmbxchg::getStatus()
{
    return m_status;
}

//
//  set flag for terminate thread
//
int16_t cmbxchg::terminate()
{
    m_status = TERMINATE;
    return EXIT_SUCCESS;
}
//
// поток обмена по Modbus с полевым оборудованием
//
void* fieldXChange(void *args) 
{
    cmbxchg *mbx=(cmbxchg *)(args);
    if(mbx->init() == INITIALIZED) {
        mbx->m_pReadData = new int16_t[mbx->m_maxReadData+1];
        mbx->m_pWriteData = new int16_t[mbx->m_maxWriteData+1];
        mbx->m_pLastWriteData = new int16_t[mbx->m_maxWriteData+1];
        cout << "start xchg " << mbx << " | " << mbx->m_pReadData <<endl;
//        cout << "minimum command delay = " << mbx->portProperty("minimumcommand")._n << endl;
        while (mbx->getStatus()!=TERMINATE) {
            mbx->runCmdCycle();
        }     
        delete []mbx->m_pReadData;
        delete []mbx->m_pWriteData;    
        delete []mbx->m_pLastWriteData;    
    }
    cout << "end xchg " << args << endl;
    return EXIT_SUCCESS;
}
//
//  modbus cycle commands
//
int16_t cmbxchg::runCmdCycle() 
{
    int32_t                 res=0;
    int16_t                 rc, i;
    mbcommands::iterator    cmdi;
    uint32_t                old_resp_to_sec;
    uint32_t                old_resp_to_usec;
    uint32_t                to_sec;
    uint32_t                to_usec;
    bool                    fTook;              // delay must be

    cmdi = cmds.begin(); i=0;
    while(cmdi!=cmds.end()) {
       
        if((*cmdi).m_enable && ( (*cmdi).m_first || (*cmdi).m_time.isDone()) ) {
            rc=-1;fTook=true;
            if (portProperty("proto")._n == RTU) {
                rc = modbus_set_slave(m_ctx, (*cmdi).m_node);
            }    
            if (rc==0) rc = modbus_connect(m_ctx);
            if (rc==0) rc = modbus_get_response_timeout( m_ctx, &old_resp_to_sec, &old_resp_to_usec );
            if (rc==0) rc = modbus_get_byte_timeout( m_ctx, &to_sec, &to_usec );
            if (rc==0) rc = modbus_set_response_timeout( m_ctx, 0, 1000*portProperty("response")._n );
            if (rc==0) rc = modbus_set_byte_timeout( m_ctx, 0, 10000);
            if (rc==0) {
                pthread_mutex_lock( &mutex_param );
                switch((*cmdi).m_func) {                // modbus commands queue processing
                    case 0x03: 
                        rc = modbus_read_registers( \
                                m_ctx,              \
                                (*cmdi).m_devAddr,  \
                                (*cmdi).m_count,    \
                                (uint16_t *)m_pReadData+(*cmdi).m_intAddress    \
                                ); 
                        break;
                    case 0x04: 
                        rc = modbus_read_input_registers(   \
                                m_ctx,                      \
                                (*cmdi).m_devAddr,          \
                                (*cmdi).m_count,            \
                                (uint16_t *)m_pReadData+(*cmdi).m_intAddress    \
                                );
                        break;
                    case 0x05:
                        // write bit if enable==1 or (2 and new<>old)
                        if( (*cmdi).m_enable==1 ||    \
                            m_pWriteData[(*cmdi).m_intAddress]!=m_pLastWriteData[(*cmdi).m_intAddress] ) {
                            
//                          outtext((*cmdi).ToString());
                            rc = modbus_write_bit(      \
                                    m_ctx,              \
                                    (*cmdi).m_devAddr,  \
                                    m_pWriteData[(*cmdi).m_intAddress]   \
                                    );
//                            cout << "rc write = "<<rc<<" | last "<<m_pLastWriteData[(*cmdi).m_intAddress]<< \
                                    " | new "<<m_pWriteData[(*cmdi).m_intAddress]<<endl;
                            if(rc==1)
                                m_pLastWriteData[(*cmdi).m_intAddress]=m_pWriteData[(*cmdi).m_intAddress];
                        } 
                        else fTook=false;
                        break;
                }
            }
            if (rc == -1) {
                (*cmdi).m_err.first   = errno;
                (*cmdi).m_err.second  = modbus_strerror(errno);
                m_pReadData[portProperty("commanderror")._n+i] =       \
                                    (errno>MODBUS_ENOBASE)?errno-MODBUS_ENOBASE:errno;
                outtext((*cmdi).ToString());
//              fprintf(stderr, "%s\n", modbus_strerror(errno));
            }
            pthread_cond_signal( &start_param );
            pthread_mutex_unlock( &mutex_param );
            modbus_set_byte_timeout( m_ctx, to_sec, to_usec );
            modbus_set_response_timeout( m_ctx, old_resp_to_sec, old_resp_to_usec );
            modbus_close(m_ctx);            
            if(rc != -1) {                              // if read/write is good
                int t;
                t = ((*cmdi).m_pollInt > 0) ? (*cmdi).m_pollInt : portProperty("minimumcommand")._n;
                (*cmdi).m_time.start( (t>0) ? t : 1);   // then set normal poll interval in ms

//                cout << "OK\n";
            }
            else {                                      // if read/write no good
                (*cmdi).m_time.start( \
                        (portProperty("errordelay")._n > 0) ?  \
                         portProperty("errordelay")._n : 10000 \
                        );                              // then set ErrorDelayCntr in ms  
//                cout << "no OK\n";
            }
        }
        if(fTook) usleep(portProperty("minimumcommand")._n*1000l);
        ++cmdi; ++i;
    }
    (*cmdi).m_first = false;
    return res;
}

