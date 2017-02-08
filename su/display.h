#ifndef _display_h
    #define _display_h

#include <config.h>
#include <Noritake_VFD_GU3000.h>
#include <string>
#include <vector>
#include "main.h"
#include <cwchar>
#include <stdio.h>

const char      _invert_on  = '\x11';
const char      _invert_off = '\x10';
const char      _bold_on[]  = { 0x1F, 0x28, 'g', 0x41, 0x01 };
const char      _bold_off[] = { 0x1F, 0x28, 'g', 0x41, 0x01 };
const int16_t   _cmd_len = 5;

typedef std::vector <std::string> rowsarray;
int16_t assignValues( string& subject, const string& sop, const string& scl );
int16_t assignValues( string& subject, const string& sop, const string& scl, char *buf, int16_t &len );   

enum {
    _view_mode,
    _task_mode
};


class pagestruct: public cproperties {
   int16_t     m_currow;
   int16_t     m_prevpage;
   
   public:
        pagestruct() {
            m_currow = 0;
        }
    rowsarray   rows; 
    void rownext() { m_currow = (m_currow<rows.size()-2)? m_currow+1: 2; }
    void rowprev() { m_currow = (m_currow>2)? m_currow-1: rows.size()-2; }
    int16_t rowget() { return m_currow; }
    int16_t rowset(int16_t _r) { 
        int16_t rc = EXIT_FAILURE;
        if(_r>=0 && _r<=rows.size()-1) { m_currow = _r; rc=EXIT_SUCCESS; }
        return rc;
    }
};
typedef std::vector <pagestruct> pagearray;

struct cbtn {
    bool esc;
    bool left;
    bool right;
    bool enter;
    bool down;
    bool up;
    bool nc1;
    bool nc2;
};

class view : public Noritake_VFD_GU3000 {
    pagearray   pages; 
    int16_t     m_curpage;
    int16_t     m_maxpage;
    cbtn        m_btn;
    int16_t     m_mode;
    bool        m_visible;
    pagestruct& pg;

    public:
        view() {
            GU3000_init();
            GU3000_setCharset(CP866);
            GU3000_setFontSize(_6x8Format,1,1);
//            GU3000_setFontSize(_8x16Format,1,1);
            GU3000_setScreenBrightness(20);
            m_curpage = 1;
            m_mode= _view_mode;
            m_visible = true;
        }
        ~view() {}
        void to_866( string&, string& );
        void outview( int16_t );
        int16_t curview() { return m_curpage; }
        void setcurview(int16_t n) { m_curpage = n; }
        void println( string& sin );
        void definedspline( int16_t, int16_t, std::string );
        
        void setproperty( int16_t npg, std::string& spr, std::string& svl ) {
            pages[npg].setproperty(spr, atoi(svl.c_str()));
        }

        int16_t getproperty( int16_t npg, std::string& spr, int16_t& res ) {
            return pages[npg].getproperty( spr, res );
        }
        
        void pageBack() {     
            int16_t n;
            n = (m_curpage<=1) ? m_maxpage: m_curpage-1;
            pages[n].m_prevpage = m_curpage;
            m_curpage = n;
         }
        void pageNext() {
            int16_t n;
            n = (m_curpage>=m_maxpage) ? 1 : m_curpage+1;
            pages[n].m_prevpage = m_curpage;
            m_curpage = n;
         }
        void pagePrev() {
            m_curpage = pages[m_curpage].m_prevpage;
        }
        void setMaxPage(int16_t n) { m_maxpage = n; }
        void keymanage();
        void gotoDetailPage();
};

extern view dsp;
void* viewProcessing(void *args);


#endif
