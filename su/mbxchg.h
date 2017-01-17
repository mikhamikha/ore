#ifndef _mbxchg
    #define _mbxchg

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
#include "param.h"
#include "utils.h"

#define EXIT_SUCCESS    0
#define EXIT_FAILURE    1

//
// add libmodbus
//
extern "C" {
    #include <modbus.h>
}
//
// enum of supported protocols
//
enum {
    TCP,
    TCP_PI,
    RTU
}; 


class ccmd {
    public:
        int16_t    m_enable;       // 0 - disabled, 1 - enabled, 2 - conditional write
        int16_t    m_intAddress;   // offset in m_pReadData [m_pWriteData]
        int16_t    m_pollInt;      // poll interval in seconds
        int16_t    m_node;         // slave address of device
        int16_t    m_func;         // function number
        int16_t    m_devAddr;      // address in device for read (write) 
        int16_t    m_count;        // registers number
        int16_t    m_swap;         // 0 - ABCD, 1 - CDAB, 2 - DCBA, 3 - BADC
        bool       m_first;        // first scan
        std::pair<uint16_t,std::string> m_err;
        ccmd(const ccmd &s);
        ccmd(std::vector<int16_t> &v);
        std::string ToString();
        cton       m_time;
};

const char _parities[] = {'N','O','E'};
//typedef std::vector<std::pair<std::string, content> > portsettings;
typedef std::vector<ccmd> mbcommands;

//
// Connection class
//
class cmbxchg {

        int16_t             m_status;
        modbus_t            *m_ctx;
        settings            ps;                   // serial port settings
        mbcommands          cmds;                 // command list

    public:
        int32_t             m_id;
        int32_t             m_maxReadData;
        int32_t             m_maxWriteData;
        cmbxchg();
        ~cmbxchg()
        {
            modbus_close(m_ctx);
            modbus_free(m_ctx);
        }   
        int32_t portPropertyCount() {return ps.size();} 
        content& portProperty(const char *); 
        content& portProperty(const int); 
        std::string portProperty2Text(const int i);
        int32_t portPropertySet(const char *, content& ); 
        int32_t mbCommandsCount() {return cmds.size();}
        ccmd* mbCommand(const int i) {return &cmds[i];}
        void mbCommandAdd(ccmd &cmd) { cmds.push_back(cmd); }
        static  int16_t    *m_pWriteData;       // write data area
        static  int16_t    *m_pLastWriteData;   // write data area
        static  int16_t    *m_pReadData;        // read data area
        int16_t init();
        int16_t runCmdCycle(bool);
        int16_t getStatus();
        int16_t terminate();
};

struct equalID 
{
    int32_t _i;
    equalID(int32_t i):_i(i) { }
    
    bool operator () (cmbxchg *p) {
        return (p->m_id==_i);
    }
};

typedef std::vector< cmbxchg * > fieldconnections;
extern fieldconnections conn;

#endif

