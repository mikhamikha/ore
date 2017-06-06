// заголовочный файл algo.h
#ifndef _ALGO_H
	#define _ALGO_H

#include <functional>
#include <numeric>
#include <string>	
#include <algorithm>
#include <vector>
#include <ctime>
#include <stdint.h>
#include <stdlib.h>
#include "main.h"

#define     _loc_data_buffer_size   10

enum algtype {
    _valveEval = 1,                 // Расчет положения клапана
    _valveProcessing = 2,           // Управление клапаном   
    _valveCalibrate,
    _floweval=10,                   // Расчет расхода по перепаду давления на сечении клапана 
    _summ=11,                       // Суммирование входных аргументов и запись в выходной
    _sub=12                         // Вычитание входных аргументов и запись в выходной
};

enum motionstate {
    _no_motion = 0,
    _open   = 1,
    _close,
    _opening = 11,
    _closing
};

enum modecontrol {
    _not_proc = 0,
    _manual,
    _manual_pulse,
    _auto_press = 3,
    _auto_diff,
    _auto_time
};

typedef std::vector <cproperties> arglist;
typedef std::vector<cparam*> paramvector;

// объявление класса Алгоритм
class calgo: public cproperties { 			// имя класса
    paramvector args;
    paramvector res;
    int16_t     nType;
//    double      rdata[_loc_data_buffer_size];
    int16_t     fInit; 
    cton        m_twait;     
    cton        m_tcmd;
    
    public:
    calgo() { fInit = 0; }  
    int16_t init();
    int16_t solveIt( void );
    int16_t argsize( void ) { return args.size(); }
    int16_t ressize( void ) { return res.size(); }
};

typedef std::vector <calgo*> alglist;

extern alglist algos;

#endif
