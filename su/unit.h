// заголовочный файл algo.h
#ifndef _UNIT_HPP_
	#define _UNIT_HPP_

#include <numeric>
#include <string>	
#include <algorithm>
#include <vector>
#include <ctime>
#include <stdint.h>
#include <stdlib.h>

#include "tag.h"
#include "utils.h"

#define _loc_data_buffer_size   10
#define _valve_mode_amount      8 
#define _lag_start              10000
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
//#define _task       m_ppos->gettask()
//#define _value      m_ppos->getvalue()
#define _maxOpen    m_ppos->getmaxeng()
#define _minOpen    m_ppos->getmineng()
#define _sw         m_psel->getvalue()                   
//#define _mode       m_pmod->getvalue()
//#define _mode_old   m_pmod->getoldvalue()
//#define _pt         m_preg->getvalue();
//#define _pttask     m_preg->gettask();   

enum motionstate {
    _no_motion  = 0,        // клапан бездвижен
    _open       = 1,        // дана команда открытия
    _close,                 // дана команда закрытия
    _closeAck,              // есть подтверждение готовности закрытия
    _opening    = 11,       // идет открытие
    _closing,               // идет закрытие
    _stopping               // процесс останова
};

enum modecontrol {
    _not_proc = 0,          // отключен
    _manual,                // ручное управление
    _auto_pid,              // ПИД 
//    _auto_time,             // автоматический по времени
//    _calibrate,             // режим калибровки
    _manual_pulse_open,     // импульс на открытие
    _manual_pulse_close     // импульс на закрытие
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
    _cmd_none = 0,
    _cmd_open,      // =1
    _cmd_close,     // =2
    _cmd_position,  // =3
    _cmd_stop       // =4
};

enum valvecmdstatus {
    _cmd_clear = 0,
    _cmd_requested, // =1
    _cmd_accepted,  // =2
    _cmd_completed, // =3
    _cmd_failed     // =4
};

enum valvestatus {
    _vlv_ready=0,
    _vlv_opened,    // =1
    _vlv_closed,    // =2
    _vlv_open,      // =3
    _vlv_opening,   // =4
    _vlv_close,     // =5    
    _vlv_closing,   // =6
    _vlv_warn,      // =7
    _vlv_fault_1,   // =8
    _vlv_fault_2,   // =9
    _vlv_fault_3,   // =10
    _vlv_fault_4,   // =11
    _vlv_override   // =12
};

enum pumpstatus {
    _p_ready=0,
    _p_running,     // =1
    _p_start,       // =2
    _p_stop,        // =3
    _p_idle,        // =4
    _p_dummy_1,     // =5
    _p_dummy_2,     // =6
    _p_warn,        // =7
    _p_fault_1,     // =8
    _p_fault_2,     // =9
    _p_fault_3,     // =10
    _p_fault_4,     // =11
    _p_override     // =12
};

typedef std::vector < cproperties<content> > vlvmode;

// объявление класса исполнительного механизма 
class cunit: public cproperties<content> { 			// имя класса
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
    bool        m_task_go;
    double      m_position;
    int16_t     m_cmd;
    int16_t     m_mode;
    int16_t     m_mode_old;
    int16_t     m_cmdstatus;
    double      m_minAuto;
    double      m_maxAuto;
            
    ctag*   m_pmod;             // режим 
    ctag*   m_psel;             // выбор 
    ctag*   m_preg;             // ссылка на регулируемый параметр
    cunit*  m_pother;           // парный юнит
    int16_t m_error;            // ошибка 
    int16_t m_motion;           // движение клапана motionstate
    ctag*   m_ppos;             // положение клапана/продуктивность

    union {
        struct {            // valve
            ctag*   m_plso;             // конечный открытия           
            ctag*   m_plsc;             // конечный закрытия
            ctag*   m_pfc;              // текущий счетчик
            ctag*   m_pcmdopen;         // команда движения            
            ctag*   m_pcmdclose;        // команда обратного движения            
            ctag*   m_pblock;           // ссылка на блокирующий параметр
            double  m_delta;            // дельта для остановки клапана
            double  m_maxdelta;         // максимальная дельта
        };
        struct {            // pump
            ctag*   m_prun;             // 1 - работает, 0 - нет           
            ctag*   m_pflt;             // ошибка
            ctag*   m_pready;           // готовность
            ctag*   m_ptask;            // задание продуктивности
            ctag*   m_pstart;           // команда включения            
            ctag*   m_pstop;            // команда отключения            
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
    int16_t control( void* );
    int16_t argsize( void ) { return args.size(); }
    int16_t gettype() { return m_nType; }
    int16_t getmode() { return m_mode; }
    int16_t getmotion() { return m_motion; }
//    int16_t task(double rval) { n_task = rval; }
//    double  taskget() { return n_task; } 
    int16_t stop();
    void    pidEval( void* );
    bool    valveCtrlEnabled() {
        return (_sw==_no_valve || _sw==m_num);
    }
    int16_t changemode(int16_t nv);
    int16_t revertmode();
    bool isOpen() { return _lso; }
    bool isClose() { return _lsc; }
    void open(); 
    void close(); 
    void settask( double, bool );
    double gettask() { return m_task; }
    int16_t getnum() { return m_num; }
    bool    isReady() { return (m_fInit>0); }
};

extern vlvmode vmodes;
extern vlvmode vstatuses;
extern vlvmode pstatuses;
typedef std::vector <cunit *> unitvector;

#endif  // _UNIT_HPP_
