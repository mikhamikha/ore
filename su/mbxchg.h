#ifndef _mbxchg
    #define _mbxchg

#include <string>
#include <stdint.h>
#include <vector>
#include <iterator>
#include <iostream>

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

const char _parities[] = {'N','O','E'};
/*
class ccon {
}
*/
struct ccmd {
    uint8_t     m_enable;       // 0 - disabled, 1 - enabled, conditional write
    uint16_t    m_intAddress;   // offset in m_pReadData [m_pWriteData]
    uint16_t    m_pollInt;      // poll interval in seconds
    uint8_t     m_node;         // slave address of device
    uint8_t     m_func;         // function number
    uint16_t    m_devAddr;      // address in device for read (write) 
    uint8_t     m_count;        // registers number
    uint8_t     m_swap;         // 0 - ABCD, 1 - CDAB, 2 - DCBA, 3 - BADC
    ccmd(ccmd &s)
    {
        m_enable     =  s.m_enable;   
        m_intAddress =  s.m_intAddress;
        m_pollInt    =  s.m_pollInt;  
        m_node       =  s.m_node;     
        m_func       =  s.m_func;     
        m_devAddr    =  s.m_devAddr;  
        m_count      =  s.m_count;    
        m_swap       =  s.m_swap;     
    }
};
//
// Connection class
//
class cmbxchg {
    private:
        uint32_t    m_baud;                     // serial port parameters
        uint8_t     m_bits;
        uint8_t     m_stop;
        char        m_parity;
        std::string m_name;
        uint8_t     m_protocol;                 // connection type TCP, TCP_PI, RTU
        int         m_status;
        uint32_t    m_minCmdDelay;              // delay ms between commands
        modbus_t    *m_ctx;

        std::vector<ccmd> cmds;                 // command list
    
    public:
        cmbxchg(char *name, uint32_t baud, uint8_t bits, uint8_t stop, uint8_t parity, uint8_t proto);
        ~cmbxchg()
        {
            modbus_close(m_ctx);
            modbus_free(m_ctx);
        }   
        
        static uint16_t    *m_pWriteData;      // write data area
        static uint16_t    *m_pReadData;       // read data area
        int16_t init();
        int16_t runCmdCycle();
        int16_t getStatus();
        int16_t terminate();
};

extern std::vector< cmbxchg, std::allocator<cmbxchg> > conn;

#endif
