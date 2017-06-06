// заголовочный файл param.h
#ifndef _PARAM_H
	#define _PARAM_H

#include <string>	
#include <algorithm>
#include <vector>
#include <ctime>
#include <stdint.h>
#include <stdlib.h>
#include "mbxchg.h"
#include "main.h"
#include "utils.h" 
#include <pugixml.hpp>

#define m_pos       param1
/*
#define m_ft1       param1
#define m_ft2       param2

#define m_fc        param1
#define m_lso       param2
#define m_lsc       param3
#define m_cmdo      param4
#define m_cmdc      param5

#define m_dt        param1
#define m_kv        param2
#define m_fv        param3
#define m_pt1       param4
#define m_pt2       param5
*/

// объявление класса Параметр
class cparam: public cproperties { 			// имя класса

private: 				// спецификатор доступа private
protected: 				// спецификатор доступа protected
    
    timespec	m_ts;
    timespec	m_oldts;
    double		m_rvalue;
    double		m_rvalue_old;
    int16_t     m_task;             // task to out
    int16_t     m_task_delta;       // task out allowable deviation 
    bool        m_task_go;          // flag 4 task to out
    double      m_raw;              // raw value from module
    double      m_raw_old;          // raw value from module prev step
    bool        m_trigger;          // trigger value from module
    uint8_t     m_quality;          // quality of value
    uint8_t     m_quality_old;      // quality of value prev step
    bool        m_valueupdated;     // new value arrived
    int16_t     m_readOff;
    int16_t     m_readbit;    
    int16_t     m_writeOff;
    double      m_minDev;
    double      m_maxDev;
    double      m_minEng;
    double      m_maxEng;
    double      m_minRaw;
    double      m_maxRaw;
    int32_t     m_fltTime;
    int16_t     m_isBool;
    double      m_hihi;
    double      m_hi;
    double      m_lo;
    double      m_lolo;
    int16_t     m_connErr;
    double      m_deadband;
    std::string m_name;
    std::string m_topic;
    bool        m_firstscan;

    // referenses to counter, limit switches (opened & closed), commands (open & close)
    cparam*     param1; //m_fc;
/*    
    cparam*         param2;//m_lso;
    cparam*         param3;//m_lsc;
    cparam*         param4;//m_cmdo;
    cparam*         param5;//m_cmdc;

//  int32_t         m_cnt_old;
    int16_t         m_motion;  
    int16_t         m_motion_old;  
*/
    
public: 				// спецификатор доступа public
    cparam();			// конструктор класса
//    cparam(const cparam&) {}		// конструктор класса
   ~cparam(){};   
//    int16_t     m_sub;   
    cton        m_tasktimer;
    void		*p_conn;	
               
    timespec* getTS()   { return &m_ts; }
    double  gettrigger()  {
//        cmbxchg     *mb = (cmbxchg *)p_conn;

        if( m_trigger && m_readOff >= 0 ) {
            if( m_readbit >= 0 ) {
                cmbxchg::m_pReadTrigger[m_readOff] &= (0xFFFF ^ ( 1 << m_readbit ));
            }   
        }
        return m_trigger; 
    }
    string  getname() { return m_name; }
    void    getlimits(double& emin, double& emax)  { emin=m_minEng; emax=m_maxEng; }
    double  getvalue()  { return m_rvalue; }
    double  getoldvalue() { return m_rvalue_old; }
    void    setoldvalue(double val) { m_rvalue_old = val; }
    uint8_t getquality(){ return m_quality; }
    void    setquality( uint8_t qual ){ m_quality = qual; }
    void    getfullname (string &sfn) { sfn = m_topic+"/"+m_name; }
    void    init();
    int16_t getraw();                           // get raw data from readdata buffer
    int16_t getvalue(double &rOut);             // get value in EU
    int16_t setvalue(double);                   // write tasks to modbus writedata area 
    double  gettask( bool fraw=false ) {        // get task value; if fraw==true then unscaled task
        double val;
        if( !fraw && m_maxRaw-m_minRaw!=0 && m_maxEng-m_minEng!=0 ) 
            val = (m_maxEng-m_minEng)/(m_maxRaw-m_minRaw)*(m_task-m_minRaw)+m_minEng;
        else val = m_task;

        return val;    
    }
    int16_t settask( double rin, bool fgo=true ); 

    int16_t cleartask() {
        m_task_go = false;    
    }    

    int16_t settaskpulse(double rin, int32_t pre=2000) {
        settask( rin, true ); 
        m_task_go = true;    
        m_tasktimer.start(pre);
    }

    double getrawval() {
        return m_raw;
    }   

    void setrawval( double r ) {
        m_raw = r;
        setproperty( "raw", r );
//        if( m_name.substr(0,4)=="FV11") cout << "? really raw = "<< r <<endl; 
    }  

    void setrawscale( double minr, double maxr ) {
        m_maxRaw = maxr;
        m_minRaw = minr;
        setproperty( "minraw", minr );
        setproperty( "maxraw", maxr );
    }
    double  getmaxraw() { return m_maxRaw; }
    double  getminraw() { return m_minRaw; }
    bool    taskset() { return m_task_go; }
    bool    hasnewvalue() { return m_valueupdated; }
    bool    acceptnewvalue() { m_valueupdated = false; }
    int16_t getpubcon() { int16_t u=0; getproperty("pub", u); return --u; }    
    int16_t getsubcon() { int16_t u=0; getproperty("sub", u); return --u; }
    bool    isbool() { return m_isBool; }

    int16_t rawValveValueEvaluate();
    int16_t flowEvaluate(); 
    std::string to_text() {
        std::string s;
        std::stringstream out;
        double rsim_en = 0, rsim_v;

        getproperty("simen", rsim_en);
        getproperty("simvalue", rsim_v);       
        out << " name="<<m_name<<" value="<<m_rvalue<<" quality="<<int16_t(m_quality) \
            << " simen="<<rsim_en<<" simval="<<rsim_v;
        s = out.str();
        return s;
    }
}; // конец объявления класса cparam

int16_t readCfg();
void* fieldXChange(void *args);    // поток обмена по Modbus с полевым оборудованием
void* paramProcessing(void *args); // поток обработки параметров 

//int16_t taskparam( std::string&, std::string&, std::string& );
int16_t taskparam( std::string&, std::string& );

int16_t getparam( const char*, double&, int16_t& qual, timespec*, int16_t );
int16_t getparam( const char*, std::string& );
cparam* getparam( const char* );
int16_t getparamcount( const char*, int16_t& );
int16_t getparamlimits( const char*, double&, double& );

typedef std::vector<std::pair<std::string, cparam> > paramlist;

extern paramlist                tags;
extern bool                     fParamThreadInitialized;
extern pthread_mutex_t          mutex_param;
extern pthread_mutexattr_t      mutex_param_attr;
//extern pthread_cond_t  data_ready;
extern pthread_mutex_t          mutex_pub;
//extern pthread_cond_t  pub_ready;

#endif // _PARAM_H
