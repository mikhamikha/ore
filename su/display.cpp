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
void view::definedspline( int16_t nd, int16_t nr, const char* sr, const std::string& attr="" ) {
    if( nd >= 0 && nr >= 0 ) {
        addpages(nd);        
        pages.at(nd).setproperty( nr, sr, attr );
        /*
        if( pages.at(nd).rows.size() <= nr ) {
            properties s;
            while( pages.at(nd).rows.size() <= nr ) { pages.at(nd).rows.push_back(s); pages.at(nd).attr.push_back(s); } 
        }
        string strim(" \t");
        pages.at(nd).rows.at(nr) = trim( sr, strim );
        pages.at(nd).attr.at(nr) = trim( attr, strim );
        cout<<"define dsp "<<nd<<" row "<<nr<<" content "<<sr \
            <<" dspsize "<<pages.size()<<" rows "<<pages.at(nd).rows.size()<<endl;
            */
    }
}

//
// перевод кодировки русских символов в CP866  
//
void to_866(string& sin, string& sout) {
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

void view::println( string& sin, bool setValue=true, bool last=false ) {
//    char    buff[100];
//    int16_t len;
    int16_t nfont;
    string buf; 
    string sfont("fonts");
    getproperty( m_curpage, sfont, nfont );
//   cout<<"println "<<m_curpage<<" "<<sfont<<" "<<nfont<<endl;
    buf = sin;
//    to_866( sin, buf );
//    string  buf, son("{<"), soff("}>");
//    if( setValue ) assignValues(buf, son, soff);
    sympad( buf, ' ', nfont );
   
    if(last) Noritake_VFD_GU3000::print(buf.c_str());
    else Noritake_VFD_GU3000::println(buf.c_str());
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
int16_t assignValues(cparam &p, string& subject, const string& sop, const string& scl) {
    size_t      nbe = 0, nen = 0, found;
    int16_t     rc=EXIT_SUCCESS;
    bool        fgo = true;

    while(fgo) {
        nbe = subject.find_first_of(sop, nen); 
        fgo = (nbe != std::string::npos);
        found = sop.find(subject[nbe]);
        if( fgo && found != std::string::npos && found<scl.size() && \
                (nen = subject.find(scl[found], nbe)) != std::string::npos ) {
            string na(subject, nbe+1, nen-nbe-1);
            string va = "нет значения", vatmp;
            
//            if( subject[nbe]=='{' ) {                                               // подставляем значение свойства   
                vector<string> vc;
                strsplit(na, ':', vc);
                na = vc.at(0);
//                cout<<"dsp vc size="<<vc.size();
//                for(int j=0; j<vc.size(); ++j) cout<<" "<<vc[j]; cout<<endl;
                if(p.getproperty( na.c_str(), vatmp ) == EXIT_SUCCESS ) {
                    to_866(vatmp, va);
                    if(vc.size() > 1) {
                        vector<string> fld;
                        stringstream ss;
                        double rVal = atof(va.c_str());
                        if( na!="task" || rVal>=0 ) {
                            int16_t ns = strsplit(vc.at(1), '.', fld);
                            int ch=0;
    //                        cout<<"dsp fld size="<<vc.size();
    //                        for(int j=0; j<fld.size(); ++j) cout<<" "<<fld[j]; cout<<endl;   
                            if( fld.at(0).size() ) ch = fld.at(0)[0];
                            if( ch && isdigit((ch)) ) {
                                ss<<setw(atoi(fld.at(0).c_str()))<<left;
                                ch = 0;
                                if( ns > 1 && fld.at(1).size() ) ch = fld.at(1)[0];
                                if( ch && isdigit(ch) ) {
                                    ss<<fixed<<setprecision(atoi(fld.at(1).c_str()))<<rVal;
                                }
                                else ss<<va;
                                va = ss.str();
                            }
                        }
                    }
                }
                else va="-1";
/*            }
            else if( subject[nbe]=='<' ) {                                          // подставляем значение задания
                pagestruct* pg = dsp.curpage();
                if(pg) va = to_string(pg->gettask());
            }
*/
            subject.replace( nbe, nen-nbe+1, va );
            nen = nbe+va.length();
        }
        else fgo =false;
    }
    return rc;
}

//
// функция вывода сконфигурированного кадра на дисплей
//
void view::outview(int16_t ndisp=-1) {
    int16_t     nd = (ndisp>=0)? ndisp: m_curpage;
    int16_t     nr;
    pagestruct  &pg = pages.at(nd);
    string      sout, snam, stmp;
    string      sprop;
    int16_t     nfont;
    string      buf, bufa, son("{<"), soff("}>");
        
    string sfont("fonts");
    int16_t rc=getproperty( nd, sfont, nfont );
//    cout<<"page "<<nd<<" "<<sfont<<" "<<nfont;
    
    GU3000_setFontSize( (nfont==2)? _8x16Format: _6x8Format, 1, 1 );
    GU3000_setCursor( 0, 0 );

    for(nr = 0; nr < pg.rowssize(); ++nr) {
        pg.getproperty( nr, "format", bufa );       
        to_866( bufa, buf );
        if( pg.getproperty( nr, "tag", snam )==EXIT_SUCCESS ) {
            cparam *p = getparam( snam.c_str() );
            if( p ) {
                assignValues( *p, buf, son, soff );
            }
        }
        if( nr == pg.rowget() ) { GU3000_invertOn(); GU3000_boldOn(); }            
        println( buf, true );
        if( nr == pg.rowget() ) { GU3000_invertOff(); GU3000_boldOff(); } 
    }
}

//
// поток обработки консоли 
//
void* viewProcessing(void *args) {
    cton t;    
    const int16_t _refresh = 200;
    cout<<"start dsp thread\n";
    dsp.setMaxPage( dsp.pagessize()-1 );
    sleep(5);

    while (fParamThreadInitialized) {
        dsp.keymanage();
        if( !t.isTiming() || t.isDone() ) { dsp.outview(); t.reset(); t.start(_refresh); }
        usleep(100000);
    }
    return EXIT_SUCCESS;
}

void view::keymanage() {
//    std::string     stag; 
    static bool     first  = true; 
    int16_t         nval[] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    int16_t         nqu;
    timespec*       tts;
    static bool     binit = false;
    const char*     btns[] = { "HS01", "HS02", "HS03", "HS04", "HS05", "HS06" };
    static bool     olds[] = { false,  false,  false,  false,  false,  false  };      
    uint16_t        bbtn = 0;
    pagestruct      &pg = pages.at(m_curpage);

//    cout << " key size = "<<sizeof(btns)/sizeof(char*)<<" button str size = "<<sizeof(m_btn)<<endl;
//    cout << " keys ";
    for(int j=0; j<sizeof(btns)/4; j++) {         // read buttons states on front panel of box
//      getparam( btns[j], rval, nqu, tts, 1 );
        getparamcount( btns[j], nval[j] );
//        cout<<nval[j]<<" ";
        bbtn |= ( (nval[j] != 0 && !olds[j]) << j );
        olds[j] = ( (nval[j] != 0) && !first );
    }
//    cout<<endl;
    if(first) { first = false; return; }
    memcpy( (void*)&m_btn, &bbtn, sizeof(cbtn) );
    // processing buttons according to the display mode
    switch(m_mode) {
        case _view_mode:
            if(m_btn.esc) {                                 // 0
                cout << "btn Esc\n";
//                m_visible = !m_visible;
//                (m_visible) ? GU3000_displayOn() : GU3000_displayOff();
                int n = m_curpage; 
                pagePrev();
                if( n > m_maxpage ) pages.pop_back();
            }
            if(m_btn.left) {                                // 1
                cout << "btn Left\n";
                pageBack();
            }
            if(m_btn.right) {                               // 2
                cout << "btn Right\n";
                pageNext();
            }
            if(m_btn.down) {                                // 4
                cout << "btn Down\n";
                pg.rownext(nval[4]);
            }  
            if(m_btn.up) {                                  // 5
                cout << "btn Up\n";
                pg.rowprev(nval[5]);
            }                    
            if(m_btn.enter) {                               // 3
                cout << "btn Enter\n";
                gotoDetailPage();
            }                    
            break;
        case _task_mode:
            double n = pg.gettask();
            double rem = n-floor(n);
            n = floor(n) + ((rem<0.3) ? 0 : (rem>0.7) ? 1 : 0.5); 
            if(m_btn.esc) {                                 // 0
//                m_visible = !m_visible;
//                (m_visible) ? GU3000_displayOn() : GU3000_displayOff();
                int n = m_curpage; 
                pagePrev();
                if( n > m_maxpage ) pages.pop_back();
                m_mode = _view_mode;
            }
            if(m_btn.down) {                                // 4
                cout << "btn Down\n";
//                n = n-nval[4]*5;
//                pg.settask( n );
                string sn, srch="FV", srpl="MV";
                int16_t mot, mode;
                sn = pg.m_tag;
                cparam *p = getparam( sn.c_str() );   
                if( p && p->getproperty("motion",mot)==EXIT_SUCCESS && mot==_no_motion ) {
                    replaceString( sn, srch, srpl );
                    cparam *p1 = getparam( sn.c_str() );
                    pthread_mutex_lock( &mutex_param );   
                    if( p1 && (mode=p1->getvalue())!=_manual_pulse ) {
                        sn = "/top/"+pg.m_tag+"/task";
                        p->settask(0-_close);               // дадим толчок на закрытие
                    }
                    pthread_mutex_unlock( &mutex_param );   
               }
            }  
            if(m_btn.up) {                                  // 5
                cout << "btn Up\n";
//                n = n+nval[5]*5;
//                pg.settask( n );
                string sn, srch="FV", srpl="MV";
                int16_t mot, mode;
                sn = pg.m_tag;
                cparam *p = getparam( sn.c_str() );   
                if( p && p->getproperty("motion",mot)==EXIT_SUCCESS && mot==_no_motion ) {
                    replaceString( sn, srch, srpl );
                    cparam *p1 = getparam( sn.c_str() );
                    pthread_mutex_lock( &mutex_param );   
                    if( p1 && (mode=p1->getvalue())!=_manual_pulse ) {
                        sn = "/top/"+pg.m_tag+"/task";
                        p->settask(0-_open);                // дадим толчок на открытие
                    }
                    pthread_mutex_unlock( &mutex_param );   
               }
            }                    
            if(m_btn.left) {                                // 1
                cout << "btn Left\n";
                n = n-nval[1]*5;
                pg.settask( n );
                pg.p->settask( n, false );
            }
            if(m_btn.right) {                               // 2
                cout << "btn Right\n";
                n = n+nval[2]*5;
                pg.settask( n );
                pg.p->settask( n, false );
            }
            if(m_btn.enter) {                               // 3
                cout << "btn Enter\n";
                string sn, sval;
//                cout<<pg.rows[pg.rowget()]<<" "<<getTagName(pg.rows[pg.rowget()].c_str())<<endl;
//                cout<<"yahoo\n";
//                string sn = getTagName(pg.rows[pg.rowget()].c_str());
//                sn = sn.substr( 0, sn.find(':') );
                sn = "/top/"+pg.m_tag+"/task";
                sval = to_string( pg.gettask() );
                taskparam( sn, sval );
            }                    
            break;
    }
}

string getTagName( const char* sin ) {
    string  s(sin);

    std::size_t found1 = s.find_last_of( "{<" );
    std::size_t found2 = s.find_last_of( "}>" );
    
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
// 
// создание временного окна для ввода задания для параметра
// 
void view::gotoDetailPage() {
    pagestruct& pg = pages.at(m_curpage);
    int16_t row = pg.rowget();
    string ss, sform;// = pg.attr.at(row); 
//    if( ss.size() && ss[0]=='i' ) {
    string snam;// = getTagName(pg.rows.at(row).c_str());
    
    if( pg.getproperty( row, "tag", snam )==EXIT_SUCCESS &&                                 // если получили имя тэга
            pg.getproperty( row, "task", ss )==EXIT_SUCCESS && ss=="1" ) {                  // и для него возможно вводить задание
        int16_t n = m_maxpage+1;                                                            // создаем страницу
        pg.getproperty( row, "format", sform ); 

        ss = "Значение  {value:5.1}";

        definedspline( n, 3, "tag", snam );
        definedspline( n, 3, "format", ss );
        ss = "Задание   <task:5.1}>";
        definedspline( n, 5, "tag", snam );
        definedspline( n, 5, "format", ss );
            
//        s = s.substr(0, s.find(':'));
        ss = "Режим управления";
        definedspline( n, 0, "format", ss );
        definedspline( n, 2, "format", snam );         
        pages.at(n).setprev( m_curpage );
        pages.at(n).rowset(5);
        pages.at(n).m_tag = snam;
        
        pages.at(n).p = getparam( snam.c_str() );
        if( pages.at(n).p ) pages.at(n).settask( pages.at(n).p->gettask() );  
        
        getparam( snam.c_str(), ss );
        cout<<"gotodetail init "<<snam<<" curPg="<<m_curpage<<" row="<<row<<" val = "<<ss;
        pages.at(n).settask( atof(ss.c_str()) );
        ss="2";
        string sfont("fonts");
        setproperty( n, sfont, ss );
        GU3000_clearScreen();
        m_curpage = n;
        m_mode = _task_mode;
    }
}

