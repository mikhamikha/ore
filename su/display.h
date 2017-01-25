#ifndef _display_h
    #define display_h

#include <config.h>
#include <Noritake_VFD_GU3000.h>
#include <string>
#include <vector>
#include "utils.h"


typedef std::vector <std::string> rowsarray;

struct pagestruct {
    rowsarray   rows; 
};

typedef std::vector <pagestruct> pagearray;

class view : public Noritake_VFD_GU3000, public cproperties {
    pagearray   pages;    
    public:
        view(){}
        ~view(){}
        void outview( int16_t );
        void definedspline( int16_t, int16_t, std::string );

};

extern view dsp;

#endif
