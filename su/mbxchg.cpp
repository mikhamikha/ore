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
int16_t *cmbxchg::m_pReadTrigger;
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
    m_first      =  s.m_first;
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
    m_errCnt     = 0;
    cout << ToString() << endl;
}

std::string ccmd::ToString()
{
    std::string s;
    std::stringstream out;
    out << " node="<<m_node<<" func="<<m_func<<" addr="<<m_devAddr<<" count="<<m_count<< \
           " en="<<m_enable<<" int="<<m_intAddress<<" poll="<<m_pollInt<< \
           " swap="<<m_swap<<" errCnt="<<m_errCnt<<" err="<<m_err.first<<" errDesc="<<m_err.second<<" fi="<<m_first;
    s = out.str();
    return s;
}

cmbxchg::cmbxchg()
{
    setproperty( "path",                "" );///dev/ttyS2")) );
    setproperty( "enabled",             int16_t(0) );
    setproperty( "protocol",            int16_t(RTU) );    // пока работаем только RTU
    setproperty( "baudrate",            int16_t(0) );//9600)) );
    setproperty( "parity",              int16_t('E') );
    setproperty( "databits",            int16_t(7) );
    setproperty( "stopbits",            int16_t(1) );
    setproperty( "minimumcommanddelay", int16_t(10) );
    setproperty( "commanderrorpointer", int16_t(500) );
    setproperty( "responsetimeout",     int16_t(1000) );
    setproperty( "retrycnt",            int16_t(0) );
    setproperty( "errordelaycntr",      int16_t(0) );
    m_maxReadData = 500;
    m_maxWriteData= 500;
}
//
//  Modbus connection initialization
//
int16_t cmbxchg::init()
{   
    int16_t proto;
    int16_t rc = getproperty("protocol", proto);
    std::string path;
    int16_t     baud, parity, data, stop;
    
    m_ctx = NULL;
    if(rc==EXIT_SUCCESS) {
        if (proto == TCP) {
            m_ctx = modbus_new_tcp("127.0.0.1", 1502);              
        } 
        else {   
            if (proto == TCP_PI) {
                m_ctx = modbus_new_tcp_pi("::1", "1502");
            }   
            else {
                rc = getproperty("path", path)       | \
                     getproperty("baudrate", baud)   | \
                     getproperty("parity", parity)   | \
                     getproperty("databits", data)   | \
                     getproperty("stopbits", stop);
                if(rc == 0) {
                    m_ctx = modbus_new_rtu( path.c_str(), baud, _parities[parity], data, stop );
                }
            }
        }
    }
    if (m_ctx == NULL) {
        m_status = INIT_ERR;
//        fprintf(stderr, "Unable to allocate libmodbus context\n");
//        return -1;
    } 
    else {
        m_status = INITIALIZED;
        cout << "init serial | "<<path<<" | "<<baud<<" | "<< parity<<" | "<<data<<" | "<< stop<< endl;
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
        mbx->m_pReadTrigger = new int16_t[mbx->m_maxReadData+1];
        mbx->m_pWriteData = new int16_t[mbx->m_maxWriteData+1];
        mbx->m_pLastWriteData = new int16_t[mbx->m_maxWriteData+1];
        memset(mbx->m_pReadData, 0, (mbx->m_maxReadData+1)*sizeof(int16_t) );
        memset(mbx->m_pReadTrigger, 0, (mbx->m_maxReadData+1)*sizeof(int16_t) );
        memset(mbx->m_pWriteData, 0, (mbx->m_maxWriteData+1)*sizeof(int16_t) );
        memset(mbx->m_pLastWriteData, 0, (mbx->m_maxWriteData+1)*sizeof(int16_t) );
        cout << "start xchg " << mbx << " | " << mbx->m_pReadData <<endl;
//        cout << "minimum command delay = " << mbx->getproperty("minimumcommand")._n << endl;
        while (mbx->getStatus()!=TERMINATE) {
            mbx->runCmdCycle(false);
        }     
        mbx->runCmdCycle(true);
        delete []mbx->m_pReadData;
        delete []mbx->m_pReadTrigger;
        delete []mbx->m_pWriteData;    
        delete []mbx->m_pLastWriteData;    
    }
    cout << "end xchg " << args << endl;
    return EXIT_SUCCESS;
}
//
//  modbus cycle commands
//
int16_t cmbxchg::runCmdCycle(bool fLast=false) 
{
    int32_t                 res=0;
    int16_t                 rc, i;
    mbcommands::iterator    cmdi;
    uint32_t                old_resp_to_sec;
    uint32_t                old_resp_to_usec;
    uint32_t                to_sec;
    uint32_t                to_usec;
    bool                    fTook;              // delay must be
    int16_t                 resp, proto, erroff, mincmddel, errdel;
    bool                    fConnected = false;
//----- cycle time measure-------------------------------------
    timespec    tvs, tve;
    int32_t     nTime;
    int64_t     nsta, nfin;
    int64_t     nD;

    clock_gettime(CLOCK_MONOTONIC,&tvs);
    nsta = tvs.tv_sec*_million + tvs.tv_nsec/1000;
//---------------------------------------------------------------    
    cmdi = cmds.begin(); i=0;
    rc = getproperty( "protocol", proto )               | \
         getproperty( "responsetim", resp )             | \
         getproperty( "minimumcommand", mincmddel )  | \
         getproperty( "errordelay", errdel )         | \
         getproperty( "commanderror", erroff );
    
//    cout<<"rc="<<rc<<" proto="<<proto<<" response="<<resp<<" minimumcommand="<<mincmddel \
        <<" errordelay="<<errdel<<" commanderror="<<erroff<<" fConn="<<fConnected<<endl;

    if(!fConnected) {
        rc = modbus_connect(m_ctx);
        if(!rc) {
            modbus_get_response_timeout( m_ctx, &old_resp_to_sec, &old_resp_to_usec );
            modbus_get_byte_timeout( m_ctx, &to_sec, &to_usec );
            rc = modbus_set_response_timeout( m_ctx, 0, 1000*resp );
            rc = modbus_set_byte_timeout( m_ctx, 0, 100000);
            if(!rc) fConnected = true;
        }
    }

    if( rc == 0)
    while(cmdi!=cmds.end()) {
        if(i==900) {
            cout<<"cmd="<<i<<" n="<<(*cmdi).m_node<<" f="<<(*cmdi).m_func<<" en="<<(*cmdi).m_enable<<" fi="<<(*cmdi).m_first<<endl;
            outtext(cmdi->ToString());
        }
        if( cmdi->m_enable && ( cmdi->m_first || cmdi->m_time.isDone() ) ) {
            fTook = true; rc = 0;
            if(proto == RTU) {
                rc = modbus_set_slave(m_ctx, cmdi->m_node);
            }   
            if(rc==0) {
                pthread_mutex_lock( &mutex_param );
                switch(cmdi->m_func) {                // modbus read commands queue processing
                    case 1: 
                        {
                            uint8_t *tab_value = new uint8_t[cmdi->m_count];
                            rc = modbus_read_bits( m_ctx, cmdi->m_devAddr, cmdi->m_count, tab_value );
                            for(int16_t j=0; j<cmdi->m_count; j++) m_pReadData[cmdi->m_intAddress+j] = tab_value[j];
                            if(0 && i==1) {
                                cout<<"func 1 :";
                                for( int j=0; j<cmdi->m_count; j++ ) cout<<m_pReadData[cmdi->m_intAddress+j]<<" ";
                                cout<<endl;                            
                            }
                            delete []tab_value;
                        }
                        break;

                    case 3: 
                    case 103: 
                       rc = modbus_read_registers( \
                                m_ctx,             \
                                cmdi->m_devAddr,   \
                                cmdi->m_count,     \
                                (uint16_t *)m_pReadData+cmdi->m_intAddress    \
                                ); 
//                      for( int j=0; i==7 && j<cmdi->m_count; j++ ) cout<<m_pReadData[cmdi->m_intAddress+j]<<" "; cout<<endl;
                        if(cmdi->m_func==103) {     // сохраним защелки и сбросим их в модуле ВВ
                            for(int j=0; j<cmdi->m_count; j++ ) 
                                m_pReadTrigger[j+cmdi->m_intAddress] |= m_pReadData[j+cmdi->m_intAddress];
                            modbus_write_register(m_ctx, cmdi->m_devAddr, 0);    
                        }
                        break;

                    case 4: 
                        rc = modbus_read_input_registers(   \
                                m_ctx,                      \
                                cmdi->m_devAddr,          \
                                cmdi->m_count,            \
                                (uint16_t *)m_pReadData+cmdi->m_intAddress    \
                                );
                        break;
                
 // modbus write commands queue processing
                    case 5:
//                        pthread_mutex_lock( &mutex_param );
                        // write bit if enable==1 or (2 and new<>old)
                        if( (*cmdi).m_first || fLast || cmdi->m_enable==1 ||    \
                            m_pWriteData[cmdi->m_intAddress]!=m_pLastWriteData[cmdi->m_intAddress] ) {
                            rc = modbus_write_bit(      \
                                    m_ctx,              \
                                    cmdi->m_devAddr,  \
                                    (fLast)? 0: m_pWriteData[cmdi->m_intAddress]   \
                                    );
//                            cout<<"func 5 rc="<<rc<<" dev="<<cmdi->m_devAddr<< \
                                " val="<<(fLast? 0: m_pWriteData[cmdi->m_intAddress])<<endl;
                            if(rc!=-1)
                                m_pLastWriteData[cmdi->m_intAddress]=m_pWriteData[(*cmdi).m_intAddress];
                        } 
                        else fTook=false;
//                        pthread_mutex_unlock( &mutex_param );
                        break;

                    case 15:
//                        pthread_mutex_lock( &mutex_param );
                        // write bit if enable==1 or (2 and new<>old)
//                        cout<<"func 15 ";
                        if( cmdi->m_first || fLast || cmdi->m_enable==1 ||    \
                            memcmp(m_pWriteData+cmdi->m_intAddress, \
                                m_pLastWriteData+cmdi->m_intAddress, cmdi->m_count*2) ) {
//                            cout<<" est` task ";
                            uint8_t *tab_value = new uint8_t[cmdi->m_count];
                            for(int16_t j=0;j<cmdi->m_count;j++) {
                                tab_value[j] = fLast? 0: m_pWriteData[cmdi->m_intAddress+j];
//                                cout<<tab_value[j]<<" ";
                            }
                            rc = modbus_write_bits(     \
                                    m_ctx,              \
                                    cmdi->m_devAddr,    \
                                    cmdi->m_count,      \
                                    tab_value           \
                                    );
                            delete []tab_value;
                            if(rc!=-1)
                                memcpy(m_pLastWriteData+cmdi->m_intAddress, \
                                        m_pWriteData+cmdi->m_intAddress, cmdi->m_count*2);                                    
                        } 
                        else fTook=false;
//                        cout<<endl;
//                        pthread_mutex_unlock( &mutex_param );
                        break;

                    case 16:
//                        pthread_mutex_lock( &mutex_param );
                        // write if enable==1 or (2 and new<>old)
                        if( /*cmdi->m_first ||*/ cmdi->m_enable==1 ||    \
                            memcmp(m_pWriteData+cmdi->m_intAddress, \
                                m_pLastWriteData+cmdi->m_intAddress, cmdi->m_count*2) ) {
                            rc = modbus_write_registers(  \
                                    m_ctx,                \
                                    cmdi->m_devAddr,      \
                                    cmdi->m_count,        \
                                    (uint16_t *)m_pWriteData+cmdi->m_intAddress \
                                    );
                            if(i==11) {
//                                cout<<"cmd clear counter FC "<<cmdi->m_devAddr<<endl;
                                for( int j=0; j<cmdi->m_count; j++ ) cout<<m_pLastWriteData[cmdi->m_intAddress+j]<<" "; cout<<endl;
                                for( int j=0; j<cmdi->m_count; j++ ) cout<<m_pWriteData[cmdi->m_intAddress+j]<<" "; cout<<endl;
//                                cout<<"rc="<<rc<<endl;
                            }
                            if(rc!=-1)
                                memcpy(m_pLastWriteData+cmdi->m_intAddress, \
                                        m_pWriteData+cmdi->m_intAddress, cmdi->m_count*2);                    
                        } 
                        else fTook=false;
//                        pthread_mutex_unlock( &mutex_param );
                        break;
                }
                pthread_mutex_unlock( &mutex_param );
            }
            if (rc == -1) {
                if( cmdi->m_errCnt++ >= _max_conn_err ) {
                    cmdi->m_errCnt = _max_conn_err;
                    cmdi->m_err.first   = errno;
                    cmdi->m_err.second  = modbus_strerror(errno);
                    m_pReadData[erroff+i] = (errno>MODBUS_ENOBASE)?errno-MODBUS_ENOBASE:errno;
                }
                outtext(cmdi->ToString());
            }
            else {
//              if(i==9) cout<<"command OK\n";
                if( cmdi->m_errCnt-- <= 0 ) {
                    cmdi->m_errCnt = 0;
                    cmdi->m_err.first   = 0;
                    cmdi->m_err.second  = "no error";
                    m_pReadData[erroff+i] = 0;
                }
            }
            int t;
            t = (cmdi->m_pollInt > 0) ? cmdi->m_pollInt : mincmddel;
            cmdi->m_time.start( (t>0) ? t : 1);   // then set normal poll interval in ms
            cmdi->m_first = false;
        }
        if(fTook) usleep(mincmddel*1000l);
        ++cmdi; ++i;
    }
    if( fConnected ) {
        modbus_set_byte_timeout( m_ctx, to_sec, to_usec );
        modbus_set_response_timeout( m_ctx, old_resp_to_sec, old_resp_to_usec );
        modbus_close(m_ctx);  
        fConnected = false;
    }

//----------- cycle time measure --------------------------------
    clock_gettime(CLOCK_MONOTONIC,&tve);
    nfin = tve.tv_sec*_million + tve.tv_nsec/1000;
    nD = abs(nfin-nsta);
//    cout<<"xchange cycle time "<<nD/1000.0<<" ms"<<endl;
//---------------------------------------------------------------    
   
    return res;
}

