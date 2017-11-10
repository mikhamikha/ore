/* 
 * Модуль управления списком управляющих механизмов ( задвижки, насосы )
 */

#ifndef _UNIT_DIRECTOR_HPP_
    #define _UNIT_DIRECTOR_HPP_

#include "unit.h"
#include "thread.h"

#define _unit_prc_delay    100000

typedef std::vector<std::pair<std::string, cunit> > unitlist;

// объявление класса 
class cunitdirector: public cthread {
    private:
        unitlist units;
       
    public:
        int16_t addunit( string& s, cunit& p );
        cunit*   getunit( const char* );
        /*
        int16_t taskunit( std::string&, std::string&, std::string& );
        int16_t taskunit( std::string&, std::string& );

        int16_t getunit( const char*, double&, int16_t& qual, timespec*, int16_t );
        int16_t getunit( const char*, std::string& );
        int16_t getunitcount( const char*, int16_t& );
        int16_t getunitlimits( const char*, double&, double& );
        */
        void run();
        int32_t size() { return units.size(); }
};


extern cunitdirector unitdir;


cunit* getaddrunit(string& str);    // получить ссылку на юнит по имени

#endif // _UNIT_DIRECTOR_HPP_
