// заголовочный файл tag.h
#ifndef _TAG_HPP_
	#define _TAG_HPP_

#include <string>	
#include <algorithm>
#include <vector>
#include <ctime>
#include <stdint.h>
#include <stdlib.h>
#include "utils.h"

#define m_pos       tag1

// объявление класса Параметр
class ctag: public cproperties<content> { 			// имя класса

private: 				// спецификатор доступа private
protected: 				// спецификатор доступа protected
    
    timespec	m_ts;
    timespec	m_oldts;
    union {
        double  m_rvalue;
        int32_t m_nvalue;    
    };
    union {
        double	m_rvalue_old;
        int32_t m_nvalue_old;
    };
    double      m_task;             // task to out
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
    int16_t     m_type;
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
    ctag*     tag1; 
    void*       m_pup;          // указатель на исходящее соединения  
public: 				// спецификатор доступа public
    ctag();			// конструктор класса
//    ctag(const ctag&) {}		// конструктор класса
   ~ctag(){};   
//    int16_t     m_sub;   
    cton        m_tasktimer;
    void		*p_conn;	
    enum DataType {
        _real_type,
        _int_type,
    };

    timespec* getTS() { return &m_ts; }
    int32_t getmsec();
    double  gettrigger();
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
        if( !fraw && m_maxRaw-m_minRaw!=0 && m_maxEng-m_minEng!=0 ) { 
            val = (m_maxEng-m_minEng)/(m_maxRaw-m_minRaw)*(m_task-m_minRaw)+m_minEng;
            val = min( val, m_maxEng );         // ограничим значение инженерной шкалой
            val = max( val, m_minEng );
            
            if( m_minDev!=m_maxDev )  {         // если задана шкала параметра < шкалы прибора,
                val = (m_maxEng-m_minEng)/(m_maxDev-m_minDev)*(val-m_minDev)+m_minEng;
                val = min( val, m_maxEng );     // ограничим значение инженерной шкалой
                val = max( val, m_minEng );
            }        
        }
        else val = m_task;

        return val;    
    }
    int16_t settask( double rin, bool fgo=true ); 

    void cleartask() {
        m_task_go = false;    
    }    

    int16_t settaskpulse(double rin, int32_t pre=2000) {
        int16_t rc = settask( rin, true ); 
        m_task_go = true;    
        m_tasktimer.start(pre);
        return rc;
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
    double  getdead() { return m_deadband; }
    double  gethi() { return m_hi; }
    double  gethihi() { return m_hihi; }
    double  getlo() { return m_lo; }
    double  getlolo() { return m_lolo; }
    double  getmaxraw() { return m_maxRaw; }
    double  getminraw() { return m_minRaw; }
    double  getmaxeng() { return m_maxEng; }
    double  getmineng() { return m_minEng; }
    bool    taskset() { return m_task_go; }
//    bool    hasnewvalue() { return m_valueupdated; }
//    void    acceptnewvalue() { m_valueupdated = false; }
    //  получить номер внешнего соединения
    int16_t getpubcon() { int16_t u=0; getproperty("pub", u); return --u; }    
    int16_t getsubcon() { int16_t u=0; getproperty("sub", u); return --u; }
    bool    isbool() { return (m_type>0 && m_type<2); }

    int16_t rawValveValueEvaluate();
    int16_t flowEvaluate(); 
    int16_t getdescvalue( string& name, string& sval );
   
    std::string to_text() {
        std::string s;
        std::stringstream out;
        double rsim_en = 0, rsim_v;

        getproperty("simenable", rsim_en);
        getproperty("simvalue", rsim_v);       
        out << " name="<<m_name<<" value="<<m_rvalue<<" quality="<<int16_t(m_quality) \
            << " simen="<<rsim_en<<" simval="<<rsim_v;
        s = out.str();
        return s;
    }
}; // конец объявления класса ctag

typedef std::vector <ctag*> tagvector;


extern bool                     ftagThreadInitialized;
extern pthread_mutex_t          mutex_tag;
extern pthread_mutexattr_t      mutex_tag_attr;
//extern pthread_cond_t         data_ready;
extern pthread_mutex_t          mutex_pub;
//extern pthread_cond_t         pub_ready;

uint8_t getqual(ctag* p);   // Получить качество тэга
double getval(ctag* p);     // Получить значение


#endif // _tag_H

