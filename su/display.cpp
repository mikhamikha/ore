//#include "display.h"
#include "main.h"

using namespace std;

view dsp;

//
// функция заполнения данных класса дисплея конфигурационными данными
//
void view::definedspline( int16_t nd, int16_t nr, std::string sr) {
    if( nd > 0 && nr > 0 ) {
        if( pages.size() < nd ) {
            pagestruct  page;
            while( pages.size() < nd ) pages.push_back(page);
        }
        if( pages[nd-1].rows.size() < nr ) {
            string s("");
            while( pages[nd-1].rows.size() < nr ) pages[nd-1].rows.push_back(s);
        }
        pages[nd-1].rows[nr-1] = sr;
        cout<<"define dsp "<<nd<<" row "<<nr<<" content "<<sr \
            <<" dspsize "<<pages.size()<<" rows "<<pages[nd-1].rows.size()<<endl;
    }
}

//
// перевод кодировки русских символов в CP866  
//
void view::to_866(string& sin, string& sout) {
    uint16_t    n, j;
    wchar_t     ws[2];    
    char        ss[2];
//    int i = 0;
    vector<char>buf;

    for(j=0; j<sin.length(); ) {
//        printf("%d=%d>", j, sin[j]);
        if(sin[j]<208) {
            buf.push_back(sin[j]);
//            *(*sout+i) = sin[j];
            ++j;
        } 
        else if(j<sin.length()-2){
            ss[1]=sin[j]; 
            ss[0]=sin[j+1];
            n = *((int16_t *)ss);
    
            if(n>=53392 && n<=53439) {
                n = n-53264;
            }
            else if(n>=53632 && n<=53647) {
                n = n-53408;
            }
            else if(n==53377) {
                n = 240;
            }
            else if(n==53649) {
                n = 241;
            }
            buf.push_back(n);
            j += 2;
        }
//        ++i;
    }
    string s(buf.begin(), buf.end());
    sout = s;
}

void view::println( string& sin ) {
    string buf, son("{"), soff("}");
    char    buff[100];
    int16_t len;

    to_866( sin, buf );
    assignValues(buf, son, soff, buff, len);
//    assignValues(buf, son, soff);
//    cout<<buf<<endl;
//    Noritake_VFD_GU3000::println(buf.c_str());
    
    Noritake_VFD_GU3000::println( buff, len );
}

//
// функция замены тэгов в строках {tag} на их значения
//
int16_t assignValues(string& subject, const string& sop, const string& scl, char* buf, int16_t& len ) {
    size_t      nbe = 0, nen = 0, oldp = 0;
    int16_t     rc=EXIT_SUCCESS;
    bool        fgo = true;
    int16_t     pos=0;

//    cout<<subject<<" "<<subject.length()<<endl;
    while(fgo) {
        nbe = subject.find(sop, nen); 
        fgo = (nbe != std::string::npos);
        if( fgo && (nen = subject.find(scl, nbe)) != std::string::npos ) {
                string na(subject, nbe+1, nen-nbe-1);
                string va;
//                cout<<" beg="<<nbe<<" end="<<nen<<" tag="<<na<<" | ";
                if(getparam( na, va ) == EXIT_SUCCESS) {
                    memcpy( buf+pos, string(subject, oldp, nbe-oldp).c_str(), nbe-oldp );
                    oldp = nen;
                    pos += (nbe-oldp);
                    memcpy( buf+pos, _bold_on, _cmd_len );
                    memcpy( buf+pos+_cmd_len, va.c_str(), va.length() );
                    memcpy( buf+pos+_cmd_len+va.length(), _bold_off, _cmd_len );
                    pos += (_cmd_len*2+va.length());
                }
        }
        else fgo =false;
//        cout<<fgo<<" "<<subject<<" "<<subject.length()<<endl;
    }
    len = pos;
//    cout<<endl;
    return rc;
}

//
// функция замены тэгов в строках {tag} на их значения
//
int16_t assignValues(string& subject, const string& sop, const string& scl) {
    size_t      nbe = 0, nen = 0;
    int16_t     rc=EXIT_SUCCESS;
    bool        fgo = true;

//    cout<<subject<<" "<<subject.length()<<endl;
    while(fgo) {
        nbe = subject.find(sop, nen); 
        fgo = (nbe != std::string::npos);
        if( fgo && (nen = subject.find(scl, nbe)) != std::string::npos ) {
                string na(subject, nbe+1, nen-nbe-1);
                string va;
//                cout<<" beg="<<nbe<<" end="<<nen<<" tag="<<na<<" | ";
                if(getparam( na, va ) == EXIT_SUCCESS) {
                    subject.replace( nbe, nen-nbe+1, va );
                    nen = nbe+va.length();
                }
        }
        else fgo =false;
//        cout<<fgo<<" "<<subject<<" "<<subject.length()<<endl;
    }
//    cout<<endl;
    return rc;
}

//
// функция вывода сконфигурированного кадра на дисплей
//
void view::outview(int16_t ndisp=0) {
    int16_t     nd = (ndisp>0)? ndisp-1: m_curpage;
    int16_t     nr;
    pagestruct  &pg = pages[nd];
    int16_t     nfont;
    
    string sfont("font");
    getproperty( nd, sfont, nfont );
    for(nr = 0; nr < pg.rows.size(); ++nr) {
        string sout;
        if(!nr) {
            GU3000_setFontSize(_6x8Format,1,1);
            GU3000_boldOn();
        }
        else {
            GU3000_setFontSize( (nfont==2)? _6x8Format: _8x16Format, 1, 1 );
            GU3000_boldOff();
        }
        sout = pg.rows[nr];
        sout.append( 10u, ' ' );
        GU3000_setCursor( 0, nr*11 );
        if( nr == pg.rowget() ) Noritake_VFD_GU3000::print( _invert_on );
        println( sout );
        if( nr == pg.rowget() ) Noritake_VFD_GU3000::print( _invert_off );
    }
}

//
// поток обработки консоли 
//
void* viewProcessing(void *args) {
    
    dsp.setMaxPage( dsp.pages.size() );

    while (fParamThreadInitialized) {
        dsp.keymanage();
        dsp.outview( dsp.curview() );
        usleep(100000);
    }
    return EXIT_SUCCESS;
}

void view::keymanage() {
    std::string stag; 
    double      rval;
    int16_t     nqu;
    time_t      tts;
    static bool binit = false;
    std::string btns[]  = { "HS01", "HS02", "HS03", "HS04", "HS05", "HS06" };
    bool        bbtn[]  = { false, false, false, false, false, false, false, false };
    pagestruct  &pg = pages[m_cur];
    
    for(int j=0; j<sizeof(btns); j++) {         // read buttons states on front panel of box
        getparam( btns[j], rval, nqu, &tts );
        bbtn[j] = (rval != 0);
    }
    memcpy( (void*)&m_btn, bbtn, sizeof(m_btn) );
    // processing buttons according to the display mode
    switch(m_mode) {
        case _view_mode:
            if(m_btn.esc) {
                m_visible = !m_visible;
                (m_visible) ? GU3000_displayOn() : GU3000_displayOff();
            }
            if(m_btn.left) {            
                pageBack();
            }
            if(m_btn.right) {            
                pageNext();
            }
            if(m_btn.down) {            
                pg.rownext();
            }  
            if(m_btn.up) {            
                pg.rowprev();
            }                    
            if(m_btn.enter) {            
                gotoDetailPage();
            }                    
            break;
    }
}

void view::gotoDetailPage() {
    int16_t     np;

    pg = pages[m_curpage];
    definedspline( m_maxpage+1, 0, pg.rows[0]  );   
    definedspline( m_maxpage+1, 1, pg.rows[1]  );   
    definedspline( m_maxpage+1, 2, getTagName(pg.rows[m_currow]) );   
    definedspline( m_maxpage+1, 3, buildOutStr( "Значение ", pg.rows[m_currow] );
    definedspline( m_maxpage+1, 4, buildInStr ( "Задание ", pg.rows[m_currow] );
    definedspline( m_maxpage+1, 5, "[Esc]     [ < ]     [ > ]" );   
    m_mode = _task_mode;
}

