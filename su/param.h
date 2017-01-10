// заголовочный файл param.h
#ifndef _PARAM_H
	#define _PARAM_H

#include <string>	
#include <algorithm>
#include <vector>
#include <ctime>
#include <stdint.h>
#include <stdlib.h>

#include "utils.h" 
/*
*/

struct cfield {
    std::string _n;
    int8_t      _t;
    std::string _v;
    float ToReal() {
        return atof(_v.c_str());
    }
    int32_t ToInt() {
        return atoi(_v.c_str());
    }
    std::string ToText() {
        return _n + std::string(" = ") + _v;
    }
};

typedef std::vector<cfield> fields;

struct compProp
{
    std::string _s;
    compProp(std::string const& s);     
    bool operator () (cfield const& p); 
};


// интерфейс класса
// объявление класса Параметр
class cparam 			// имя класса
{
private: 				// спецификатор доступа private
protected: 				// спецификатор доступа protected
    
    timespec		m_ts;
    timespec		m_oldts;
    double			m_dvalue;
    fields          m_prop;             // tag fields
    int16_t         m_task;             // task to out
    bool            m_task_go;          // flag 4 task to out

	int8_t 	        m_quality;
    int8_t          m_type;
	int32_t         m_adc;
    float           m_minEng;
    float           m_maxEng;
    float           m_minRaw;
    float           m_maxRaw;
    float           m_hihi;
    float           m_hi;
    float           m_lo;
    float           m_lolo;
    float           m_hihi_en;
    float           m_hi_en;
    float           m_lo_en;
    float           m_lolo_en;
    double          m_simvalue;
	bool			m_bvalue;
    bool            m_sim;
    bool            m_overrange;
    bool            m_underrange;
    bool            m_hihi_tr;
    bool            m_hi_tr;
    bool            m_lo_tr;
    bool            m_lolo_tr;
    bool            m_isBool;
    std::string     topic;

public: 				// спецификатор доступа public
    cparam();			// конструктор класса
    ~cparam(){};   
	std::string	name;
    void		*p_conn;	
	void 		setValue();
    void 		getValue(); 				//                     
    time_t*     getTS() {
        return &(m_ts.tv_sec);
    }
    void addproperty(std::string na, std::string v);
    int16_t getraw(int16_t &nOut);                           // get raw data from readdata buffer
    int16_t getvalue(double &rOut);                          // get value in EU
    int16_t taskprocess();                                   // write tasks to modbus writedata area
    bool    taskset() { return m_task_go; }
    cfield* getproperty(int16_t n);
    std::string getproperty(std::string s);
    int16_t getproperty(std::string s, int16_t &nOut);
    int16_t getproperty(std::string s, int32_t &nOut);
    int16_t getproperty(std::string s, double &rOut);
    int16_t setproperty(int16_t n, std::string na, std::string v);
    int16_t setproperty(std::string s, std::string &sIn);
    int16_t setproperty(std::string s, int16_t &nIn);
    int32_t getpropertysize() { return m_prop.size(); }
}; // конец объявления класса cparam

int16_t readCfg();
void* fieldXChange(void *args);    // поток обмена по Modbus с полевым оборудованием
void* paramProcessing(void *args); // поток обработки параметров 
int16_t taskparam(std::string, std::string);

typedef std::vector< cparam, std::allocator<cparam> > paramlist;
extern paramlist tags;
extern bool fParamThreadInitialized;
extern pthread_mutex_t mutex_param;
extern pthread_cond_t  start_param;

#endif // _PARAM_H
