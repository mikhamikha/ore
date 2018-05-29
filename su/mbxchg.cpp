//#include "utils.h"
#include "main.h"
#include "mbxchg.h"
#include "tag.h"
#include "const.h"
#include <errno.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>

//
using namespace std;
fieldconnections conn;
int16_t *cmbxchg::m_pReadTrigger = NULL;
int16_t *cmbxchg::m_pReadData = NULL;
int16_t *cmbxchg::m_pWriteData = NULL;
int16_t *cmbxchg::m_pLastWriteData = NULL;
int32_t cmbxchg::m_maxReadData;
int32_t cmbxchg::m_maxWriteData;

extern "C" {
    extern int Interface_Segment( int Interface, int pin1, int pin2, int pin3, int pin4, int res );
    extern int gpio_init();
}

ccmd::ccmd(const ccmd &s)
{
    m_enable     = s.m_enable;   
    m_intAddress = s.m_intAddress;
    m_count      = s.m_count;    
    m_pollInt    = s.m_pollInt;  
    m_node       = s.m_node;     
    m_func       = s.m_func;     
    m_devAddr    = s.m_devAddr;  
    m_swap       = s.m_swap;     
    m_err.first  = s.m_err.first;
    m_err.second = s.m_err.second;
    m_first      = s.m_first;
    m_num        = s.m_num;
    m_errCnt     = 0;
}

ccmd::ccmd(std::vector<int32_t> &v)
{
    m_enable     = v[0];
    m_intAddress = v[1]; 
    m_count      = v[2]; 
    m_pollInt    = v[3]; 
    m_node       = v[4]; 
    m_func       = v[5]; 
    m_devAddr    = v[6]; 
    m_swap       = v[7];
    m_num        = v[8];
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
    setproperty( "path",                _sZero );///dev/ttyS2")) );
    setproperty( "enabled",             _nZero );
    setproperty( "protocol",            _nZero );    // пока работаем только RTU
    setproperty( "baudrate",            _nZero );//9600)) );
    setproperty( "parity",              _nZero );
    setproperty( "databits",            _nZero );
    setproperty( "stopbits",            _nZero );
    setproperty( "minimumcommanddelay", _nZero );
    setproperty( "commanderrorpointer", _nZero );
    setproperty( "responsetimeout",     _nZero );
    setproperty( "charTO",              _nZero );
    setproperty( "retrycnt",            _nZero );
    setproperty( "errordelaycntr",      _nZero );
}
//
//  Modbus connection initialization
//
int16_t cmbxchg::init()
{   
    int16_t proto;
    int16_t rc = getproperty("protocol", proto);
    std::string path;
    int16_t     parity, data, stop;
    int32_t     baud=0;

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
                gpio_init();
                Interface_Segment(3, 1, 0, 1, 0, 1);               
                rc = getproperty("path", path)       | \
                     getproperty("baudrate", baud)   | \
                     getproperty("parity", parity)   | \
                     getproperty("databits", data)   | \
                     getproperty("stopbits", stop);
                if(rc == 0) {
                    m_ctx = modbus_new_rtu( path.c_str(), baud, _parities[parity], data, stop );
                    cout << "init serial | "<<path<<" | "<<baud<<" | "<< parity<<" | "<<data<<" | "<< stop;
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
        cout <<" m_ctx= "<<m_ctx<<endl;
//        modbus_set_debug(m_ctx, TRUE);
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
//void* fieldXChange(void *args) 
void cmbxchg::run(){
    if(init() == INITIALIZED) {
        cmbxchg::m_maxReadData = 500;
        cmbxchg::m_maxWriteData= 500;
        if( !cmbxchg::m_pReadData ) {
            cmbxchg::m_pReadData        = new int16_t[cmbxchg::m_maxReadData+1];
            cmbxchg::m_pReadTrigger     = new int16_t[cmbxchg::m_maxReadData+1];
            cmbxchg::m_pWriteData       = new int16_t[cmbxchg::m_maxWriteData+1];
            cmbxchg::m_pLastWriteData   = new int16_t[cmbxchg::m_maxWriteData+1];
            memset(cmbxchg::m_pReadData,      0, (cmbxchg::m_maxReadData+1)*sizeof(int16_t) );
            memset(cmbxchg::m_pReadTrigger,   0, (cmbxchg::m_maxReadData+1)*sizeof(int16_t) );
            memset(cmbxchg::m_pWriteData,     0, (cmbxchg::m_maxWriteData+1)*sizeof(int16_t) );
            memset(cmbxchg::m_pLastWriteData, 0, (cmbxchg::m_maxWriteData+1)*sizeof(int16_t) );
        }
        cout << "start xchg num = " << m_id << " | " << cmbxchg::m_pReadData <<endl;
        while( getStatus()!=TERMINATE ) {
            runCmdCycle(false);
        }     
        runCmdCycle(true);
        if( !cmbxchg::m_pReadData ) {
            delete []cmbxchg::m_pReadData;
            delete []cmbxchg::m_pReadTrigger;
            delete []cmbxchg::m_pWriteData;    
            delete []cmbxchg::m_pLastWriteData;    
        }
    }
    cout << "end xchg " << m_id << endl;
//    return EXIT_SUCCESS;
}
//
//  modbus cycle commands
//
int16_t cmbxchg::runCmdCycle(bool fLast=false) 
{
    int32_t                 res=0;
    int16_t                 rc=_exFail, i;
    mbcommands::iterator    cmdi;
    uint32_t                old_resp_to_sec;
    uint32_t                old_resp_to_usec;
    uint32_t                to_sec;
    uint32_t                to_usec;
    bool                    fTook;              // delay must be
    int16_t                 proto, erroff;
    int32_t                 resp, charto, mincmddel, errdel;
    bool                    fConnected = false;
//----- cycle time measure-------------------------------------
//    timespec    tvs, tve;
//    int32_t     nTime;
//    int64_t     nsta, nfin;
//    int64_t     nD;

//    clock_gettime(CLOCK_MONOTONIC,&tvs);
//    nsta = tvs.tv_sec*_million + tvs.tv_nsec/1000;

//---------------------------------------------------------------    
    cmdi = cmds.begin(); i=0;
    rc = getproperty( "protocol", proto )               | \
         getproperty( "responseto", resp )             | \
         getproperty( "minimumCmdDelay", mincmddel )  | \
         getproperty( "charTO", charto )  | \
         getproperty( "errordelay", errdel )         | \
         getproperty( "cmderroffs", erroff );
    

    if(rc==_exOK) {
        rc = modbus_connect(m_ctx);
        if(!rc) {
            modbus_get_response_timeout( m_ctx, &old_resp_to_sec, &old_resp_to_usec );
            modbus_get_byte_timeout( m_ctx, &to_sec, &to_usec );
            rc = modbus_set_response_timeout( m_ctx, floor(resp/1000), 1000*(resp%1000) );
            rc = modbus_set_byte_timeout( m_ctx, floor(charto/1000), 1000*(charto%1000));
            if(!rc) fConnected = true;
        }
    }
/*    
    cout<<dec<<"xchgId="<<pthread_self()<<" proto="<<proto<<" response="<<floor(resp/1000)<<","<<1000*(resp%1000)
        <<" charTO="<<floor(charto/1000)<<","<<1000*(charto%1000)<<" minCmdDelay="<<mincmddel 
        <<" errordelay="<<errdel<<" commanderror="<<erroff<<" fConn="<<fConnected<<endl;
*/    
    if( rc == 0)
    while(cmdi!=cmds.end()) {
        if(i==900) {
            cout<<"cmd="<<i<<" n="<<(*cmdi).m_node<<" f="<<(*cmdi).m_func<<" en="<<(*cmdi).m_enable<<" fi="<<(*cmdi).m_first<<endl;
            outtext(cmdi->ToString());
        }
        fTook = true; 
        if( cmdi->m_enable && ( cmdi->m_first || cmdi->m_time.isDone() ) ) {
            rc = 0;
            if(proto == RTU) {
                rc = modbus_set_slave(m_ctx, cmdi->m_node);
            }   
            if(rc==0) {
//                pthread_mutex_lock( &mutex_tag );
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
//                      for( int j=0; i==7 && j<cmdi->m_count; j++ ) 
//                                cout<<m_pReadData[cmdi->m_intAddress+j]<<" "; cout<<endl;
                        if( cmdi->m_func==103 && rc!=-1 ) {     // сохраним защелки и сбросим их в модуле ВВ
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
                        pthread_mutex_lock( &mutex_tag );
                        // write bit if enable==1 or (2 and new<>old)
                        if( (*cmdi).m_first || fLast || cmdi->m_enable==1 ||    \
                            m_pWriteData[cmdi->m_intAddress]!=m_pLastWriteData[cmdi->m_intAddress] ) {
                            rc = modbus_write_bit(      \
                                    m_ctx,              \
                                    cmdi->m_devAddr,  \
                                    (fLast)? 0: m_pWriteData[cmdi->m_intAddress]   \
                                    );
//                            cout<<"func 5 rc="<<rc<<" dev="<<cmdi->m_devAddr<< 
//                                " val="<<(fLast? 0: m_pWriteData[cmdi->m_intAddress])<<endl;
                            if(rc!=-1)
                                m_pLastWriteData[cmdi->m_intAddress]=m_pWriteData[(*cmdi).m_intAddress];
                        } 
                        else fTook=false;
                        pthread_mutex_unlock( &mutex_tag );
                        break;

                    case 15:
                        pthread_mutex_lock( &mutex_tag );
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
                        pthread_mutex_unlock( &mutex_tag );
                        break;

                    case 16:
                        pthread_mutex_lock( &mutex_tag );
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
//                                for( int j=0; j<cmdi->m_count; j++ ) cout<<m_pLastWriteData[cmdi->m_intAddress+j]<<" "; cout<<endl;
//                                for( int j=0; j<cmdi->m_count; j++ ) cout<<m_pWriteData[cmdi->m_intAddress+j]<<" "; cout<<endl;
//                                cout<<"rc="<<rc<<endl;
                            }
                            if(rc!=-1)
                                memcpy(m_pLastWriteData+cmdi->m_intAddress, \
                                        m_pWriteData+cmdi->m_intAddress, cmdi->m_count*2);                    
                        } 
                        else fTook=false;
                        pthread_mutex_unlock( &mutex_tag );
                        break;
                }
//                pthread_mutex_unlock( &mutex_tag );
            }
            if (rc == -1) {
                if( cmdi->incErr() >= _max_conn_err ) {
                    m_pReadData[erroff+cmdi->m_num] = (errno>MODBUS_ENOBASE)?errno-MODBUS_ENOBASE:errno;
                }
                outtext(cmdi->ToString());
//                modbus_flush( m_ctx );
            }
            else if(fTook) {
//              if(i==9) cout<<"command OK\n";
                if( cmdi->decErr() <= 0 ) {
                    m_pReadData[erroff+cmdi->m_num] = 0;
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
//    clock_gettime(CLOCK_MONOTONIC,&tve);
//    nfin = tve.tv_sec*_million + tve.tv_nsec/1000;
//    nD = abs(nfin-nsta);
//    if( m_id==2 )
//        cout<<"\nxchange "<<m_id<<" cycle time "<<nD/1000.0<<" ms"<<endl;
//---------------------------------------------------------------    
   
    return res;
}

