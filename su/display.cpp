//#include "display.h"
#include "main.h"

#define _dsp_width      256
#define _dsp_height     128
#define _dsp_win_height 96

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
        if( pages.at(nd-1).rows.size() < nr ) {
            string s("");
            while( pages.at(nd-1).rows.size() < nr ) pages.at(nd-1).rows.push_back(s);
        }
        pages.at(nd-1).rows.at(nr-1) = sr;
        cout<<"define dsp "<<nd<<" row "<<nr<<" content "<<sr \
            <<" dspsize "<<pages.size()<<" rows "<<pages.at(nd-1).rows.size()<<endl;
    }
}

//
// перевод кодировки русских символов в CP866  
//
void view::to_866(string& sin, string& sout) {
    uint16_t    n, j;
//    wchar_t     ws[2];    
    char        ss[2];
//    int i = 0;
    vector<char>buf;

    for(j=0; j<sin.length(); ) {
//        printf("%d=%d>", j, sin[j]);
        if(sin.at(j)<208) {
            buf.push_back(sin.at(j));
//            *(*sout+i) = sin[j];
            ++j;
        } 
        else if(j<sin.length()-1){
            ss[1]=sin.at(j); 
            ss[0]=sin.at(j+1);
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
//    string s(buf.begin(), buf.end());
    sout.assign( buf.begin(), buf.end() );
}

void sympad( string& sin, char cpad, int16_t sizef ) {
    int16_t len;
//    cout<<"old length "<<sin.length()<<" "<<sin<<" | font "<<sizef<<" "<<_dsp_width<<" "<<_dsp_width/6<<endl;
    switch(sizef) {
        case 1: len = int(_dsp_width/6-0.5) - sin.length(); break;
        case 2: len = int(_dsp_width/8-0.5) - sin.length(); break;
        case 3: len = int(_dsp_width/12-0.5) - sin.length(); break;
        case 4: len = int(_dsp_width/16-0.5) - sin.length(); break;
    }
    if(len > 0) sin.append( len, cpad );
//    cout<<"new length "<<sin.length()<<" "<<sin<<" |"<<endl;
}

void view::println( string& sin ) {
    string  buf, son("{"), soff("}");
//    char    buff[100];
//    int16_t len;
    int16_t nfont;
     
    string sfont("fonts");
    getproperty( m_curpage, sfont, nfont );
 //   cout<<"println "<<m_curpage<<" "<<sfont<<" "<<nfont<<endl;
    to_866( sin, buf );
    assignValues(buf, son, soff);
    sympad( buf, ' ', nfont );
   
    Noritake_VFD_GU3000::println(buf.c_str());
//    assignValues(buf, son, soff, buff, len);
//    Noritake_VFD_GU3000::println( buff, len );
}

//
// функция замены тэгов в строках {tag} на их значения
//
/*
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
*/
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
                vector<string> vc;
                strsplit(na, ':', vc);
                na = vc.at(0);
                
//                cout<<" vc="<<vc.size();
//                for(int j=0;j<vc.size();j++) cout<<" "<<vc.at(j)<<" ";cout<<" \n ";
                if(getparam( na.c_str(), va ) == EXIT_SUCCESS) {
                    if(vc.size() > 1) {
                        vector<string> fld;
                        if(strsplit(vc.at(1), '.', fld) > 1 ) {
//                            cout<<" fld="<<fld.size();
//                            for(int j=0;j<vc.size();j++) cout<<" "<<fld.at(j)<<" "; cout<<va;
                            stringstream ss;
                            ss<<fixed<<setprecision(fld.at(1).size())<<atof(va.c_str());
                            va = ss.str();
//                            cout<<" "<<fld.at(1).size()<<" "<<ss.str()<<" "<<va<<endl;
                        }
                    }
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
void view::outview(int16_t ndisp=-1) {
    int16_t     nd = (ndisp>=0)? ndisp: m_curpage;
    int16_t     nr;
    pagestruct  &pg = pages.at(nd);
    string      sout;
    string      sprop;
    int16_t     nfont;
     
    string sfont("fonts");
    int16_t rc=getproperty( nd, sfont, nfont );
//    cout<<"page "<<nd<<" "<<sfont<<" "<<nfont;

    for(nr = 0; nr <= pg.rows.size(); ++nr) {
       if(!nr) {
            GU3000_setFontSize( _6x8Format, 1, 1 );
            GU3000_boldOn();
            GU3000_setCursor( 0, 0 );
            sprop = "caption";
            getproperty( sprop, sout ); 
//            cout<<" sprop = "<<sprop<<" sout = "<<sout<<" font="<<nfont<<endl;
            println( sout );
            sout = "";
            sympad( sout, '-', 1 ); println( sout );
       }
       else {
            GU3000_setFontSize( (nfont==2)? _8x16Format: _6x8Format, 1, 1 );
            GU3000_boldOff();
            int16_t nrow = nr-1;
            sout = pg.rows.at(nrow);
            
//            sympad( sout, ' ', nfont );
            if( nrow == pg.rowget() ) GU3000_invertOn();
            
            println( sout );
            if( nrow == pg.rowget() ) GU3000_invertOff();
        }
    }
    sout = "";
    println( sout );

    GU3000_setFontSize( _6x8Format, 1, 1 );
    GU3000_setCursor( 0, 120 );
    sprop = "bottom";
    getproperty( sprop, sout ); 
//    cout<<"sprop = "<<sprop<<" sout = "<<sout<<endl;
    println( sout );
}

//
// поток обработки консоли 
//
void* viewProcessing(void *args) {
    cton t;    
    const int16_t _refresh = 200;
    dsp.setMaxPage( dsp.pagessize()-1 );
    sleep(2);

    while (fParamThreadInitialized) {
        dsp.keymanage();
        if( !t.isTiming() || t.isDone() ) { dsp.outview(); t.reset(); t.start(_refresh); }
        usleep(100000);
    }
    return EXIT_SUCCESS;
}

void view::keymanage() {
    std::string     stag; 
    double          rval;
    int16_t         nqu;
    timespec*       tts;
    static bool     binit = false;
    const char*     btns[] = { "HS01", "HS02", "HS03", "HS04", "HS05", "HS06" };
    static bool     olds[] = { false,  false,  false,  false,  false,  false  };      
    uint16_t        bbtn = 0;
    pagestruct      &pg = pages.at(m_curpage);

//    cout << " key size = "<<sizeof(btns)/sizeof(char*)<<" button str size = "<<sizeof(m_btn)<<endl;
    for(int j=0; j<sizeof(btns)/4; j++) {         // read buttons states on front panel of box
        getparam( btns[j], rval, nqu, tts );
        bbtn |= ( (rval != 0 && !olds[j]) << j );
        olds[j] = (rval != 0);
    }
    memcpy( (void*)&m_btn, &bbtn, sizeof(cbtn) );
    // processing buttons according to the display mode
    switch(m_mode) {
        case _view_mode:
            if(m_btn.esc) {
                cout << "btn Esc\n";
//                m_visible = !m_visible;
//                (m_visible) ? GU3000_displayOn() : GU3000_displayOff();
                int n = m_curpage; 
                pagePrev();
                if( n > m_maxpage ) pages.pop_back();
            }
            if(m_btn.left) {            
                cout << "btn Left\n";
                pageBack();
            }
            if(m_btn.right) {            
                cout << "btn Right\n";
                pageNext();
            }
            if(m_btn.down) {            
                cout << "btn Down\n";
                pg.rownext();
            }  
            if(m_btn.up) {            
                cout << "btn Up\n";
                pg.rowprev();
            }                    
            if(m_btn.enter) {            
                cout << "btn Enter\n";
                gotoDetailPage();
            }                    
            break;
        case _task_mode:
            if(m_btn.esc) {
//                m_visible = !m_visible;
//                (m_visible) ? GU3000_displayOn() : GU3000_displayOff();
                int n = m_curpage; 
                pagePrev();
                if( n > m_maxpage ) pages.pop_back();
                m_mode = _view_mode;
            }
            break;
    }
}

string getTagName( const char* sin ) {
    string  s(sin);

    std::size_t found1 = s.find_last_of( "{" );
    std::size_t found2 = s.find_last_of( "}" );
    
    if( found1 != std::string::npos && found2 != std::string::npos && found1 < found2 ) {
        s = s.substr( found1+1, found2-found1-1 );
    }
    return s;
}

string buildLine( const char* smess, const string& sin ) {

    string na = getTagName( sin.c_str() );
    string va( "no value" );

    if( getparam( na.c_str(), va ) == EXIT_SUCCESS ) {
        na = smess+va;
    }

    return na;
}

void view::gotoDetailPage() {
    pagestruct& pg = pages.at(m_curpage);
    string s = getTagName(pg.rows.at(pg.rowget()).c_str());
    
    definedspline( m_maxpage+2, 1, s );         
    definedspline( m_maxpage+2, 2, "Значение  {"+s+"}"  );   
    definedspline( m_maxpage+2, 3, "Задание   <input>"  );
    pages.at(m_maxpage+1).setprev( m_curpage );
    s="2";
    string sfont("fonts");
    setproperty( m_maxpage+1, sfont, s );
    GU3000_clearScreen();
    m_curpage = m_maxpage+1;
    m_mode = _task_mode;
}

