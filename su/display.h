#ifndef _display_h
    #define _display_h

#include <config.h>
#include <Noritake_VFD_GU3000.h>
#include <string>
#include <vector>
#include "main.h"
#include <cwchar>
#include <stdio.h>


typedef std::vector <std::string> rowsarray;
int16_t assignValues(string& subject, const string& sop, const string& scl);
    
struct pagestruct {
    rowsarray   rows; 
};

typedef std::vector <pagestruct> pagearray;

class view : public Noritake_VFD_GU3000, public cproperties {
    pagearray   pages;    
    public:
        view(){
            GU3000_init();
            GU3000_setCharset(CP866);
            GU3000_setFontSize(_6x8Format,1,1);
//            GU3000_setFontSize(_8x16Format,1,1);
            GU3000_setScreenBrightness(20);
        }
        ~view(){}
        void to_866( string&, string& );
        void outview( int16_t );
        void print( string& sin );
        void definedspline( int16_t, int16_t, std::string );

};

extern view dsp;
void* viewProcessing(void *args);

#endif
