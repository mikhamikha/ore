// заголовочный файл algo.h
#ifndef _unit_H
	#define _unit_H

#include <numeric>
#include <string>	
#include <algorithm>
#include <vector>
#include <ctime>
#include <stdint.h>
#include <stdlib.h>
#include "main.h"
#include "tag.h"

#define _loc_data_buffer_size   10
#define _valve_mode_amount      8 
#define _lag_start              2000
#define _waitcomplete           500000

#define _cnt        m_pfc->getvalue()           // текущий счетчик
#define _cnt_old    m_pfc->getoldvalue()        // предыдущий счетчик
#define _lso        m_plso->getvalue()          // конечный открытия
#define _lso_old    m_plso->getoldvalue()       // конечный открытия
#define _lsc        m_plsc->getvalue()          // конечный закрытия
#define _lsc_old    m_plsc->getoldvalue()       // конечный закрытия
#define _cmd_tsk    m_pcmdopen->gettask(true)   // команда движения
#define _dir_tsk    m_pcmdclose->gettask(true)  // команда обратного движения
#define _cmd_val    m_pcmdopen->getvalue()      // подтв. команда движения
#define _dir_val    m_pcmdclose->getvalue()     // подтв. команда обратного движения
#define _raw        m_ppos->getrawval()         // текущее "сырое" значение положения
#define _task       m_ppos->gettask()
#define _value      m_ppos->getvalue()
#define _maxOpen    m_ppos->getmaxeng()
#define _minOpen    m_ppos->getmineng()
#define _sw         m_psel->getvalue()                   
#define _mode       m_pmod->getvalue()
#define _mode_old   m_pmod->getoldvalue()
//#define _pt         m_preg->getvalue();
//#define _pttask     m_preg->gettask();   

enum motionstate {
    _no_motion  = 0,        // клапан бездвижен
    _open       = 1,        // дана команда открытия
    _close,                 // дана команда закрытия
    _closeAck,              // есть подтверждение готовности закрытия
    _stopping,              // процесс останова
    _opening    = 11,       // идет открытие
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

enum unit_type {
    _none,
    _valve,
    _pump_station
};

enum valve_error_codes {
    _no_error,
    _2_ls,
    _timeout
};

enum valvecmd {
    _cmd_none,
    _cmd_open,
    _cmd_close,
    _cmd_position,
    _cmd_stop
};

enum valvecmdstatus {
    _cmd_clear,
    _cmd_accepted,
    _cmd_completed,
    _cmd_failed
};

typedef std::vector <cproperties> vlvmode;
typedef std::vector<ctag*> tagvector;

;// объявление класса Алгоритм
class cunit: public cproperties { 			// имя класса
    tagvector   args;
    int16_t     m_nType;
    int16_t     m_num;   
    int16_t     m_fInit; 
    cton        m_twait;     
    cton        m_tcmd;
    string      m_name;
    int16_t     m_status;
    int16_t     m_direction;
    int32_t     m_pulsew;
    double      m_task;
    int16_t     m_cmd;
    int16_t     m_lastcmd;
    int16_t     m_cmdstatus;

    union {
        struct {            // valve
            ctag*   m_plso;             // конечный открытия           
            ctag*   m_plsc;             // конечный закрытия
            ctag*   m_pfc;              // текущий счетчик
            ctag*   m_ppos;             // положение клапана
            ctag*   m_pmod;             // режим клапана
            ctag*   m_psel;             // выбор клапана
            ctag*   m_pcmdopen;         // команда движения            
            ctag*   m_pcmdclose;        // команда обратного движения            
            ctag*   m_pblock;           // ссылка на блокирующий параметр
            ctag*   m_preg;             // ссылка на регулируемый параметр
            int16_t m_motion;           // движение клапана motionstate
            double  m_delta;            // дельта для остановки клапана
            double  m_maxdelta;         // максимальная дельта
            int16_t m_error;            // ошибка клапана valve_error_codes
        };
        struct {            // pump
        };
    };
    struct {
        double  kp;
        double  ki;
        double  kd;
        double  period;
        double  out;
        double  sp;
        double  err1;
        double  err2;
        double  dead;
        int16_t type;
        cton    timer;
        bool    enable;
    } m_pid;

    public:
    cunit();   
    int16_t init();
    string  getname() { return m_name; }

    int16_t getstate( void );
    int16_t control( void );
    int16_t argsize( void ) { return args.size(); }
    int16_t gettype() { return m_nType; }
    int16_t getmode() { return m_pmod->getvalue(); }
    int16_t getmotion() { return m_motion; }
//    int16_t task(double rval) { n_task = rval; }
//    double  taskget() { return n_task; } 
    int16_t stop();
    void    pidEval( void* );
    bool    valveCtrlEnabled() {
        return (_sw==_no_valve || _sw==m_num);
    }
    bool isOpened() { return _lso; }
    bool isClosed() { return _lsc; }
};

extern vlvmode vmodes;

#endif
