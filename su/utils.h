#ifndef _utils_h
    #define _utils_h

#include <time.h>
#include <stdint.h>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <string.h>
#include <sstream>
#include <algorithm>

#define _billion    1000000000l
#define _million    1000000l

using namespace std;

void outtext(std::string tx);
std::string to_string(int32_t i);
char easytoupper(char in);
char easytolower(char in);
void removeCharsFromString( string &str, char* charsToRemove );

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

#endif
