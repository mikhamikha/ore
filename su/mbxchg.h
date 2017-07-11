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
#include <errno.h>

//#define EXIT_SUCCESS    0
//#define EXIT_FAILURE    1

#define _max_conn_err   3

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
        int16_t     m_enable;       // 0 - disabled, 1 - enabled, 2 - conditional write
        int16_t     m_intAddress;   // offset in m_pReadData [m_pWriteData]
        int16_t     m_pollInt;      // poll interval in seconds
        int16_t     m_node;         // slave address of device
        int16_t     m_func;         // function number
        int16_t     m_devAddr;      // address in device for read (write) 
        int16_t     m_count;        // registers number
        int16_t     m_swap;         // 0 - ABCD, 1 - CDAB, 2 - DCBA, 3 - BADC
        int16_t     m_num;
        bool        m_first;        // first scan
        int16_t     m_errCnt;
        std::pair<uint16_t,std::string> m_err;
        ccmd(const ccmd &s);
        ccmd(std::vector<int16_t> &v);
        std::string ToString();
        cton       m_time;
        int16_t incErr() {
            m_errCnt += ((m_errCnt < _max_conn_err) ? 1 : 0);
            m_err.first  = errno;
            m_err.second = modbus_strerror(errno);
            return m_errCnt;
        }
        int16_t decErr() {
             m_errCnt -= ((m_errCnt > 0) ? 1 : 0);
             if(!m_errCnt) {
                m_err.first   = 0;
                m_err.second  = "no error";            
             }
             return m_errCnt;
        }
};

const char _parities[] = {'N','O','E'};
typedef std::vector<ccmd> mbcommands;

//
// Connection class
//
class cmbxchg: public cproperties, public cthread {
        int16_t             m_status;
        modbus_t            *m_ctx;
        mbcommands          cmds;                 // command list

    public:
        int32_t             m_id;
        cmbxchg();
        ~cmbxchg()
        {
            modbus_close(m_ctx);
            modbus_free(m_ctx);
        }   
        int32_t mbCommandsCount() {return cmds.size();}
        ccmd* mbCommand(const int i) {return &cmds[i];}
        void mbCommandAdd(ccmd &cmd) { cmds.push_back(cmd); }
        static  int16_t    *m_pWriteData;       // write data area
        static  int16_t    *m_pLastWriteData;   // write data area
        static  int16_t    *m_pReadData;        // read data area
        static  int16_t    *m_pReadTrigger;     // read data area
        static int32_t     m_maxReadData;
        static int32_t     m_maxWriteData;
        int16_t init();
        int16_t runCmdCycle(bool);
        int16_t getStatus();
        int16_t terminate();
        void run();
};

typedef std::vector< cmbxchg * > fieldconnections;
extern fieldconnections conn;

#endif

