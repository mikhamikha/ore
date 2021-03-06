#ifndef _DISPLAY_HPP_ 
    #define _DISPLAY_HPP_

#include <config.h>
#include <Noritake_VFD_GU3000.h>
#include <string>
#include <vector>
#include <cwchar>
//#include "tag.h"
#include "utils.h"
#include "thread.h"

const char      _invert_on  = '\x11';
const char      _invert_off = '\x10';
const char      _bold_on[]  = { 0x1F, 0x28, 'g', 0x41, 0x01 };
const char      _bold_off[] = { 0x1F, 0x28, 'g', 0x41, 0x01 };
const int16_t   _cmd_len = 5;

typedef std::vector <cproperties<content> > rowsarray;
int16_t assignValues( void*, string&, const string&, const string& );
//int16_t assignValues( string&, const string&, const string&, char*, int16_t& );   
void to_866( string&, string& );

enum {
    _view_mode,
    _task_mode,
    _sleep_mode
};

enum _type_dsp_obj {
    _tag,
    _unit
};

class pagestruct: public cproperties<content> {
   int16_t     m_currow;
   int16_t     m_prevpage;
   double      m_task;
    
   rowsarray   rows; 

    public:
        pagestruct() {
            m_currow = 0;
            m_prevpage = 0;
            m_task = 0;
        }
        
    string    m_tag;
    void      *p; 
    
    void setprev(int16_t n) { m_prevpage = n; }
    int16_t getprev() { return m_prevpage; }
    void rownext(int16_t n=1) { 
        int16_t _tmp1;
//        cout<<" old row "<<m_currow;
        m_currow = (m_currow+n)%rows.size(); 
//        cout<<" new row "<<m_currow<<endl;
        if(getproperty(m_currow, "tag", _tmp1)) rownext();
    }    

    void rowprev(int16_t n=1) { 
        int16_t _tmp, _tmp1;
        int16_t _size = rows.size();
        cout<<" old row "<<m_currow<<" rows="<<_size<<" n="<<n;
        _tmp = (m_currow-n);
        cout<<" immed="<<_tmp;
        _tmp = (_tmp % _size);
        cout<<" immed="<<_tmp;
        if(_tmp<0) _tmp = _size-abs(_tmp);
        m_currow = _tmp;
        cout<<" new row "<<m_currow<<endl;
        if(getproperty(_tmp, "tag", _tmp1)) rowprev();
    }

    int16_t rowget() { return m_currow; }
    int16_t rowssize() { return rows.size(); }
   
    int16_t rowset(int16_t _r) { 
        int16_t rc = EXIT_FAILURE;
        if(_r>=0 && _r<=int(rows.size())-1) { m_currow = _r; rc=EXIT_SUCCESS; }
        return rc;
    }
        
    template <class T>
    void setproperty( std::string& spr, T& svl ) {
        cproperties::setproperty(spr, svl);
    }       
    
    template <class T>
    int16_t getproperty( std::string& spr, T& svl ) {
        return cproperties::getproperty(spr, svl);
    }       
   
    template <class T>
    void setproperty( int16_t nr, const char* prop, T& svl ) {
        std::string spr = prop;
        if( int(rows.size()) <= nr ) {
            cproperties s;
            s.setproperty("format","");
//            s.setproperty("visible", 0);
            while( int(rows.size()) <= nr ) rows.push_back(s);  
        }       
        rows.at(nr).setproperty( spr, svl );
    }       
    template <class T>
    int16_t getproperty( int16_t nr, const char* prop, T& svl ) {
        int16_t rc;
        std::string spr = prop;

        if(nr>=0 && (size_t)nr<rows.size())
            rc = rows.at(nr).getproperty(spr, svl);
        else rc = EXIT_FAILURE;

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

class view : public Noritake_VFD_GU3000, public cproperties<content>, public cthread {
    pagearray   pages; 
    int16_t     m_curpage;
    int16_t     m_maxpage;
    cbtn        m_btn;
    int16_t     m_mode;
    bool        m_visible;
    cton        m_tsleeper;
    uint16_t    m_unlockkey;
     
    public:
        view() {
            GU3000_init();
            GU3000_setCharset(CP866);
            GU3000_setFontSize(_6x8Format,1,1);
//          GU3000_setFontSize(_8x16Format,1,1);
            GU3000_setScreenBrightness(20);
            m_curpage = 0;
            m_mode= _view_mode;
            m_visible = true;
        }
        ~view() {}
        void outview( int16_t );
//      int16_t curview() { return m_curpage; }
        pagestruct* curpage() { 
            return ((m_curpage>=0 && m_curpage<(int(pages.size()))) ? &pages[m_curpage] : NULL); 
        }
        void setcurview(int16_t n) { m_curpage = n; }
        void println( string& sin, bool, bool );
        void definedspline( int16_t, int16_t, const char*, const std::string& );
        
        template <class T>
        void setproperty( std::string& spr, T& svl ) {
            cproperties::setproperty(spr, svl);
        }       
        
        template <class T>
        int16_t getproperty( std::string& spr, T& svl ) {
            return cproperties::getproperty(spr, svl);
        }       
        
        void addpages( int16_t nd ) {
            if( int(pages.size()) <= nd ) {
                pagestruct  page;
                while( int(pages.size()) <= nd ) pages.push_back(page);
            }   
        }
        
        void setproperty( int16_t npg, std::string& spr, std::string& svl ) {
            addpages(npg);
            cout<<"\npage="<<pages.size()<<"\n";
            pages.at(npg).setproperty(spr, svl);
        }

        int16_t getproperty( int16_t npg, std::string& spr, int16_t& res ) {
            int16_t rc = EXIT_FAILURE;
            if(npg>=0 && npg<int(pages.size()))
                rc =  pages.at(npg).getproperty( spr, res );
            return rc;
        }
        
        void pageDisplay( int16_t npg ) {     
            uint16_t n;
            int16_t num=-1;
            for( n=0; n<pages.size(); n++ ) {
                string s = "num";
                if( getproperty( n, s, num )==EXIT_SUCCESS && num==npg ) {
                    pages.at(n).setprev( m_curpage );
                    GU3000_clearScreen();
                    m_curpage = n;
                    break;
                }
            }
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
            if(m_curpage<0 || m_curpage>=int(pages.size())) m_curpage = 0;
        }
        void run();
        void setMaxPage(int16_t n) { m_maxpage = n; }
        void keymanage();
        void gotoDetailPage();
        int16_t pagessize() { return pages.size(); }
        void setSleepWait(int32_t n) { m_tsleeper.setPreset(n); }
        void setUnlockCode(string &scode) {
            m_unlockkey=0;   
            vector<string> keys;
            strsplit(scode, ';', keys);       
            for( uint16_t i=0; i<keys.size(); i++ ) {
                int num = atoi(keys[i].c_str());
                if(num--) m_unlockkey |= (1<<num);
            }
            cout<<"unlock Code = "<<hex<<m_unlockkey<<dec<<endl;
        }
};

extern view dsp;
void* viewProcessing(void *args);
string getTagName( const char* );


#endif
