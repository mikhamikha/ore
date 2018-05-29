// заголовочный файл algo.h
#ifndef _ALGO_HPP_
	#define _ALGO_HPP_

#include <functional>
#include <numeric>
#include <string>	
#include <algorithm>
#include <vector>
#include <ctime>
#include <stdint.h>
#include <stdlib.h>

#include "unit.h"
#include "tag.h"
#include "utils.h"

#define _loc_data_buffer_size   10
#define _valve_mode_amount      8 

enum algtype {
    _unitProcessing = 1,   // Управление механизмом   
    /*
    _valveEval = 1,        // Расчет положения клапана
    _valveProcessing = 2,  // Управление клапаном   
    _valveCalibrate,       // Калибровка клапана
    _timeValveControl,     // Управление клапаном по времени
    _pidValveControl,      // Управление клапаном по ПИД
    */
    _floweval=10,          // Расчет расхода по перепаду давления на сечении клапана 
    _summ=11,              // Суммирование входных аргументов и запись в выходной
    _sub=12                // Вычитание входных аргументов и запись в выходной
};

enum state_algo {
    _idle_a,
    _buzy_a,
    _done_a
};


// объявление класса Алгоритм
class calgo: public cproperties<content> { 			// имя класса
    tagvector   m_args; // вычислить адреса объектов
    tagvector   m_res;
    unitvector  m_units;
    int16_t     m_nType;
    int16_t     m_nUnits;   
//    double      rdata[_loc_data_buffer_size];
    int16_t     m_fInit; 
    cton        m_twait;     
    cton        m_tcmd;
    int16_t     m_stor;
    int16_t     m_enable;
    
    public:
    calgo() { m_fInit=0; m_nUnits=0; m_stor=0; }  
    int16_t init();
    int16_t solveIt( void );
    int16_t argsize( void ) { return m_args.size(); }
    int16_t ressize( void ) { return m_res.size(); }
};

typedef std::vector <calgo*> alglist;

extern alglist algos;


#endif
