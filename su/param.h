// заголовочный файл param.h
#ifndef _PARAM_H
	#define _PARAM_H

#include <string>	
#include <algorithm>
#include <vector>
#include <ctime>
#include <stdint.h>
#include <stdlib.h>
#include "main.h"
#include "utils.h" 


enum {
    _parse_root,
    _parse_mbport,
    _parse_mbcmd,
    _parse_ai,
    _parse_upcon,
    _parse_display
};


// интерфейс класса
// объявление класса Параметр
class cparam: public cproperties { 			// имя класса

private: 				// спецификатор доступа private
protected: 				// спецификатор доступа protected
    
    timespec		m_ts;
    timespec		m_oldts;
    double			m_rvalue;
    int16_t         m_task;             // task to out
    int16_t         m_task_delta;       // task out allowable deviation 
    bool            m_task_go;          // flag 4 task to out
    int16_t         m_raw;              // raw value from module
    int16_t         m_raw_old;          // raw value from module prev step
    uint8_t         m_quality;          // quality of value
    bool            m_valueupdated;     // new value arrived
    int16_t         m_readOff;
    int16_t         m_readbit;    
    int16_t         m_writeOff;
    double          m_minEng;
    double          m_maxEng;
    double          m_minRaw;
    double          m_maxRaw;
    int32_t         m_fltTime;
    bool            m_isBool;
    double          m_hihi;
    double          m_hi;
    double          m_lo;
    double          m_lolo;
    int16_t         m_connErr;
    double          m_deadband;
    std::string     m_name;
    std::string     topic;

public: 				// спецификатор доступа public
    cparam();			// конструктор класса
    cparam(const cparam&) {}		// конструктор класса
   ~cparam(){};   
    int16_t     m_sub;   
    cton        m_tasktimer;
    void		*p_conn;	
    void 		getValue(); 				//                     
    time_t*     getTS() {
        return &(m_ts.tv_sec);
    }
    void    init();
    int16_t getraw(int16_t &nOut);                              // get raw data from readdata buffer
    int16_t getvalue(double &rOut, uint8_t &nQual);             // get value in EU
    int16_t setvalue();                                         // write tasks to modbus writedata area 
    int16_t settask(double rin) {
        if( m_maxRaw-m_minRaw!=0 && m_maxEng-m_minEng!=0 ) 
            m_task    = (m_maxRaw-m_minRaw)/(m_maxEng-m_minEng)*(rin-m_minEng)+m_minRaw;
        else m_task = rin;
        m_task_go = true;    
    }

    bool    taskset() { return m_task_go; }
    bool    hasnewvalue() { return m_valueupdated; }
    bool    acceptnewvalue() { m_valueupdated = false; }
    int16_t getpubcon() { int16_t u=0; getproperty("pub", u); return --u; }    
    int16_t getsubcon() { int16_t u=0; getproperty("sub", u); return --u; }   

    int16_t rawValveValueEvaluate();
}; // конец объявления класса cparam

int16_t readCfg();
void* fieldXChange(void *args);    // поток обмена по Modbus с полевым оборудованием
void* paramProcessing(void *args); // поток обработки параметров 

int16_t taskparam( std::string&, std::string&, std::string& );
int16_t taskparam( std::string&, std::string& );

int16_t getparam( std::string&, double&, int16_t& qual, time_t* );
int16_t getparam( std::string&, std::string& );

typedef std::vector<std::pair<std::string, cparam> > paramlist;

extern paramlist tags;
extern bool fParamThreadInitialized;
extern pthread_mutex_t mutex_param;
extern pthread_cond_t  data_ready;
extern pthread_mutex_t mutex_pub;
extern pthread_cond_t  pub_ready;

#endif // _PARAM_H
