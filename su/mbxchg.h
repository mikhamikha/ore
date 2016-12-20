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
#include "param.h"

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

enum {
    INITIALIZED,
    INIT_ERR,
    TERMINATE
};

struct content {
    content(int32_t n) : _n(n) { _t=0; }
    content(char c) : _c(c) {_t=1; }
    content(std::string s) : _s(s) { 
        _t = 2;
        _c = s[0];
        _n = atoi(s.c_str());
    }
    int32_t     _n;
    std::string _s;
    char        _c;
    int8_t      _t;
};

struct compContent
{
    std::string _s;
    compContent(std::string const& s) {
        _s = s;
        std::transform(_s.begin(), _s.end(), _s.begin(), easytolower);
    }
    bool operator () (std::pair<std::string, content> const& p) {
        std::string tmp = p.first;
        std::transform(tmp.begin(), tmp.end(), tmp.begin(), easytolower);
        return (tmp.find(_s)==0);
    }
};

struct ccmd {
    int16_t    m_enable;       // 0 - disabled, 1 - enabled, 2 - conditional write
    int16_t    m_intAddress;   // offset in m_pReadData [m_pWriteData]
    int16_t    m_pollInt;      // poll interval in seconds
    int16_t    m_node;         // slave address of device
    int16_t    m_func;         // function number
    int16_t    m_devAddr;      // address in device for read (write) 
    int16_t    m_count;        // registers number
    int16_t    m_swap;         // 0 - ABCD, 1 - CDAB, 2 - DCBA, 3 - BADC
    std::pair<uint16_t,std::string> m_err;
    ccmd(const ccmd &s);
    ccmd(std::vector<int16_t> &v);
    std::string ToString();
};

const char _parities[] = {'N','O','E'};
typedef std::vector<std::pair<std::string, content> > portsettings;
typedef std::vector<ccmd> mbcommands;

//
// Connection class
//
class cmbxchg {

        int                 m_status;
        modbus_t            *m_ctx;
        portsettings        ps;                   // serial port settings
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
        static  int16_t    *m_pReadData;        // read data area
        int16_t init();
        int16_t runCmdCycle();
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

