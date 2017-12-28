#ifndef _CONST_H_ORE_
#define _CONST_H_ORE_


/* module OPCDA_Qualities */
const uint8_t OPC_QUALITY_MASK                     = 0xc0;
const uint8_t OPC_STATUS_MASK                      = 0xfc;
const uint8_t OPC_LIMIT_MASK                       = 0x03;
const uint8_t OPC_QUALITY_BAD                      = 0x00;
const uint8_t OPC_QUALITY_UNCERTAIN                = 0x40;
const uint8_t OPC_QUALITY_GOOD                     = 0xc0;
const uint8_t OPC_QUALITY_CONFIG_ERROR             = 0x04;
const uint8_t OPC_QUALITY_NOT_CONNECTED            = 0x08;
const uint8_t OPC_QUALITY_DEVICE_FAILURE           = 0x0c;
const uint8_t OPC_QUALITY_SENSOR_FAILURE           = 0x10;
const uint8_t OPC_QUALITY_LAST_KNOWN               = 0x14;
const uint8_t OPC_QUALITY_COMM_FAILURE             = 0x18;
const uint8_t OPC_QUALITY_OUT_OF_SERVICE           = 0x1c;
const uint8_t OPC_QUALITY_WAITING_FOR_INITIAL_DATA = 0x20;
const uint8_t OPC_QUALITY_LAST_USABLE              = 0x44;
const uint8_t OPC_QUALITY_SENSOR_CAL               = 0x50;
const uint8_t OPC_QUALITY_EGU_EXCEEDED             = 0x54;
const uint8_t OPC_QUALITY_SUB_NORMAL               = 0x58;
const uint8_t OPC_QUALITY_LOCAL_OVERRIDE           = 0xd8;
const uint8_t OPC_LIMIT_OK                         = 0x00;
const uint8_t OPC_LIMIT_LOW                        = 0x01;
const uint8_t OPC_LIMIT_HIGH                       = 0x02;
const uint8_t OPC_LIMIT_CONST                      = 0x03;


enum exit_codes {
    _exOK = 0,      
    _exFail,        // =1
    _exBadParam,    // =2
    _exBadAddr,     // =3
    _exBadDippedHW, // =4
    _exBadIO        // =5
};                  

#endif // CONST_H_ORE_
