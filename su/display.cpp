#include <typeinfo>
#include "display.h"
#include "tagdirector.h"
#include "unit.h"

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
    }
}

//
// перевод кодировки русских символов в CP866  
//
void to_866(string& sin, string& sout) {
    uint16_t    j, n;
    char        ss[2];
    vector<char>buf;

    for(j=0; j<sin.length(); ) {
//        printf("%d=%d>", j, sin[j]);
        if(sin.at(j)<208) {
            buf.push_back(sin.at(j));
            ++j;
        } 
        else if(j<sin.length()-1){
            ss[1]=sin.at(j); 
            ss[0]=sin.at(j+1);
            memcpy( &n, ss, sizeof(ss) );

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
    }
    sout.assign( buf.begin(), buf.end() );
}

void sympad( string& sin, char cpad, int16_t sizef ) {
    int16_t len=0;
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
int16_t assignValues(void* vv, string& subject, const string& sop, const string& scl) {
    size_t      nbe = 0, nen = 0, found;
    int16_t     rc=EXIT_SUCCESS;
    bool        fgo = true;
    ctag*       p = (ctag *)vv; 

    while(fgo) {
        nbe = subject.find_first_of(sop, nen);                                  // ищем открывающую скобку
        fgo = (nbe != std::string::npos);
        if(fgo) found = sop.find(subject[nbe]);                                 // определяем тип открывающей скобки
            fgo = (found != std::string::npos);
        if( fgo &&  found<scl.size() &&         \
                (nen = subject.find(scl[found], nbe)) != std::string::npos ) {  // ищем закрывающую скобку с тем же типом
            string na(subject, nbe+1, nen-nbe-1);
            string va = "нет значения", vatmp;
            
            vector<string> vc;
            strsplit(na, ',', vc);                                              // ищем признаки форматирования
            na = vc.at(0);
            if(na.size()) {                                                     // имя непустое
                if( na[0]=='@' ) {                                              // нужно вывести описание значения
                    na = na.substr(1);
                    p->getdescvalue( na, vatmp );
                    to_866(vatmp, va);                                          // перекодируем
                }          
                else if(p->getproperty( na.c_str(), vatmp ) == EXIT_SUCCESS ) { // выводим само значение
                    to_866(vatmp, va);                                          // перекодируем
                    if(vc.size() > 1) {                                         // есть признаки форматирования
                        vector<string> fld;
                        stringstream ss;
                        double rVal = ( isdigit(va[0]) ) ? atof(va.c_str()) : 0;
                        if( na!="task" || rVal>=0 ) {
                            int16_t ns = strsplit(vc.at(1), '.', fld);
                            int ch=0;
                            if( fld.at(0).size() ) ch = fld.at(0)[0];
                            if( ch && isdigit((ch)) ) {
                                bool rightF = (vc.size() > 2 && (vc[2]=="R" || vc[2]=="r"));
                                ss<<setw(atoi(fld.at(0).c_str()))<<(rightF? std::right: std::left);
                                ch = 0;
                                if( ns > 1 && fld.at(1).size() ) ch = fld.at(1)[0];
                                if( ch && isdigit(ch) ) {
                                    ss<<fixed<<setprecision(atoi(fld.at(1).c_str()))<<rVal;
                                }
                                else ss<<va;
                                va = ss.str();
//                                cout<<p.getname()<<" "<<fld[0]<<" "<<va<<"|"<<endl;
                           }
                        }
                    }
                }
            }
            subject.replace( nbe, nen-nbe+1, va );                              // меняем {} на значение
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
    /*int16_t rc=*/getproperty( nd, sfont, nfont );
//    cout<<"page "<<nd<<" "<<sfont<<" "<<nfont;
    
    GU3000_setFontSize( (nfont==2)? _8x16Format: _6x8Format, 1, 1 );
    GU3000_setCursor( 0, 0 );

    for(nr = 0; nr < pg.rowssize(); ++nr) {
        pg.getproperty( nr, "format", bufa );       
        to_866( bufa, buf );
        if( pg.getproperty( nr, "tag", snam )==EXIT_SUCCESS ) {
            ctag *p = tagdir.gettag( snam.c_str() );
            if( p ) {
                assignValues((void*)p, buf, son, soff );
            }
        }
        if( nr == pg.rowget() ) { 
            GU3000_invertOn(); 
            GU3000_boldOn(); 
            buf = ((m_mode==_task_mode)?"*":"")+buf; 
        }            
        println( buf, true );
        if( nr == pg.rowget() ) { 
            GU3000_invertOff(); 
            GU3000_boldOff(); 
        } 
    }
}

//
// поток обработки консоли 
//
void view::run() {
    cton t;    
    const int16_t _refresh = 200;

    cout << "start display thread " << endl;
    dsp.setMaxPage( dsp.pagessize()-1 );
    sleep(5);

    while (ftagThreadInitialized) {
        dsp.keymanage();
        if( !t.isTiming() || t.isDone() ) { dsp.outview(); t.reset(); t.start(_refresh); }
        usleep(100000);
    }
    cout << "end display thread " << endl;
   
//    return EXIT_SUCCESS;
}

void view::keymanage() {
//    std::string     stag; 
    static bool     first  = true; 
    int16_t         nval[] = { 0, 0, 0, 0, 0, 0, 0, 0 };
//    int16_t         nqu;
//    timespec*       tts;
//    static bool     binit = false;
    const char*     btns[] = { "HS01", "HS02", "HS03", "HS04", "HS05", "HS06" };    // кнопки
    const char*     btns_cnt[] = { "HC01", "HC02", "HC03", "HC04", "HC05", "HC06" };// и счетчики нажатий
    static bool     olds[] = { false,  false,  false,  false,  false,  false  };      
    uint16_t        bbtn = 0;   // отслеживание новых нажатых кнопок
    uint16_t        bbtn1 = 0;  // отслеживание нажатых кнопок
    pagestruct      &pg = pages.at(m_curpage);
    ctag            *ptag;

//    cout << " key size = "<<sizeof(btns)/sizeof(char*)<<" button str size = "<<sizeof(m_btn)<<endl;
//    cout << " keys ";
    for(size_t j=0; j<sizeof(btns)/4; j++) {            // read buttons states on front panel of box
        ptag = tagdir.gettag(btns[j]);
        if(ptag) nval[j] = ptag->getvalue();
        bbtn1 |= ( (nval[j] != 0) << j );    // 
//        cout<<nval[j]<<" ";
        tagdir.gettagcount( btns_cnt[j], nval[j] );     // read number of keystrokes
//        cout<<nval[j]<<" ";
        bbtn |= ( (nval[j] != 0 && !olds[j]) << j );    // 
        olds[j] = ( (nval[j] != 0) && !first );
    }
//    cout<<endl;
    if(first) { 
        first = false; 
        if(  m_tsleeper.getPreset() ) m_tsleeper.start();
        return; 
    }
    memcpy( (void*)&m_btn, &bbtn, sizeof(cbtn) );
    if( bbtn && m_mode != _sleep_mode && m_tsleeper.getPreset() ) {  // если жались кнопки, то перезапустим таймер сна
        m_tsleeper.reset(); 
        m_tsleeper.start(); 
    } 
    if( m_tsleeper.isDone() && m_tsleeper.getPreset() ) {
        m_mode = _sleep_mode;                           // уснем, если никому не нужен
        m_tsleeper.reset();
        GU3000_displayOff();
        cout<<"I went to sleep..\n";
    }
    if( (bbtn1^m_unlockkey)==0 && m_mode == _sleep_mode ) {    // если ключ совпал,
        m_tsleeper.start();                                     // выходим из спящего режима
        GU3000_displayOn();    
        m_mode = _view_mode;   
        cout<<"I woke up!\n";
    }
    // processing buttons according to the display mode
    switch(m_mode) {
        case _view_mode:
            if(m_btn.esc) {                             // 0
                cout << "btn Esc\n";
//                m_visible = !m_visible;
//                (m_visible) ? GU3000_displayOn() : GU3000_displayOff();
                string sn;
                int16_t nm;
                ctag* pt;
                if( pg.getproperty(pg.rowssize()-1,"tag",sn)==_exOK 
                        && sn.substr(0, 2)=="FV"
                        && (pt=getaddr(sn))!=NULL
                        && pt->getproperty("status", nm)==_exOK
                        && nm>=_vlv_open && nm<=_vlv_closing) {
                    string srpl="MV", srch="FV";
                    replaceString( sn, srch, srpl ); 
                    if((pt=getaddr(sn))!=NULL) pt->setvalue(_not_proc);
                }
                else if( pg.getproperty(pg.rowssize()-1,"tag",sn)==_exOK 
                        && sn.substr(0, 2)=="NC"
                        && (pt=getaddr(sn))!=NULL
                        && pt->getproperty("status", nm)==_exOK
                        && nm==_p_running ) { 
                        ctag* pt1;
                        string srpl="MN", srch="NC";
                        replaceString( sn, srch, srpl ); 
                        if((pt1=getaddr(sn))!=NULL) pt1->setvalue(_manual);
                        pt->settask( 0 );
                }
                else {
                    int n = m_curpage; 
                    pagePrev();
                    if( n > m_maxpage ) pages.pop_back();
                }
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
            if(m_btn.esc) {                                     // 0
/*                int n = m_curpage; 
                pagePrev();
                if( n > m_maxpage ) pages.pop_back();*/
//                pg.p = NULL;
                m_mode = _view_mode;                            // возврат в режим просмотра
            }
            if( pg.p ) {                                                // параметр не NULL
                string sn;
                ctag* p = (ctag *)(pg.p);
                int16_t mode;//mot, 
                double mine = p->getmineng();
                double maxe = p->getmaxeng();
                sn = p->getname();
                if( sn.substr(0, 2)=="FV" ) {                           // ручное управление клапаном
                    string srpl="MV", srch="FV";
                    replaceString( sn, srch, srpl );
                    ctag *p1 = tagdir.gettag( sn.c_str() );
                    //if( p->getproperty("motion", mot)==EXIT_SUCCESS && mot==_no_motion ) {
                    if( p->getquality()==OPC_QUALITY_GOOD ) {
                        if( m_btn.down || m_btn.up ) {                                                      // 4 или 5
                            cout << ( (m_btn.down) ? "btn Down\n" : "btn Up\n" );
                            pthread_mutex_lock( &mutex_tag );   
                            if( p1 && (mode=p1->getvalue()) < _manual_pulse_open ) {
                                p1->setvalue(
                                        m_btn.down ? 
                                        _manual_pulse_close :           // дадим толчок на закрытие      
                                        _manual_pulse_open              // дадим толчок на открытие
                                        );   
                            }
                            pthread_mutex_unlock( &mutex_tag ); 
                        }
                        else
                        if( (m_btn.left || m_btn.right) && p1 && (mode=p1->getvalue()) == _manual ) {       // 1 или 2
                            cout << ( ( m_btn.left ) ? "btn Left\n" : "btn Right\n" );
                            double r =   p->gettask();
                            double rem = r/10 - floor(r/10);
                            r = 10 * (floor(r/10) + ((rem<0.3) ? 0 : (rem>0.7) ? 1 : 0.5));
                            r = r + ( ( m_btn.left ) ? -nval[1]*5 : nval[2]*5 );
                            r = max( r, mine );
                            r = min( r, maxe );
                            pthread_mutex_lock( &mutex_tag );   
                            p->settask( r, false );                  // сохраним задание клапану
                            pthread_mutex_unlock( &mutex_tag );   
                        }
                        else
                        if( m_btn.enter && p1 && (mode=p1->getvalue()) == _manual ) {   // 3
                            cout << "btn Enter\n";
                            double r;
                            pthread_mutex_lock( &mutex_tag );   
                            if( (r=p->gettask())!=p->getvalue() ) 
                                p->settask( r );                     // выполним сохраненное задание клапану
                            pthread_mutex_unlock( &mutex_tag );
                            m_mode = _view_mode;                      // возврат в режим просмотра
                        }                    
                    }
                }
                else
                if( sn.substr(0, 2)=="NC" ) {                           // ручное управление клапаном
                    string srpl="MN", srch="NC";
                    replaceString( sn, srch, srpl );
                    ctag *p1 = tagdir.gettag( sn.c_str() );
                    //if( p->getproperty("motion", mot)==EXIT_SUCCESS && mot==_no_motion ) {
                    if( p->getquality()==OPC_QUALITY_GOOD ) {
                        if( m_btn.down || m_btn.up ) {                                                      // 4 или 5
                            //cout << ( (m_btn.down) ? "btn Down\n" : "btn Up\n" );
                            double r =   p->gettask();
                            double rem = r - floor(r);
                            //cout<<"gettask r0="<<r<<" rem0="<<rem<<" dwn="<<nval[4]<<" up="<<nval[5];
                            //r = 10 * (floor(r/10) + ((rem<0.3) ? 0 : (rem>0.7) ? 1 : 0.5));
                            rem = ((rem<0.3) ? 0 : (rem>0.7) ? 1 : 0.5);
                            //cout<<" rem1="<<rem;
                            rem = rem + ( ( m_btn.down ) ? -nval[4]*0.5 : nval[5]*0.5 );
                            r = floor(r)+rem;
                            //cout<<" rem2="<<rem<<" r1="<<r<<" min="<<mine<<" max="<<maxe;
                            r = max( r, mine );
                            r = min( r, maxe );
                            //cout<<" r3="<<r<<endl;
                            pthread_mutex_lock( &mutex_tag );   
                            p->settask( r, false );                  // сохраним задание клапану
                            pthread_mutex_unlock( &mutex_tag );   
                        }
                        else
                        if( (m_btn.left || m_btn.right) && p1 && (mode=p1->getvalue()) == _manual ) {       // 1 или 2
                            cout << ( ( m_btn.left ) ? "btn Left\n" : "btn Right\n" );
                            double r =   p->gettask();
                            double rem = r/10 - floor(r/10);
                            r = 10 * (floor(r/10) + ((rem<0.3) ? 0 : (rem>0.7) ? 1 : 0.5));
                            r = r + ( ( m_btn.left ) ? -nval[1]*5 : nval[2]*5 );
                            r = max( r, mine );
                            r = min( r, maxe );
                            pthread_mutex_lock( &mutex_tag );   
                            p->settask( r, false );                  // сохраним задание клапану
                            pthread_mutex_unlock( &mutex_tag );   
                        }
                        else
                        if( m_btn.enter && p1 && (mode=p1->getvalue()) == _manual ) {   // 3
                            cout << "btn Enter\n";
                            double r;
                            pthread_mutex_lock( &mutex_tag );   
                            cout<<p->getname()<<" val="<<p->getvalue()<<" task="<<p->gettask()<<endl;;
                            if( (r=p->gettask())!=p->getvalue() ) 
                                p->settask( r );                     // выполним сохраненное задание клапану
                            pthread_mutex_unlock( &mutex_tag );
                            m_mode = _view_mode;                      // возврат в режим просмотра
                        }                    
                    }
                }
                else
                if( sn.substr(0, 2)=="MV" || sn.substr(0, 2)=="MN" ) { // задание режима клапана/насоса
                    mode = p->gettask();
                    pthread_mutex_lock( &mutex_tag );   
                    if( m_btn.down || m_btn.up || m_btn.left || m_btn.right ) {
                        cout<<"valve mode="<<mode<<" vmodes="<<vmodes.size();
                        int16_t add;
                        add = ((m_btn.down || m_btn.left) ? -1 : 1);                        
                        if( mode < int(vmodes.size()) ) {
                            int16_t idsp=0;
                            size_t i=0;
                            do {
                                string ss;
                                mode = (mode+add) % vmodes.size(); 
                                mode = (mode<0) ? vmodes.size()-abs(mode) : mode;
                                vmodes[mode].getproperty( "visible", idsp );
                                vmodes[mode].getproperty( "name", ss );   
                                cout<<" askmode="<<mode<<" "<<ss<<" vis="<<idsp<<endl;
                            } while( !idsp && i++<vmodes.size() );
                        }
                        p->settask( mode, false );
                    }
                    else
                    if( m_btn.enter ) {
                        p->setvalue( mode );
                        m_mode = _view_mode;                            // возврат в режим просмотра
                    }
                    else
                    if( m_btn.esc ) {
                        p->settask( p->getvalue(), false );
                        m_mode = _view_mode;                            // возврат в режим просмотра
                    }
                    pthread_mutex_unlock( &mutex_tag );   
                }
                else
                if( sn.substr(0, 1)=="P" ) {                            // задание уставки давления
                    double r=0;// =   floor(p->gettask());
                    //double rem = r/10 - floor(r/10);
                    //r = 10 * (floor(r/10) + ((rem<0.3) ? 0 : (rem>0.7) ? 1 : 0.5));
                    double perc = 0.2;//(maxe-mine)/100000.0;
                    double add = 0;
                    p->getproperty("task", r);
                    r = floor( r*10 )/10;
    //if( m_name.substr(0,4)=="PT11") 
            cout<<"gettask "<<r<<" min="<<mine<<" max="<<maxe<<endl;
                    if( m_btn.down || m_btn.up || m_btn.left || m_btn.right ) {         
                        if( m_btn.down ) { add -= perc; }
                        else if( m_btn.up ) { add += perc; }
                        else if( m_btn.left ) { add -= 5*perc; }
                        else if( m_btn.right ) { add += 5*perc; }

                        r += add;
                        r = max( r, mine );
                        r = min( r, maxe );
                        pthread_mutex_lock( &mutex_tag );   
                        p->settask( r, false );                      // сохраним задание давления
                        pthread_mutex_unlock( &mutex_tag ); 
                    }
                    else
                    if( m_btn.enter ) {                                 // 3
                        cout << "btn Enter\n";
                        double r;
                        pthread_mutex_lock( &mutex_tag );   
                        if( (r=p->gettask())!=p->getvalue() ) 
                            p->settask( r );                         // выполним сохраненное задание 
                        pthread_mutex_unlock( &mutex_tag );
                        m_mode = _view_mode;                         // возврат в режим просмотра
                    }                    
                }
            }  
            break;
    }
}
// 
// переход на окно с номером task
// 
void view::gotoDetailPage() {
    pagestruct& pg = pages.at(m_curpage);
    int16_t npg;
    int16_t row = pg.rowget();   
    
    if( pg.getproperty( row, "task", npg )==EXIT_SUCCESS ) { 
        if( npg > 0 ) {                                                 // есть детальный кадр
            pageDisplay( npg );
        }
        else
        if( npg==-1 ) {                                                 // если возможен ввод значения
            string snam;
            if( pg.getproperty( row, "tag", snam )==EXIT_SUCCESS ) {    // получим имя тэга
                ctag* p;
                if( (p=tagdir.gettag( snam.c_str() )) != NULL) {        // получим ссылку на тэг
                    pg.p = p;
                    p->settask( p->getvalue(), false );
                    m_mode = _task_mode;                                // перейдем в режим ввода значения
                }
            }           
        }
    }
}

