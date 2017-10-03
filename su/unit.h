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

#define _cnt     m_pfc->getvalue()          // текущий счетчик
#define _cnt_old m_pfc->getoldvalue()       // предыдущий счетчик
#define _lso     m_plso->getvalue()         // конечный открытия
#define _lso_old m_plso->getoldvalue()      // конечный открытия
#define _lsc     m_plsc->getvalue()         // конечный закрытия
#define _lsc_old m_plsc->getoldvalue()      // конечный закрытия
#define _cmd_tsk m_pcmdopen->gettask(true)  // команда движения
#define _dir_tsk m_pcmdclose->gettask(true) // команда обратного движения
#define _raw     m_ppos->getrawval()        // текущее "сырое" значение положения
#define _task    m_ppos->gettask()
#define _value   m_ppos->getvalue()
#define _maxOpen m_ppos->getmaxeng()
#define _minOpen m_ppos->getmineng()
#define _sw      m_psel->getvalue();                   
#define _mode       m_pmod->getvalue();
#define _mode_old   m_pmod->getoldvalue();
//#define _pt         m_preg->getvalue();
//#define _pttask     m_preg->gettask();   

enum algtype {
    _valveEval = 1,         // Расчет положения клапана
    _valveProcessing = 2,   // Управление клапаном   
    _valveCalibrate,        // Калибровка клапана
    _timeValveControl,      // Управление клапаном по времени
    _pidValveControl,       // Управление клапаном по ПИД
    _floweval=10,           // Расчет расхода по перепаду P на сечении клапана 
    _summ=11,               // Суммирование входных аргументов и запись в выходной
    _sub=12                 // Вычитание входных аргументов и запись в выходной
};

enum motionstate {
    _no_motion = 0,         // клапан бездвижен
    _open   = 1,            // команда открытия
    _close,                 // команда закрытия
    _opening = 11,          // идет открытие
    _closing                // идет закрытие
};

enum modecontrol {
    _not_proc = 0,          // отключен
    _manual,                // ручное управление
    _auto_pid,              // ПИД 
    _auto_time,             // автоматический по времени
    _calibrate,             // режим калибровки
    _manual_pulse_open,     // импульс на открытие
    _manual_pulse_close,    // импульс на закрытие
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

enum unit_type {
    _none,
    _valve,
    _pump_station
};

typedef std::vector <cproperties> vlvmode;
//typedef std::vector <cproperties> arglist;
typedef std::vector<ctag*> tagvector;

// объявление класса Алгоритм
class cunit: public cproperties { 			// имя класса
    tagvector   args;
    int16_t     m_ntype;
    int16_t     m_num;   
    int16_t     fInit; 
    cton        m_twait;     
    cton        m_tcmd;
    string      m_name;
    int16_t     m_status;

    union {
        struct {            // valve
            ctag*   m_plso;
            ctag*   m_plsc;
            ctag*   m_pfc;
            ctag*   m_ppos;
            ctag*   m_pmod;
            ctag*   m_psel;
            ctag*   m_pcmdopen;
            ctag*   m_pcmdclose;
            ctag*   m_pblock;
            ctag*   m_preg;
        };
        struct {            // pump
        };
    };
    public:
    cunit() { fInit = 0; }  
    int16_t init();
//    int16_t 
    int16_t getstate( void );
    int16_t control( void );
    int16_t argsize( void ) { return args.size(); }
};

typedef std::vector <cunit*> unitlist;

extern unitlist units;
extern vlvmode vmodes;

#endif
