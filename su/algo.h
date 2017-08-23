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

#define _loc_data_buffer_size   10
#define _valve_mode_amount      8 

enum algtype {
    _valveEval = 1,                 // Расчет положения клапана
    _valveProcessing = 2,           // Управление клапаном   
    _valveCalibrate,                // Калибровка клапана
    _timeValveControl,              // Управление клапаном по времени
    _pidValveControl,               // Управление клапаном по ПИД
    _floweval=10,                   // Расчет расхода по перепаду давления на сечении клапана 
    _summ=11,                       // Суммирование входных аргументов и запись в выходной
    _sub=12                         // Вычитание входных аргументов и запись в выходной
};

enum motionstate {
    _no_motion = 0,                 // клапан бездвижен
    _open   = 1,                    // команда открытия
    _close,                         // команда закрытия
    _opening = 11,                  // идет открытие
    _closing                        // идет закрытие
};

enum modecontrol {
    _not_proc = 0,                  // отключен
    _manual,                        // ручное управление
    _auto_pid,                      // ПИД 
    _auto_time,                     // автоматический по времени
    _calibrate,                     // режим калибровки
    _manual_pulse_open,//=11,       // импульс на открытие
    _manual_pulse_close,            // импульс на закрытие
};

enum valvenum {
    _no_valve=0,
    _first_v,
    _second_v,
    _mask_v
};

enum state_algo {
    _idle_a,
    _buzy_a,
    _done_a
};

typedef std::vector <cproperties> vlvmode;
//typedef std::vector <cproperties> arglist;
typedef std::vector<ctag*> tagvector;

// объявление класса Алгоритм
class calgo: public cproperties { 			// имя класса
    tagvector args;
    tagvector res;
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
extern vlvmode vmodes;

#endif
