#include "main.h"
//
//  valve object construcror by tags names
//  lso, lsc - limit switches open|close
//  cmdo, cmdc - commands open|close
//  fc - Hall sensor counter
//  fv - control valve
//
cvalue( string& lso, string& lsc, string& cmdo, string& cmdc, string& fc, string& fv ) {
    pfv   = getparam( fv.c_str()   );  // FV
    pfc   = getparam( fc.c_str()   );  // FC
    plso  = getparam( lso.c_str()  );  // ZV opened
    plsc  = getparam( lsc.c_str()  );  // ZV closed
    pcmdo = getparam( cmdo.c_str() );  // CV open cmd
    pcmdc = getparam( cmdc.c_str() );  // CV close cmdi
}


