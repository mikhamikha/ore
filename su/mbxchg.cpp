#include "mbxchg.h"
#include <errno.h>
#include <stdio.h>

//
using namespace std;
fieldconnections conn;
int16_t *cmbxchg::m_pReadData;
int16_t *cmbxchg::m_pWriteData;

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
        cout << ToString() << endl;
    }
std::string ccmd::ToString()
    {
        std::string s;
        char buf[100];
        sprintf(buf,"en=%d intA=%d regCnt=%d poll=%d node=%d func=%d addr=%d swap=%d",
            m_enable,m_intAddress,m_count,m_pollInt,m_node,
            m_func,m_devAddr, m_swap);
        s = buf;
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
        struct timeval response_timeout;
        m_status = INITIALIZED;
        //modbus_set_response_timeout( m_ctx, 0, portproperty("response")._n );
        modbus_get_response_timeout(m_ctx, &response_timeout);
        response_timeout.tv_sec = 0;
        response_timeout.tv_usec = portProperty("response")._n*1000;
        modbus_set_response_timeout(m_ctx, &response_timeout);
        modbus_get_byte_timeout(m_ctx, &response_timeout);
        response_timeout.tv_sec = 0;
        response_timeout.tv_usec = 100000;
        modbus_set_byte_timeout(m_ctx, &response_timeout);
        cout << "init serial | "<<portProperty("path")._s<<" | "<<portProperty("baud")._n<<" | "<< easytoupper(portProperty("parity")._c)<<" | "<<portProperty("databits")._n<<" | "<<portProperty("stop")._n<<" | "<<portProperty("response")._n<<endl;
//    modbus_set_debug(m_ctx, TRUE);
//    modbus_set_error_recovery(m_ctx, MODBUS_ERROR_RECOVERY_LINK | MODBUS_ERROR_RECOVERY_PROTOCOL);
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
        cout << "start xchg " << args << endl;
        mbx->m_pReadData = new int16_t[500];
        mbx->m_pWriteData = new int16_t[500];
        cout << "minimum command delay = " << mbx->portProperty("minimumcommand")._n*1000 << endl;
        while (mbx->getStatus()!=TERMINATE) {
            mbx->runCmdCycle();
        }     
        delete []mbx->m_pReadData;
        delete []mbx->m_pWriteData;    
    }
    cout << "end xchg " << args << endl;
    return EXIT_SUCCESS;
}
//
//  modbus cycle commands
//
int16_t cmbxchg::runCmdCycle() 
{
    int32_t res=0;
    int16_t rc;
    std::vector<ccmd>::iterator cmdi;
    
    cmdi = cmds.begin();
    while(cmdi!=cmds.end()) {
        ccmd cmd = *cmdi;
        if(cmd.m_enable){
            rc=0;
            if (portProperty("proto")._n == RTU) {
                rc = modbus_set_slave(m_ctx, cmd.m_node);
                if (rc==0) rc = modbus_connect(m_ctx);
            }
            if(rc==0) {
//                cout << "cmd " << m_ctx << " | " << cmd.m_node << " | " << cmd.m_func << 
//                        " | " << cmd.m_devAddr << " | " << cmd.m_count << endl;
                switch(cmd.m_func) {
                    case 0x03: 
                        rc = modbus_read_registers(
                                m_ctx, 
                                cmd.m_devAddr, 
                                cmd.m_count, 
                                (uint16_t *)m_pReadData+cmd.m_intAddress
                                ); // здесь потом по желанию добавить чтение float
                        break;
                    case 0x04: 
                        rc = modbus_read_input_registers(
                                m_ctx, 
                                cmd.m_devAddr, 
                                cmd.m_count, 
                                (uint16_t *)m_pReadData+cmd.m_intAddress
                                );
                        break;
                }
            }
            if (rc == -1) {
                cout << modbus_strerror(errno) << endl;
//              fprintf(stderr, "%s\n", modbus_strerror(errno));
//              return -1;
            }
            else {
                cout << "resp = | " << m_pReadData[cmd.m_intAddress] << " | " << m_pReadData[cmd.m_intAddress+1] << " | "<< m_pReadData[cmd.m_intAddress+2] << " | "<< m_pReadData[cmd.m_intAddress+3] << " "<<endl;
               modbus_close(m_ctx);            
            }
        }
//        usleep(portProperty("minimumcommand")._n*1000);
        sleep(1);
        ++cmdi;
    }
    return res;
}

