#ifndef _MAIN_H
    #define _MAIN_H

#include <time.h>
#include <stdint.h>
#include <iostream>
#include <iomanip>
#include <string>
#include <ctype.h>
#include "utils.h"

using namespace std;

#ifndef __OPCDA_Qualities_MODULE_DEFINED__
#define __OPCDA_Qualities_MODULE_DEFINED__


/* module OPCDA_Qualities */


const uint16_t OPC_QUALITY_MASK                     =   0xc0;
const uint16_t OPC_STATUS_MASK                      =   0xfc;
const uint16_t OPC_LIMIT_MASK                       =   0x03;
const uint16_t OPC_QUALITY_BAD                      =   0x00;
const uint16_t OPC_QUALITY_UNCERTAIN                =   0x40;
const uint16_t OPC_QUALITY_GOOD                     =   0xc0;
const uint16_t OPC_QUALITY_CONFIG_ERROR             =   0x04;
const uint16_t OPC_QUALITY_NOT_CONNECTED            =   0x08;
const uint16_t OPC_QUALITY_DEVICE_FAILURE           =   0x0c;
const uint16_t OPC_QUALITY_SENSOR_FAILURE           =   0x10;
const uint16_t OPC_QUALITY_LAST_KNOWN               =   0x14;
const uint16_t OPC_QUALITY_COMM_FAILURE             =   0x18;
const uint16_t OPC_QUALITY_OUT_OF_SERVICE           =   0x1c;
const uint16_t OPC_QUALITY_WAITING_FOR_INITIAL_DATA =   0x20;
const uint16_t OPC_QUALITY_LAST_USABLE              =   0x44;
const uint16_t OPC_QUALITY_SENSOR_CAL               =   0x50;
const uint16_t OPC_QUALITY_EGU_EXCEEDED             =   0x54;
const uint16_t OPC_QUALITY_SUB_NORMAL               =   0x58;
const uint16_t OPC_QUALITY_LOCAL_OVERRIDE           =   0xd8;
const uint16_t OPC_LIMIT_OK                         =   0x00;
const uint16_t OPC_LIMIT_LOW                        =   0x01;
const uint16_t OPC_LIMIT_HIGH                       =   0x02;
const uint16_t OPC_LIMIT_CONST                      =   0x03;

#endif /* __OPCDA_Qualities_MODULE_DEFINED__ */

#endif
