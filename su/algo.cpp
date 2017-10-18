#include "algo.h"

alglist algos;

// Получить значение
double getval(ctag* p) {
    double rval=-1;
    if( p ) rval = p->getvalue();

    return rval;
}

// Получить качество тэга
uint8_t getqual(ctag* p) {
    uint8_t nval=0;
    if ( p ) nval = p->getquality();

    return nval;
}

// получить ссылку на тэг по имени
ctag* getaddr(string& str) {
    ctag* p = tagdir.gettag( str.c_str() );
    cout<<"gettag "<<hex<<long(p);
    if(p) cout<<" name="<< p->getname();
    cout<<endl;

    return p;
}

// получить ссылку на юнит по имени
cunit* getaddrunit(string& str) {
    cunit* p = unitdir.getunit( str.c_str() );
    cout<<"getunit "<<hex<<long(p);
    if(p) cout<<" name="<< p->getname();
    cout<<endl;

    return p;
}

// проверка на NULL
bool testaddr(void* x) {
    return (x==NULL);
}

// вывод на экран
template <class T>
void printdata(T in) {
    cout<<' '<<in;
}

// Расчет перепада давлений МПа->кПа
double dPress( double in1, double in2 ) { 
    return (in1-in2)*1000.0;
}

int16_t calgo::init() {
    int16_t rc = _exOK;
    string  sin, sres, seq;
    
    cout<<" calgo init";  
    if( getproperty( "type", m_nType ) != _exOK || \
      (  getproperty( "args", sin ) != _exOK  && \
         getproperty( "res", sres ) != _exOK  && \
         getproperty( "equipments", seq ) != _exOK ) ) rc = _exBadParam;

    cout<<" rc="<<rc<<" type="<<m_nType<<" args="<<sin<<" res="<<sres<<endl;
    
    if( !rc ) {
        vector<string> arr;
        
        arr.clear();
        strsplit( sin, ';', arr );               
        for_each( arr.begin(), arr.end(), printdata<string> ); cout<<endl;
        m_args.resize( arr.size() );
        transform( arr.begin(), arr.end(), m_args.begin(), getaddr );              // получим ссылки на тэги
        cout<<"args"; for( tagvector::iterator it=m_args.begin(); it!=m_args.end(); ++it ) cout<<' '<<hex<<*it; cout<<endl;
        
        arr.clear();
        strsplit( sres, ';', arr );               
        for_each( arr.begin(), arr.end(), printdata<string> ); cout<<endl;
        m_res.resize(arr.size());
        transform( arr.begin(), arr.end(), m_res.begin(), getaddr );               // получим ссылки на тэги
        cout<<"res"; for( tagvector::iterator it=m_res.begin(); it!=m_res.end(); ++it ) cout<<' '<<hex<<*it; cout<<endl;
       
        arr.clear();
        strsplit( seq, ';', arr );               
        for_each( arr.begin(), arr.end(), printdata<string> ); cout<<endl;
        m_units.resize(arr.size());
        transform( arr.begin(), arr.end(), m_units.begin(), getaddrunit );           // получим ссылки на устройства
        cout<<"res"; for( unitvector::iterator it=m_units.begin(); it!=m_units.end(); ++it ) cout<<' '<<hex<<*it; cout<<endl;
       
        if( (!m_args.size() || find_if( m_args.begin(), m_args.end(), testaddr ) != m_args.end()) && \
            (!m_res.size() || find_if( m_res.begin(), m_res.end(), testaddr ) != m_res.end()) && \
            (!m_units.size() || find_if( m_units.begin(), m_units.end(), testaddr ) != m_units.end()) )  
            { rc = _exBadAddr; m_fInit = -1; }
        else m_fInit = 1;
    }
//    cout<<" args="<<args.size()<<" res="<<res.size()<<endl;    
    return rc;
}

int16_t calgo::solveIt() {
    double raw;
    uint8_t nqual;
    int16_t rc=_exOK;

    if( m_fInit>0 ) {
        switch( algtype(m_nType) ) {                    
            /*
                        ---------- Управление МЕХАНИЗМАМИ  ----------
            */
            case _unitProcessing: {
                    int16_t fEn;
                    getproperty( "enable", fEn );
                    if( !fEn ) break;                                               // если алгоритм запрещен -> выходим 
                    if( !m_nUnits ) getproperty("number", m_nUnits);                // считываем количество устройств                      
                    if( m_nUnits && m_nUnits<=m_units.size()  ) {
                        for( int16_t j=0; fEn && j<m_nUnits; j++ ) {
                            cunit* puni = (cunit*)m_units[j];
                            puni->getstate();                                       // получим состояние
                            switch( puni->getmode() ) {                             // управляем механизмом согласно режима
                                case _auto_pid:                                     // ПИД
                                    if( m_args.size() >= m_nUnits ) {
                                        puni->pidEval( m_args[j] );
                                    }                                
                                    break;
                                case _auto_time: 
                                    if( puni->gettype()==_valve && m_nUnits==2 && m_args.size() >= m_nUnits ) {
                                        int16_t jj = ((j+1) ^ _mask_v);             // вычислим номер второго клапана
                                        if(jj && jj<=m_nUnits) {
                                            jj--;
                                            cunit* poth = (cunit*)m_args[jj];
                                            if( poth->getmode()==_auto_time ) {
                                                 
                                            }
                                        }
                                    }
                                    break; 
                            }
                            if( puni->gettype()!=_valve || puni->valveCtrlEnabled() ) puni->control(); // запустим исполнение
                        }
                    }
                }
                break;
                /*
            case _timeValveControl:                                             // алгоритм управления клапанами по времени
                if( args.size() >= 6 && res.size() >= 2 ) {
                    ctag*   pmode1        = args[0];                            // режим клапана 1
                    ctag*   pmode2        = args[1];                            // режим клапана 2
                    ctag*   plso1         = args[2];                            // конечник открыто клапана 1
                    ctag*   plso2         = args[3];                            // конечник открыто клапана 2
                    ctag*   plsc1         = args[4];                            // конечник открыто клапана 1
                    ctag*   plsc2         = args[5];                            // конечник открыто клапана 2
                   
                    ctag*   psw           = res[0];                             // ключ выбора клапана 
                    ctag*   pvl1          = res[1];                             // клапан 1
                    ctag*   pvl2          = res[2];                             // клапан 2 
                    int32_t ntim; 
                    const int16_t rc2     = (args[3]->getquality()!=OPC_QUALITY_GOOD); 
                    static int16_t selv   = 0;
                    const int16_t lso1    = plso1->getvalue();
                    const int16_t lso2    = plso2->getvalue();
                    const int16_t lsc1    = plsc1->getvalue();
                    const int16_t lsc2    = plsc2->getvalue();
                   
                  
                    const int16_t sw      = psw->getvalue();                   
                    const int16_t sw_old  = psw->getoldvalue();                   
                    // текущий режим - авт по времени
                    const bool    fMode   = ( pmode1->getvalue()==_auto_time ) && ( pmode2->getvalue()==_auto_time );
                    // переход в авт режим по времени
                    const bool    fInMode = ( pmode1->getvalue()==_auto_time && pmode1->getoldvalue()!=_auto_time ) \
                                            || ( pmode2->getvalue()==_auto_time && pmode2->getoldvalue()!=_auto_time );
                    // выход из режима
                    const bool    fOutMode1 = ( pmode1->getvalue()!=_auto_time && pmode1->getoldvalue()==_auto_time );
                    const bool    fOutMode2 = ( pmode2->getvalue()!=_auto_time && pmode2->getoldvalue()==_auto_time );

                    if( fOutMode1 ) pmode2->setvalue( _manual );
                    if( fOutMode2 ) pmode1->setvalue( _manual );
                    if( fOutMode1 || fOutMode2 ) {                   
                        pmode2->setoldvalue( pmode2->getvalue() );
                        pmode1->setoldvalue( pmode1->getvalue() );
                    }
                    if( fInMode ) {
                        pmode1->setvalue(_auto_time);
                        pmode2->setvalue(_auto_time);
                        pmode1->setoldvalue(_auto_time);
                        pmode2->setoldvalue(_auto_time);
                        psw->setvalue(_no_valve);
                        m_twait.reset();
                        selv = 0;
                    }
                    const int16_t state = m_twait.isDone() ? _done_a : (m_twait.isTiming() ? _buzy_a : _idle_a);
                    if( fMode ) cout<<"timeValvecontrol"\
                        <<" mode1="<<pmode1->getoldvalue()<<"|"<<pmode1->getvalue()\
                        <<" mode2="<<pmode2->getoldvalue()<<"|"<<pmode2->getvalue()\
                        <<" Mode="<<fMode<<" inMode="<<fInMode<<" outMode="<<fOutMode1<<"|"<<fOutMode2\
                        <<" SelValve="<<sw<<" state="<<state<<endl;
                    if( !fMode ) { break; }                                                             // уйдем, если другой режим
                    if( !sw ) {
                        if( state == _idle_a ) {                                                        // если режим ожидания
                            if( !selv ) {                                                               // если клапан не выбран
                                int16_t lso = ( ( lso2*2 + lso1 ) & _mask_v );                          // проверим конеч открытия
                                switch( lso ) {
                                    case 0: 
//                                        if( pvl1->gettask()!=pvl1->getmaxeng())
                                            pvl1->settask( pvl1->getmaxeng() ); break;                  // не открыты - откроем 1
                                    case 1: break;                                                      // открыт 1
                                    case 2: break;                                                      // открыт 2
                                    case 3: lso=1; break;                                               // открыты оба - выбор 1 
                                }
                                selv = lso;                                                             // выберем открытый
                            }
                            cout<<"selv="<<selv;
                            if( selv ) {                                                                // если есть выбранный
                                ctag* pvl;
                                int16_t fLSO = ( selv==_first_v  ) ? lso1 : lso2;
                                int16_t fLSC = ( selv==_first_v  ) ? lsc2 : lsc1;
                                if( fLSO ) {                                                            // и он открыт
                                    pvl = ( selv==_first_v  ) ? pvl2 : pvl1;
                                    cout<<endl<<"autotime change "<<pvl->getname()<<" task="<<pvl->gettask()<<"|"<<pvl->getmineng();
//                                    if( pvl->gettask()!=pvl->getmineng())
                                    if( !fLSC ) pvl->settask( pvl->getmineng() );                       // закроем второй                    
                                    string s = string("autotime")+to_string( selv );
                                    getproperty( s.c_str(), ntim );   
                                    cout<<" timer="<<ntim<<endl;
                                    m_twait.start( ntim*1000 );                                         // запустим таймер
                                }
                                else {
                                    cout << "Алг auto_time: Нет конечника!\n";
                                }
                            }
                        }
                        else if( state == _done_a ) {                                                   // если время первого прошло
                            ctag* pvl = ( selv==_first_v  ) ? pvl2 : pvl1;
                            pvl->settask( pvl->getmaxeng() );                                           // откроем второй                    
                            selv ^= _mask_v;                                                            // сделаем основным 
                            m_twait.reset();                                                            // сбросим таймер
                        }
                    }
                }
                break;
                */
            // Расчет расхода по перепаду давления на сечении клапана   
            case _floweval:
                ctag* pout;   
//                cout<<" eval flow for delta pressure size="<<args.size()<<" | "<<res.size()<<endl;
                if( m_args.size() >= 5 && m_res.size() >= 1 && nqual==OPC_QUALITY_GOOD ) {
                    ctag* pfv  = (ctag*)m_args[0];  // valve mm opened
                    ctag* ppt1 = (ctag*)m_args[1];  // давление в пласте
                    ctag* ppt2 = (ctag*)m_args[2];  // давление у насоса
                    ctag* pdt  = (ctag*)m_args[3];  // density 
                    ctag* pkv  = (ctag*)m_args[4];  // Kv factor
                    pout = (ctag*)m_res[0];

                    double sq, r1=1, r11=4, ht1 = pfv->getvalue()-1, _tan = 0.0448210728500398; 
/*                    
                    cout<<"fv\tpt1\tpt2\tkv\tdt\tsq\tflow\n";
                    cout<<pfv->getname()<<'\t' <<ppt1->getname() <<'\t'<<ppt2->getname() <<'\t'<<pkv->getname() <<'\t'<<pdt->getname()<<endl;
                    cout<<fixed<<setprecision(2)<<pfv->getvalue()<<'\t'<<ppt1->getvalue()<<'\t'<<ppt2->getvalue()<<'\t'<<pkv->getvalue() \
                    <<'\t'<<pdt->getvalue()<<'\t';
*/                   
                    if( ht1 < 1 ) {
                        sq = (ht1<0)?0:3.14;
                    }
                    else {
                        double _add;
                        switch(int(ht1)) {
                            case 68: _add = 6.652; break;
                            case 69: _add = 19.654; break;
                            case 70: _add = 34.434; break;
                            case 71: _add = 50.266; break;
                            default: _add = 0;         
                        }
                        if( ht1 > 67 ) ht1 = 67;
                        sq = 3.14 + ht1 * ( r1 + 1 + ht1 * _tan )*2 + _add;     // расчет площади диафрагмы
                    }
                    
                    raw = pkv->getvalue()*sq*1e-6*sqrt((ppt1->getvalue()-ppt2->getvalue())*2*1e6/ \
                           ((pdt->getvalue() > 100 ) ? pdt->getvalue() : 1000) )*86400;
//                    cout<<sq<<'\t'<<raw<<endl;
                    pout->setrawval( raw );   
                    pout->setquality( OPC_QUALITY_GOOD );
                }
                else if(m_res.size()) pout->setquality(OPC_QUALITY_BAD);
                break;
            // Суммирование входных аргументов и запись в выходной
            case _summ:
//                cout<<" eval flow summary size="<<args.size()<<" | "<<res.size()<<endl;   
                if( m_args.size() >= 2 && m_res.size() >= 1 && nqual==OPC_QUALITY_GOOD ) { 
                    vector<double> arr_real;
                    ctag* pout = (ctag*)m_res[0];   
                    arr_real.resize( m_args.size() );
                    transform( m_args.begin(), m_args.end(), arr_real.begin(), getval );
                    raw = accumulate( arr_real.begin(), arr_real.end(), 0.0, plus<double>() );
                    pout->setrawval(raw);
                    pout->setquality( OPC_QUALITY_GOOD );
                }
                else if(m_res.size()) pout->setquality(OPC_QUALITY_BAD);   
                break;
            // Вычисление перепада давления: sub входных аргументов, mul 1000 и запись в выходной
            case _sub:
//                cout<<" eval deltaP size="<<args.size()<<" | "<<res.size()<<endl;   
                if( m_args.size() >= 2 && m_res.size() >= 1 && nqual==OPC_QUALITY_GOOD ) { 
                    vector<double> arr_real; 
                    ctag* pout = (ctag*)m_res[0];    
                    arr_real.resize( m_args.size() );
                    transform( m_args.begin(), m_args.end(), arr_real.begin(), getval );
//                    raw = accumulate( arr_real.begin(), arr_real.end(), 0.0, dPress );
                    pout->setrawval( dPress( arr_real[0], arr_real[1] ) );
                    pout->setquality( OPC_QUALITY_GOOD );
                }
                else if( m_res.size() ) pout->setquality(OPC_QUALITY_BAD);   
                break;
            default: rc = _exFail;;
        }
    }
    else rc = init();
    
    //    cout<<endl;
    return rc;
}

