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
//int16_t assignValues( string&, const string&, const string&, char*, int16_t& );   

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
    void setprev(int16_t n) { m_prevpage = n; }
    int16_t getprev() { return m_prevpage; }
    void rownext() { 
//        cout<<" old row "<<m_currow;
        m_currow = (m_currow<rows.size()-1)? m_currow+1: 0; 
//        cout<<" new row "<<m_currow<<endl;
    }    
    void rowprev() { 
//        cout<<" old row "<<m_currow;
        m_currow = (m_currow>0)? m_currow-1: rows.size()-1; 
//        cout<<" new row "<<m_currow<<endl;
    }
    int16_t rowget() { return m_currow; }
    int16_t rowset(int16_t _r) { 
        int16_t rc = EXIT_FAILURE;
        if(_r>=0 && _r<=rows.size()-1) { m_currow = _r; rc=EXIT_SUCCESS; }
        return rc;
    }
};
typedef std::vector <pagestruct> pagearray;

struct cbtn {
    bool esc:1;
    bool left:1;
    bool right:1;
    bool enter:1;
    bool down:1;
    bool up:1;
    bool nc1:1;
    bool nc2:1;
};

class view : public Noritake_VFD_GU3000, public cproperties {
    pagearray   pages; 
    int16_t     m_curpage;
    int16_t     m_maxpage;
    cbtn        m_btn;
    int16_t     m_mode;
    bool        m_visible;

    public:
        view() {
            GU3000_init();
            GU3000_setCharset(CP866);
            GU3000_setFontSize(_6x8Format,1,1);
//            GU3000_setFontSize(_8x16Format,1,1);
            GU3000_setScreenBrightness(20);
            m_curpage = 0;
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
        
        template <class T>
        void setproperty( std::string& spr, T& svl ) {
            cproperties::setproperty(spr, svl);
        }       
        
        template <class T>
        int16_t getproperty( std::string& spr, T& svl ) {
            return cproperties::getproperty(spr, svl);
        }       
       
        void setproperty( int16_t npg, std::string& spr, std::string& svl ) {
            pages.at(npg).setproperty(spr, atoi(svl.c_str()));
        }

        int16_t getproperty( int16_t npg, std::string& spr, int16_t& res ) {
            return pages.at(npg).getproperty( spr, res );
        }
        
        void pageBack() {     
            int16_t n;
            n = (m_curpage<1) ? m_maxpage: m_curpage-1;
            pages.at(n).setprev( m_curpage );
            GU3000_clearScreen();
            m_curpage = n;
         }
        void pageNext() {
            int16_t n;
            n = (m_curpage>=m_maxpage) ? 0 : m_curpage+1;
            pages.at(n).setprev( m_curpage );
            GU3000_clearScreen();
            m_curpage = n;
         }
        void pagePrev() {
            GU3000_clearScreen();
            m_curpage = pages.at(m_curpage).getprev();
        }
        void setMaxPage(int16_t n) { m_maxpage = n; }
        void keymanage();
        void gotoDetailPage();
        int16_t pagessize() { return pages.size(); }
};

extern view dsp;
void* viewProcessing(void *args);


#endif
