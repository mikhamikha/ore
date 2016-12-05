#include "mbxchg.h"


using namespace std;
vector< cmbxchg, allocator<cmbxchg> > conn;

cmbxchg::cmbxchg(char *name, uint32_t baud, uint8_t bits, uint8_t stop, uint8_t parity):m_name(name),m_baud(baud),m_bits(bits),m_stop(stop)
{
    m_status = INITIALIZED;
    m_protocol = RTU;                   // пока работаем только RTU

    if (parity>2 && parity<1)
        m_parity = _parities[0];
    else
        m_parity = _parities[parity];

    if (m_protocol == TCP) {
        m_ctx = modbus_new_tcp("127.0.0.1", 1502);              
    } else  if (m_protocol == TCP_PI) {
                m_ctx = modbus_new_tcp_pi("::1", "1502");
            }   else {
                    m_ctx = modbus_new_rtu(m_name.c_str(), m_baud, m_parity, m_bits, m_stop);
                }

    if (m_ctx == NULL) {
        m_status = INIT_ERR;
//        fprintf(stderr, "Unable to allocate libmodbus context\n");
//        return -1;
    } 
    else {
//    modbus_set_debug(m_ctx, TRUE);
//    modbus_set_error_recovery(m_ctx, MODBUS_ERROR_RECOVERY_LINK | MODBUS_ERROR_RECOVERY_PROTOCOL);
    }
}

//
// поток обмена по Modbus с полевым оборудованием
//
void* fieldXChange(void *args) 
{
    cmbxchg *mbx=(cmbxchg *)(args);
    while (1) {
        mbx->runCmdCycle();
    }
}

//
//
//
int32_t cmbxchg::runCmdCycle() 
{
    int32_t res=0;
    int16_t rc;
    std::vector<ccmd>::iterator cmdi;
    
    cmdi = cmds.begin();
    while(cmdi!=cmds.end()) {
        ccmd cmd = *cmdi;
        if(cmd.m_enable){
            rc=0;
            if (m_protocol == RTU) {
                rc = modbus_set_slave(m_ctx, cmd.m_node);
                if (rc==0) rc = modbus_connect(m_ctx);
            }
            if(rc==0) {
                switch(cmd.m_func) {
                    case 0x03: rc = modbus_read_registers(m_ctx, cmd.m_devAddr, cmd.m_count, m_pReadData); // здесь потом по желанию добавить чтение float
                        break;
                    case 0x04: rc = modbus_read_input_registers(m_ctx, cmd.m_devAddr, cmd.m_count, m_pReadData);
                        break;
                }
            }
            if (rc == -1) {
//              fprintf(stderr, "%s\n", modbus_strerror(errno));
//              return -1;
            }
            else modbus_close(m_ctx);            
        }
        usleep(m_minCmdDelay);
        ++cmdi;
    }
    return res;
}
