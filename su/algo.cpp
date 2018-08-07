#include "algo.h"
#include "tagdirector.h"
#include "unitdirector.h"
#include "utils.h"

alglist algos;


// Расчет перепада давлений МПа->кПа
double dPress( double in1, double in2 ) { 
    return (in1-in2)*1000.0;
}

int16_t calgo::init() {
    int16_t rc = _exOK;
    string  sin, sres, seq;
    
    cout<<" calgo init";  
        getproperty( "type", m_nType );
        getproperty( "args", sin );
        getproperty( "res", sres );
        getproperty( "equipments", seq );

    cout<<" rc="<<rc<<" type="<<m_nType<<" args="<<sin<<" res="<<sres<<" equip="<<seq<<endl;
    
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
        transform( arr.begin(), arr.end(), m_units.begin(), getaddrunit );         // получим ссылки на устройства
        cout<<"equip"; for( unitvector::iterator it=m_units.begin(); it!=m_units.end(); ++it ) cout<<' '<<hex<<*it; cout<<endl;
       
        if( !testaddrlist(m_args) && !testaddrlist(m_res) && !testaddrlist(m_units) ) { rc = _exBadAddr; m_fInit = -1; }
        else m_fInit = 1;
    }
//    cout<<" args="<<args.size()<<" res="<<res.size()<<endl;    
    return rc;
}

int16_t calgo::solveIt() {
    double raw;
    uint8_t nqual=OPC_QUALITY_GOOD;
    vector<double> arr_real;
    vector<uint8_t> arr_qual;   
    int16_t rc=_exOK;
    
    getproperty( "enable", m_enable );
    if( m_enable ) {                                                                // если алгоритм запрещен -> 
    if( m_fInit>0 ) {
        arr_qual.resize( m_args.size() );
        transform( m_args.begin(), m_args.end(), arr_qual.begin(), getqual );
        nqual = accumulate( arr_qual.begin(), arr_qual.end(), 0, bit_or<uint8_t>() );
        switch( algtype(m_nType) ) {                    
            /*
                        ---------- Управление МЕХАНИЗМАМИ  ----------
            */
            case _unitProcessing: {
//                    cout<<"_unitProcessing arg="<<(m_args.size()?m_args[0]->getname():"")<<endl;
                    if( !m_nUnits ) getproperty("number", m_nUnits);                // считываем количество устройств                      
                    if( m_nUnits && (size_t)m_nUnits<=m_units.size()  ) {
                        for( int16_t j=0; j<m_nUnits; j++ ) {
                            cunit* puni = m_units[j];
                            // --- test --
                            puni->control( (m_args.size() >= (size_t)m_nUnits) ? &m_args: NULL );  // запустим исполнение ;  
                            // --- instead below ---
                            /*
                            if( puni->getstate()!=_exOK ) continue;                 // получим состояние
                            // -- обработку режимов перенести в unit.cpp --
                            switch( puni->getmode() ) {                             // управляем механизмом согласно режима
                                case _auto_pid:                                     // ПИД
                                    if( m_args.size() >= (size_t)m_nUnits ) {
                                        puni->pidEval( m_args[j] );
                                    }                                
                                break;
                            
                                case _auto_time: 
                                    if( puni->gettype()==_valve && m_nUnits==2 && \
                                            m_args.size() >= (size_t)m_nUnits*2 && m_res.size() ) {
                                        uint16_t sw = ((ctag*)m_res[0])->getvalue();
                                        int16_t jj = ((j+1) ^ _mask_v);             // вычислим номер второго клапана
                                        if(jj && jj<=m_nUnits) {
                                            jj--;
                                            cunit* poth = m_units[jj];
                                            if( poth && poth->getmode()==_auto_time && j<jj && !sw ) {     // если 1й клапан из 2х и нет управления
                                                const int16_t state = m_twait.isDone() ? _done_a : (m_twait.isTiming() ? _buzy_a : _idle_a);
                                                if( state == _idle_a ) {                                                        // если режим ожидания
                                                    if( !m_stor ) {                                                             // если клапан не выбран
                                                        int16_t lso = ( ( poth->isOpen()*2 + puni->isOpen() ) & _mask_v );      // проверим конеч открытия
                                                        switch( lso ) {
                                                            case 0: puni->open(); break;                                        // не открыты - откроем 1
                                                            case 1: break;                                                      // открыт 1
                                                            case 2: break;                                                      // открыт 2
                                                            case 3: lso=1; break;                                               // открыты оба - выбор 1 
                                                        }
                                                        m_stor = lso;                                                           // запомним открытый
                                                    }
                                                    cout<<"selected valve="<<m_stor<<endl;
                                                    if( m_stor ) {                                                              // если есть выбранный
                                                        cunit* pvl;
                                                        int16_t fLSO = ( m_stor==_first_v  ) ? puni->isOpen() : poth->isOpen();
                                                        int16_t fLSC = ( m_stor==_first_v  ) ? poth->isClose() : puni->isClose();
                                                        if( fLSO ) {                                                            // и он открыт
                                                            pvl = ( m_stor==_first_v  ) ? poth : puni;
                                                            cout<<endl<<"autotime change "<<pvl->getname()<<" task="<<pvl->gettask();
                        //                                    if( pvl->gettask()!=pvl->getmineng())
                                                            if( !fLSC ) pvl->close();                                           // закроем другой                    
                                                            jj = ( m_stor==_first_v  ) ? j : jj;
                                                            ctag* pt = m_args[jj+m_nUnits];                                     // вычислим тэг с временем
                                                            int32_t ntim;   
                                                            if( pt && (ntim = pt->getvalue())>0 ) {
                                                                cout<<" timer="<<ntim<<endl;
                                                                m_twait.start( ntim*1000 );                                     // запустим таймер
                                                            }
                                                        }
                                                        else {
                                                            cout << "Алг auto_time: Нет конечника!\n";
                                                        }
                                                    }
                                                }
                                                else if( state == _done_a ) {                                                   // если время первого прошло
                                                    cunit* pvl = ( m_stor==_first_v  ) ? poth : puni;
                                                    pvl->open();                                                                // откроем второй                    
                                                    m_stor ^= _mask_v;                                                          // сделаем основным 
                                                    m_twait.reset();                                                            // сбросим таймер
                                                }
                                            }
                                        }
                                    }
                                break;
                                    */
                            //}
                            /*
                            if( puni->gettype()!=_valve || puni->valveCtrlEnabled() ) puni->control(); // запустим исполнение ;
                            */
                        } 
                    } 
                } 
                break;
            // Расчет расхода по перепаду давления на сечении клапана   
            case _floweval:
//                    cout<<"_floweval"<<endl;
//                cout<<" eval flow for delta pressure size="<<args.size()<<" | "<<res.size()<<endl;
                if( m_args.size() >= 5 && m_res.size() >= 1 && testaddrlist(m_args) && testaddrlist(m_res) ) {
                    ctag* pfv  = (ctag*)m_args[0];  // valve mm opened
                    ctag* ppt1 = (ctag*)m_args[1];  // давление в пласте
                    ctag* ppt2 = (ctag*)m_args[2];  // давление у насоса
                    ctag* pdt  = (ctag*)m_args[3];  // density 
                    ctag* pkv  = (ctag*)m_args[4];  // Kv factor
                    ctag* pout = (ctag*)m_res[0];

                    double sq, r1=1, /*r11=4,*/ ht1 = pfv->getvalue()-1, _tan = 0.0448210728500398; 
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
                    pout->setquality( nqual );
                }
                else if(m_res.size()) ((ctag*)m_res[0])->setquality(OPC_QUALITY_BAD);
                break;
            // Суммирование входных аргументов и запись в выходной
            case _summ:
//                    cout<<"_summ"<<endl;
//                cout<<" eval flow summary size="<<args.size()<<" | "<<res.size()<<endl;   
                if( m_args.size() >= 2 && m_res.size() >= 1 && testaddrlist(m_args) && testaddrlist(m_res) ) { 
                    ctag* pout = (ctag*)m_res[0];   
                    arr_real.resize( m_args.size() );
                    transform( m_args.begin(), m_args.end(), arr_real.begin(), getval );
                    raw = accumulate( arr_real.begin(), arr_real.end(), 0.0, plus<double>() );
                    pout->setrawval(raw);
                    pout->setquality( nqual );
                }
                else if(m_res.size()) ((ctag*)m_res[0])->setquality( OPC_QUALITY_BAD );   
                break;
            // Вычисление перепада давления: sub входных аргументов, mul 1000 и запись в выходной
            case _sub:
//                    cout<<"_sub"<<endl;
//                cout<<" eval deltaP size="<<args.size()<<" | "<<res.size()<<endl;   
                if( m_args.size() == 2 && m_res.size() >= 1 && testaddrlist(m_args) && testaddrlist(m_res) ) {
                    ctag* pout = (ctag*)m_res[0];    
                    if( nqual==OPC_QUALITY_GOOD ) { 
                        arr_real.resize( m_args.size() );
                        transform( m_args.begin(), m_args.end(), arr_real.begin(), getval );
    //                    raw = accumulate( arr_real.begin(), arr_real.end(), 0.0, dPress );
                        pout->setrawval( dPress( arr_real[0], arr_real[1] ) );
                    }
                    pout->setquality( nqual );
                }
                else if( m_res.size() ) ((ctag*)m_res[0])->setquality( OPC_QUALITY_BAD );   
                break;
            default: rc = _exFail;
        }
    }
    else rc = init();
    } 
    //    cout<<endl;
    return rc;
}

