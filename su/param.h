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

enum {
    _parse_root,
    _parse_mbport,
    _parse_mbcmd,
    _parse_ai,
    _parse_upcon
};

inline int32_t getnumfromstr(std::string in, std::string st, std::string fin) {
    string line = in;
    int32_t res=-1;
    line.erase(0, line.find(st)+st.length()+1);
//    line.erase(line.find(fin));
    if(isdigit(line[0])) res = atoi(line.c_str());
    return res;
}

//typedef std::vector<std::pair<std::string, content> > fields; 

// интерфейс класса
// объявление класса Параметр
class cparam: public cproperties { 			// имя класса

private: 				// спецификатор доступа private
protected: 				// спецификатор доступа protected
    
    timespec		m_ts;
    timespec		m_oldts;
    double			m_rvalue;
//    settings        m_prop;             // tag fields
    int16_t         m_task;             // task to out
    bool            m_task_go;          // flag 4 task to out
    int16_t         m_raw;              // raw value from module
	uint8_t         m_quality;          // quality of value
    bool            m_valueupdated;     // new value arrived

    int8_t          m_type;
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
    int16_t getraw(int16_t &nOut);                              // get raw data from readdata buffer
    int16_t getvalue(double &rOut, uint8_t &nQual);             // get value in EU
    int16_t setvalue(int16_t nIn);                              // set value 
    int16_t taskprocess();                                      // write tasks to modbus writedata area
    bool    taskset() { return m_task_go; }
    bool    hasnewvalue() { return m_valueupdated; }
    bool    acceptnewvalue() { m_valueupdated = false; }
    int16_t getupcon() { int16_t u=0; getproperty("pub", u); return --u; }    
 }; // конец объявления класса cparam

int16_t readCfg();
void* fieldXChange(void *args);    // поток обмена по Modbus с полевым оборудованием
void* paramProcessing(void *args); // поток обработки параметров 
int16_t taskparam(std::string, std::string);
typedef std::vector<std::pair<std::string, cparam> > paramlist;

extern paramlist tags;
extern bool fParamThreadInitialized;
extern pthread_mutex_t mutex_param;
extern pthread_cond_t  data_ready;
extern pthread_mutex_t mutex_pub;
extern pthread_cond_t  pub_ready;

#endif // _PARAM_H
