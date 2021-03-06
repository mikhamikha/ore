#ifndef _UTILS_HPP_
    #define _UTILS_HPP_

#include <math.h>
#include <time.h>
#include <stdint.h>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <string.h>
#include <algorithm>
#include <vector>
#include <iterator>
#include "const.h"

#define _billion    1000000000l
#define _million    1000000l

using namespace std;

enum {
    INITIALIZED,
    INIT_ERR,
    TERMINATE
};

extern int32_t dT;

int16_t readCfg();
void outtext(std::string tx);
char easytoupper(char in);
char easytolower(char in);
std::string trim( const std::string& str, const std::string& whitespaces = " \t" );
void reduce( string &str, char* charsToRemove );
string time2string( time_t rawtime );
int16_t replaceString(string& subject, const string& search, const string& replace);
void setDT();
int32_t getnumfromstr(std::string in, std::string st, std::string fin);
int16_t strsplit(string& s, char delim, vector<string>& vec);

template <class T>
string to_string(T i) {
    string s;
    stringstream out;
    out << i;
    s = out.str();
    return s;
}

//
//  структура объекта таймера
//
struct cton {
    timespec    m_start;        // timer start
//    timespec    m_finish;       // timer finish
    int32_t     m_preset;       // timer value msec
    bool        m_tt;
    bool        m_dn;
    int64_t     m_delta;

    cton() { m_tt = false; m_dn = false; m_preset = 0; }

    cton(int32_t pre):m_preset(pre) {
        clock_gettime(CLOCK_MONOTONIC, &m_start);    
    }

    int16_t start (int32_t pre=0) {
        int16_t rc = _exOK;
        m_preset = (pre ? pre : m_preset);
        if(m_preset<=0) { rc=_exBadParam; }
        else {
            clock_gettime(CLOCK_MONOTONIC, &m_start);    
            m_tt = true;
            m_dn = false;
            m_delta = 0;
        }
        return rc;
    }

    bool isDone() {
        timespec    fin;       // timer finish
        
        if( m_preset ) {
            if( m_tt ) {
                clock_gettime(CLOCK_MONOTONIC, &fin);
                m_delta += ((fin.tv_sec-m_start.tv_sec)*_million+(fin.tv_nsec-m_start.tv_nsec)/1000)/1000;
                m_start = fin;
                if(m_preset<llabs(m_delta)) { 
                    m_dn = true;
                    stop();
                }
            }
        }
        return m_dn;
    }

    bool isTiming() {
        return m_tt;
    }
    
    int32_t getTT() {
        return int32_t(m_delta);
    }
  
    void stop() {
        m_tt = false;
    }

    void resume() {
        clock_gettime(CLOCK_MONOTONIC, &m_start);    
        m_tt = true;
    }

    void reset() {
        m_delta = 0;
        m_tt = false;
        m_dn = false;
    }
    
    int32_t getPreset() {
        return m_preset;
    }
    
    void setPreset(int32_t n) {
        if( n>=0 ) m_preset = n;
    }
};

struct content {
    content() { }
    content(const content& ct) : _n(ct._n), _t(ct._t), _r(ct._r), _s(ct._s) {}
    content(int32_t n) : _n(n) { _t=0; _r=n; _s=to_string(n); }
    content(double n) : _r(n) { _t=1; _n=int(n); _s=to_string(n); }
    content(std::string s) : _s(s) { 
        _t = 2;
        _n = atoi(s.c_str());
        _r = atof(s.c_str());
    }
    float ToReal() {
        return atof(_s.c_str());
    }
    int32_t ToInt() {
        int32_t n;
        if(_t==0) n = _n;
        if(_t==1) n = int(_r);
        else n = atoi(_s.c_str());
        return n;     
    }
    string ToString() {
        string sres;

        switch(_t) {
            case 0:  sres = to_string(_n); break;
            case 1:  sres = to_string(_r); break;
            default: sres = _s;
        }
        return sres;
    }    
    void setvalue(int32_t n) {
        _n = n; _s = to_string(n); _r = n; 
    }
    void setvalue(double n) {
        _r = n; _s = to_string(n); _n = int(n); 
    }
    void setvalue(string n) {
        _s = n; _n = atoi(n.c_str()); _r = atof(n.c_str()); 
    }
    int32_t getvalue(int16_t &res) { res = _n; return 0; }
    int32_t getvalue(int32_t &res) { res = _n; return 0; }
    int32_t getvalue(double &res) { res = _r; return 0; }
    int32_t getvalue(string &res) { res.assign(_s); return 0;}
    int32_t     _n;
    int16_t     _t;
    double      _r; 
    std::string _s;
};

template <class T>
struct compareP
{
    std::string _s;
    compareP(std::string const& s) {
        _s = s;
        std::transform(_s.begin(), _s.end(), _s.begin(), easytolower);
    }
    bool operator () (std::pair<std::string, T> const& p) {
        std::string tmp = p.first;
        std::transform(tmp.begin(), tmp.end(), tmp.begin(), easytolower);
        bool rc = ( tmp.size() && _s.size() && tmp.compare(0, _s.size(), _s)==0 );
//        if(rc) cout<<_s<<" "<<tmp<<" "<<double(_s.size())/tmp.size()<<"\t";
        return ( rc && double(_s.size())/tmp.size()>0.5);
    }
};

//template <class T>
//typedef std::vector<std::pair<std::string, T> > settings;

template<class U>
class settings : public std::vector< std::pair<std::string, U>/*, MyCustomAllocator<T>*/ > {};

//template<class T>
//class set_iterator : public std::vector< std::pair<std::string, T>/*, MyCustomAllocator<T>*/ >::iterator {};

//typedef settings
//template <class T>
//using cont = std::list<T>;

template <class U>
class cproperties {
    settings<U> m_set;

    public:
        cproperties() { }
        cproperties(const cproperties& pcp) {
            m_set.assign(pcp.m_set.begin(), pcp.m_set.end());
        }
        template <class T>
        int16_t setproperty( std::string na, T& va ) {       // fill settings
            int16_t res = _exBadParam;
//            if(na=="ki") cout<<endl<<"setproperty ki="<<va<<endl;
//            set_iterator<U> i = std::find_if(m_set.begin(), m_set.end(), compareP<U>(na));
            typename settings<U>::iterator i = std::find_if(m_set.begin(), m_set.end(), compareP<U>(na));
//            cout<<"setproperty "<<na<<" = "<<va.to_string()<<" ";

            if (i != m_set.end()) {
//                cout<<"found ";
                i->second.setvalue( va );
//                cout<<"set\n";
                res = _exOK;
            }
            else {
//                cout<<"not found ";
                m_set.push_back( make_pair(na, U(va)) );
                res = _exBadParam;
//                cout<<"set\n";
            }
//            if(na=="configured") cout<<"\nset property "<<na<<" = "<<va<<endl;
            return res;
        }        

        template <class T>
        int16_t getproperty( std::string na, T& va) {
            int16_t res = _exBadParam;
            typename settings<U>::iterator i = std::find_if(m_set.begin(), m_set.end(), compareP<U>(na));
//            set_iterator<U> i = std::find_if(m_set.begin(), m_set.end(), compareP<U>(na));
            if (i != m_set.end()) {
                i->second.getvalue(va);
                res = _exOK;
            } 
            return res;
        }

        template <class T>
        int16_t getproperty( int16_t i, string& na, T& va) {
            int16_t res = _exBadParam;

            if (i >= 0 && i < int16_t(m_set.size())) {
                m_set[i].second.getvalue(va);
                na = m_set[i].first;
                res = _exOK;
            } 
            return res;
        }
        int32_t getpropertysize() { return int16_t(m_set.size()); }
        void clearPropertyList() { m_set.clear(); } 
        int16_t property2text(int32_t n, std::string& va) {
            int16_t res = _exBadParam;
            
            if (n < getpropertysize()) {
                va = m_set[n].first + " = " + m_set[n].second.ToString();
                res = _exOK;
            }
            return res;
        }
 
};

int16_t readCfg();
string  getPersistData( string& _name, string& _attr );
int16_t setPersistData( string& _name, string& _attr, string& _val );

bool testaddr(void* x);   // проверка на NULL

template <class T>
bool testaddrlist(T& x) {   // проверка списка на NULL
    bool isgood = (x.size() && find_if( x.begin(), x.end(), testaddr ) == x.end());
    return isgood;
}

//template <class T>
//void printdata(T&);       // вывод на экран

// вывод на экран
template <class T>
void printdata(T& in) {
    cout<<' '<<in;
}

template <class T>
class iObserver {
    public:
        virtual ~iObserver() {}
        virtual void valueChanged( T& value ) = 0;
};

/*
class crc16 {
    static uint8_t TABLE1[256], TABLE2[256]; 

  public:

   // ************************************************************************
   // * Procedure   : sys_init_crc
   // * Description : Crc table init
   // ************************************************************************
    static void init() {
       uint8_t i;
       uint16_t mask, crc, mem;
       uint16_t const CRC16 = 0xA001;
       
       for(mask=0; mask<sizeof(TABLE1); mask++) {
          crc = mask;
          for(i=0; i<8; i++) {
             mem = (uint16_t)(crc & 0x0001) ;
             crc /=  2;
             if (mem) crc ^= CRC16 ;
          }
          TABLE2[mask] = (uint8_t)(crc & 0xff); // lobyte 
          TABLE1[mask] = (uint8_t)(crc >> 8);   // hibyte 
       }
    }

    // ***************************************************************
    // * Procedure   : sys_get_crc
    // * Description : Calculate crc
    // ***************************************************************
    static uint16_t doIt(uint8_t *buf, uint16_t size) {
        uint16_t check;
        uint8_t car, i;
        uint8_t crc0, crc1;

        crc0 = 0xff;
        crc1 = 0xff;

        for(i=0; i<size; i++) {  
         car = buf[i];
         car ^= crc0;
         crc0 = (uint8_t)(crc1 ^ TABLE2[car]);
         crc1 = TABLE1[car];
        }
        check = crc1;
        check <<= 8;
        check += crc0;

        return check;
    }
};
*/


#endif
