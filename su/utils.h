#ifndef _utils_h
    #define _utils_h

#include <math.h>
#include <time.h>
#include <stdint.h>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <string.h>
#include <sstream>
#include <algorithm>
#include <vector>
#include <iterator>

#define _billion    1000000000l
#define _million    1000000l

using namespace std;

enum {
    INITIALIZED,
    INIT_ERR,
    TERMINATE
};

extern int32_t dT;

void outtext(std::string tx);
std::string to_string(int32_t i);
std::string to_string(double i);
char easytoupper(char in);
char easytolower(char in);
void removeCharsFromString( string &str, char* charsToRemove );
string time2string( time_t rawtime );
string replaceString(string subject, const string& search, const string& replace);
void setDT();

struct cton {
    timespec    m_start;        // timer start
    int32_t     m_preset;       // timer value msec

    cton() { }

    cton(int32_t pre):m_preset(pre) {
        clock_gettime(CLOCK_MONOTONIC, &m_start);    
    }

    void start (int32_t pre) {
        m_preset = pre;
        clock_gettime(CLOCK_MONOTONIC, &m_start);    
    }

    bool isDone() {
        timespec    t;
        int64_t     delta;

        clock_gettime(CLOCK_MONOTONIC, &t);
        delta = ((t.tv_sec-m_start.tv_sec)*_million+(t.tv_nsec-m_start.tv_nsec)/1000)/1000;
//        cout <<"ton="<<m_start.tv_sec<<"."<<m_start.tv_nsec<<" "<<\
             "\nten="<<t.tv_sec<<"."<<t.tv_nsec<<" "<<endl;
//        outtext(string("ton=")+to_string(m_preset)+string(" delta=")+to_string(delta));
        return (m_preset<=delta);
    }
};

struct content {
    content(int32_t n) : _n(n) { _t=0; _r=n; _s=to_string(n); }
    content(double n) : _r(n) { _t=1; _n=int(n); _s=to_string(n); }
    content(char c) : _c(c) { _t=2; _s[0]=c; _s[1]=0; }
    content(std::string s) : _s(s) { 
        _t = 3;
        _c = s[0];
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
    int32_t getvalue(string &res) { res = _s; return 0;}
    double      _r; 
    int32_t     _n;
    std::string _s;
    char        _c;
    int16_t     _t;
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
        return (tmp.find(_s)==0);
    }
};

typedef std::vector<std::pair<std::string, content> > settings;

#endif
