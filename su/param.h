// заголовочный файл param.h
#ifndef _PARAM_H
	#define _PARAM_H

#include <string>	
#include <algorithm>
#include <vector>
#include <ctime>
#include <stdint.h>
#include <stdlib.h>

#include <sstream>

std::string to_string(int16_t i);
char easytoupper(char in);
char easytolower(char in);

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
    compProp(std::string const& s) {
        _s = s;
        std::transform(_s.begin(), _s.end(), _s.begin(), easytolower);
    }
    bool operator () (cfield const& p) {
        std::string tmp = p._n;
        std::transform(tmp.begin(), tmp.end(), tmp.begin(), easytolower);
        return (tmp.find(_s)==0);
    }
};


// интерфейс класса
// объявление класса Параметр
class CParam 			// имя класса
{
private: 				// спецификатор доступа private
protected: 				// спецификатор доступа protected
    
    time_t 			m_ts;
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
    float			m_dvalue;
    float           m_simVal;
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
    fields          m_prop;

public: 				// спецификатор доступа public
    CParam(){};			// конструктор класса
    ~CParam(){};   
	std::string	name;
	uint32_t	m_conn_id;	
	void 		setValue();
    void 		getValue(); 				//                     
    
    void addProperty(std::string na, std::string v="")
    {
        cfield fld;
        fields::iterator ifi = find_if(m_prop.begin(), m_prop.end(), compProp(na));
        if(ifi == m_prop.end()) {
            fld._n = na;
            fld._v = v;
            m_prop.push_back(fld);
        }
        return;
    }
   int16_t setProperty(int16_t n, std::string na, std::string v)
    {
        int16_t res=EXIT_FAILURE;
        if(n<m_prop.size()) {
            m_prop[n]._n = na;
            m_prop[n]._v = v;
            res=EXIT_SUCCESS;
        }
        return res;
    }
    cfield* getProperty(int16_t n)
    {
        cfield *res=NULL;
        if(n<m_prop.size()) {
            res=&m_prop[n];
        }
        return res;
    }
    std::string getProperty(std::string s)
    {
        std::string sOut = "no value";
        fields::iterator ifi = find_if(m_prop.begin(), m_prop.end(), compProp(s));
        if(ifi != m_prop.end()) {
            sOut=(*ifi)._v;
        }
        return sOut;
    }
    int16_t getProperty(std::string s, int16_t &nOut)
    {
        int16_t res=EXIT_FAILURE;
        fields::iterator ifi = find_if(m_prop.begin(), m_prop.end(), compProp(s));
        if(ifi != m_prop.end()) {
            nOut=(*ifi).ToInt();
            res=EXIT_SUCCESS;
        }
        return res;
    }
    int16_t setProperty(std::string s, std::string &sIn)
    {
        int16_t res=EXIT_FAILURE;
        fields::iterator ifi = find_if(m_prop.begin(), m_prop.end(), compProp(s));
        if(ifi != m_prop.end()) {
            (*ifi)._v = sIn;
            res=EXIT_SUCCESS;
        }
        return res;
    }
    int16_t setProperty(std::string s, int16_t &nIn)
    {
        int16_t res=EXIT_FAILURE;
        fields::iterator ifi = find_if(m_prop.begin(), m_prop.end(), compProp(s));
        if(ifi != m_prop.end()) {
            (*ifi)._v = to_string(nIn);
            res=EXIT_SUCCESS;
        }
        return res;
    }
    int32_t getPropertySize()
    {
        return m_prop.size();
    }
}; // конец объявления класса CParam

int16_t readCfg();
void* fieldXChange(void *args);    // поток обмена по Modbus с полевым оборудованием
void* paramProcessing(void *args); // поток обработки параметров 

typedef std::vector< CParam, std::allocator<CParam> > paramlist;
extern paramlist tags;
extern bool fParamThreadInitialized;

#endif // _PARAM_H
