#include "algo.h"

alglist algos;

double getval(cparam* p) {
    double rval=-1;
    if( p ) rval = p->getvalue();

    return rval;
}

int8_t getqual(cparam* p) {
    int8_t nval=0;
    if ( p ) nval = p->getquality();

    return nval;
}

cparam* getaddr(string& str) {
    cparam* p = getparam( str.c_str() );
    cout<<"getparam "<<hex<<long(p);
    if(p) cout<<" name="<< p->getname();
    cout<<endl;

    return p;
}

bool testaddr(cparam* x) {
    return (x==NULL);
}

template <class T>
void printdata(T in) {
    cout<<' '<<in;
}

double dPress( double in1, double in2 ) {
    return (in1-in2)*1000.0;
}

int16_t calgo::init() {
    int16_t rc = EXIT_SUCCESS;
    string  sin, sres;
    
    cout<<" calgo init";  
    if( getproperty( "type", nType ) != EXIT_SUCCESS || \
        getproperty( "args", sin ) != EXIT_SUCCESS || \
        getproperty( "res", sres ) != EXIT_SUCCESS ) rc = EXIT_FAILURE;

    cout<<" rc="<<rc<<" type="<<nType<<" args="<<sin<<" res="<<sres<<endl;
    
    if( !rc ) {
        vector<string> arr_in, arr_out;
        strsplit( sin,  ';', arr_in  );               
        strsplit( sres, ';', arr_out ); 
        for_each( arr_in.begin(), arr_in.end(), printdata<string> ); cout<<endl;
        for_each( arr_out.begin(), arr_out.end(), printdata<string> ); cout<<endl;
        args.resize(arr_in.size());
        res.resize(arr_out.size());   
        transform( arr_in.begin(), arr_in.end(), args.begin(), getaddr );
        transform( arr_out.begin(), arr_out.end(), res.begin(), getaddr );
        cout<<"args"; for( paramvector::iterator it=args.begin(); it!=args.end(); ++it ) cout<<' '<<hex<<*it; cout<<endl;
        cout<<"res"; for( paramvector::iterator it=res.begin(); it!=res.end(); ++it ) cout<<' '<<*it; cout<<dec<<endl;        
        
        switch(algtype(nType)) {
            case _valveEval:
//                res[0]->setproperty( "count", args[0]->getvalue() );
                setproperty( "motion", _no_motion );
               break;
            case _valveProcessing: {
                res[0]->setproperty( "count", 0 );            
                res[0]->setproperty( "motion", _no_motion );
                res[0]->setproperty( "motion_old", _no_motion ); 
//                res[0]->setproperty( "configured", -1 );   
                res[0]->setproperty( "sp", args[8]->getvalue() );
                res[0]->setproperty( "err1", 0 );
                res[0]->setproperty( "err2", 0 );
                res[0]->setproperty( "kp", 1 );
                res[0]->setproperty( "kd", 0 );
                res[0]->setproperty( "ki", 0 );
                res[0]->setproperty( "pollT", 10000.0 );
                res[0]->setproperty( "out", res[0]->getvalue() );
                setproperty( "pulsewidth", 2000 );
            }
            break;
        }
        if( !args.size() || find_if( args.begin(), args.end(), testaddr ) != args.end() || \
                !res.size() || find_if( res.begin(), res.end(), testaddr ) != res.end() ) { rc = EXIT_FAILURE; fInit = -1; }
        else fInit = 1;
    }
//    cout<<" args="<<args.size()<<" res="<<res.size()<<endl;    
    return rc;
}

int16_t calgo::solveIt() {
    int16_t         rc = EXIT_SUCCESS;
    vector<int8_t>  arr_qual;
    uint8_t         nqual;
    double          raw;


//    if( !fInit ) rc = init();
    
    if( fInit>0 ) {
        arr_qual.resize(args.size());
        transform( args.begin(), args.end(), arr_qual.begin(), getqual );                          // get quality 
        uint8_t nqual = accumulate( arr_qual.begin(), arr_qual.end(), 0, bit_or<uint8_t>() );      // summary quality evaluate
//        cout<<" nType="<<nType<<" quality="<<hex<<(int16_t(nqual)&0x00ff)<<dec; 
        
        switch(algtype(nType)) {
            // Расчет положения клапана
            case _valveEval:
                if( args.size() >= 6 && res.size() >= 1 ) {
                    int16_t fEn;
                    int16_t iscfgd;
                    
                    res[0]->getproperty( "configured", iscfgd );
                    getproperty( "enable", fEn );
                    
                    if( fEn && iscfgd>0 ) {
                        int16_t cnt      = args[0]->getvalue();                     // текущий счетчик
                        int16_t cnt_old  = args[0]->getoldvalue();                  // предыдущий счетчик
                        int16_t lso      = args[1]->getvalue();                     // конечный открытия
                        int16_t lsc      = args[2]->getvalue();                     // конечный закрытия
                        int16_t cmd      = args[3]->gettask(true);                  // команда движения
                        int16_t dir      = args[4]->gettask(true);                  // команда обратного движения
                        int16_t mode     = args[5]->getvalue(); 
                        double  raw      = res[0]->getrawval();
                        
                        int16_t mot, iscfgd;
                        
                        getproperty( "motion", mot );                               // вспомним, была ли движуха
//                        res[0]->getproperty( "count", cnt_old ); 
                        
//                        cout<<" valveEval="<<" cnt= "<<cnt_old<<"|"<<cnt<<" raw="<<raw<<" cmd = "<<cmd<<"|"<<dir \
                            <<" lso="<<lso<<" lsc="<<lsc<<" fcQ="<<hex<<int16_t(args[0]->getquality())<<dec;                   
                        if( cmd ) mot = 1;                                          // команда движения
                        if( cmd && dir ) mot = -1;                                  // команда обратного движения
                        if(cnt) raw = raw + mot*(cnt - cnt_old);                    // считаем положение клапана в импульсах
                        if(lso && mot>0) raw = res[0]->getmaxraw();
                        if(lsc && mot<0) raw = 0; 
//                        cout<<" newRaw= "<<raw;
                        if( !cmd && (lsc || lso || cnt>20000) && cnt && args[0]->getquality()==OPC_QUALITY_GOOD ) {      
                            args[0]->settask(0);                                    // сброс счетчика
                        } 
//                        cout<<" newRaw= "<<raw<<endl;
                        setproperty( "motion", mot );                               // сохраним данные о движении до следующего опроса  
//                        res[0]->setproperty( "count", cnt ); 
                        args[0]->setoldvalue( cnt );   
                        res[0]->setrawval( raw );                                   // сохраним значение положения в сырых единицах
/*
                        if( mode <= _auto_press ) {
                            res[0]->setproperty( "sp", res[0]->getvalue() );
                        }
*/
                    }
                }
                break;
            // Управление клапаном
            case _valveProcessing:
                if( args.size() >= 10 && res.size() >= 1 ) {

                    pthread_mutex_lock( &mutex_param );
                    
                    cparam* pfc         = args[0];
                    cparam* pcmd        = args[3];
                    cparam* pdir        = args[4];  
                    cparam* pvl         = res[0];
                    int16_t cnt         = pfc->getvalue();
                    int16_t cnt_old     = pfc->getoldvalue();
                    int16_t lso         = args[1]->getvalue();
                    int16_t lso_old     = args[1]->getoldvalue();
                    int16_t lsc         = args[2]->getvalue();
                    int16_t lsc_old     = args[2]->getoldvalue();
                    int16_t cmd         = pcmd->getvalue();
                    int16_t cmd_old     = pcmd->getoldvalue(); 
                    int16_t dir         = pdir->getvalue();
                    double  cur         = args[5]->getvalue();
                    int16_t sw          = args[6]->getvalue();                   
                    int16_t mode        = args[7]->getvalue();
                    int16_t mode_old    = args[7]->getoldvalue();
                    double  pt          = args[8]->getvalue();
                    double  pttask      = args[8]->gettask();   
                    int16_t mot, mot_old, iscfgd, nv;
                    double  pv          = pvl->getvalue();
                    double  delta, max_delta, outv;
                    int16_t rc0         = (args[0]->getquality()!=OPC_QUALITY_GOOD);
                    int16_t rc2         = (args[3]->getquality()!=OPC_QUALITY_GOOD); 
                    int16_t pulsewidth=0;
                    int32_t pollT;

                    pvl->getproperty( "configured", iscfgd );
                    pvl->getproperty( "valve", nv );
                    
                    pthread_mutex_unlock( &mutex_param );

                    if( !mode || iscfgd<=0 )  {
//                        if( pvl->getname()=="FV11" ) cout<<endl;
                        break; 
                    } 
   
                    if( pvl->getname()=="FV11" ) \
                        cout<<"vlv proc "<<pvl->getname()<<" selV="<<sw<<" curV="<<nv<<" mode="<<mode<<" isCfg="<<iscfgd;
                   
                    if( mode==_auto_press ) {                                                   // PID 
                        pvl->getproperty( "pollT", pollT );
                        if( (m_twait.isDone() || !m_twait.isTiming()) ) {
                            double sp, err, err1, err2, kp, kd, ki, hi, lo;
                            pvl->getproperty( "sp", sp );
                            pvl->getproperty( "err1", err1 );
                            pvl->getproperty( "err2", err2 );
                            pvl->getproperty( "kp", kp );
                            pvl->getproperty( "kd", kd );
                            pvl->getproperty( "ki", ki );
                            pvl->getproperty( "hi", hi );
                            pvl->getproperty( "lo", lo );
                            pvl->getproperty( "out", outv );
                            err = pt - sp;
                            outv = outv + kp * ( err-err1 + ki*err + kd*(err-err1*2+err2) );    // вычислим задание для клапана
                            outv = min( hi, outv );                                             // загоним в границы
                            outv = max( lo, outv );
                            pvl->setproperty( "out", outv );                                    // сохраним значение задания
                            pvl->setproperty( "err1", err );
                            pvl->setproperty( "err2", err1 );
                            m_twait.start( pollT );
                        }
                    }
                    else {                                                                      // если ручной режим
                        pvl->setproperty( "sp", pt );
                        pvl->setproperty( "out", pvl->gettask() );
                    }

                    if( sw && sw!=nv ) break;
                    
                    pvl->getproperty( "max_task_delta", max_delta );   
                    pvl->getproperty( "task_delta", delta );   
                    pvl->getproperty( "motion", mot );
                    pvl->getproperty( "motion_old", mot_old );
                    getproperty( "pulsewidth", pulsewidth );

                    if( mot==_no_motion ) {                                                     // if valve standby
                        if( mot_old==_no_motion && nqual==OPC_QUALITY_GOOD ) {                  // && old state same
                           if( mode==_auto_press ) pvl->settask(outv);                   
                           if( (pvl->taskset() || mode>=_manual_pulse_open) && !cmd ) {         // if task set
                                int16_t task = round(pvl->gettask());
                               
                                switch(mode) {                                            
                                    case _manual_pulse_open:                                  
                                        mot = _open;                            
                                        cout<<" imp mot open="<<mot<<endl;
                                        break;                                           
                                    case _manual_pulse_close:                                  
                                        mot = _close;                            
                                        cout<<" imp mot close="<<mot<<endl;
                                        break;                                                             
                                    default:        
                                        if( pvl->gettask() - pv  > delta ) {                    // cmd to open
                                            if(!lsc || !cnt) mot = _open;                       // if closed wait for reset counter
                                        }                                                
                                        else                                             
                                        if( pv - pvl->gettask() > delta ) {                     // cmd to close
                                            if(!lso || !cnt) mot = _close;                      // if opened wait for reset counter
                                        }
                                        else pvl->cleartask();
                                }
                            }
                            if( mot ) {
                                args[6]->setvalue( nv );                                        // захватим управление  
                                if( mode>=_manual_pulse_open ) {
                                    m_tcmd.start( ( (mot==_open)?pulsewidth:pulsewidth+1000 ) );
                                }
                                else
                                    m_tcmd.start( 300000 );                                     // start timer to check moving process
                                if( mot==_close ) pdir->settask( 1 );
                            }
                        }
                        else if( !cmd || mot_old<_opening || m_tcmd.isDone()) {                 // valve state is idle && old state is moving 
                            if( mode<_manual_pulse_open && fabs(pv-pvl->gettask()) > delta) {   // if задание не достигнуто
                                pvl->settask(pvl->gettask());                                   // запустим снова
                            }
                            if( mode<_manual_pulse_open && !m_tcmd.isDone() && mot_old>=_opening ) {
                                if( mot_old==_opening ) delta -= (pvl->gettask() - pv);
                                if( mot_old==_closing ) delta += (pvl->gettask() - pv);
                                delta = min( fabs(delta), max_delta );
                            }
                            if( mode>=_manual_pulse_open ) {
                                args[7]->setvalue( mode_old );
                                mode = mode_old; 
                                pvl->settask( pv, false );                                      // set task value without process start    
                            }
                            m_tcmd.reset();
                            mot_old = mot;
                            args[6]->setvalue( 0 );                                             // освободим управление 
                        }          
                    }
                    else if( mot ) {                                                            // if valve moving (motion != 0)
                        res[0]->cleartask();
                        
                        bool stopOp = ( mot%10==_open  && ( (lso && !lso_old) || (mode<_manual_pulse_open)&&(pv > (pvl->gettask()-delta)) ) ); 
                        bool stopCl = ( mot%10==_close  && ( (lsc && !lsc_old) || (mode<_manual_pulse_open)&&(pv < (pvl->gettask()+delta)) ) );

                        if( !rc2 && (m_tcmd.isDone() || (stopOp || stopCl) || rc0 ) ) {         // limit switch (open or close ) position reached
                            mot_old = mot;                                                      // or task completed                     
                            if(mot>=_opening) pcmd->settask( 0 );
                            if(mot%10==_close) pdir->settask( 0 );
                            m_tcmd.reset();
                            mot = 0;
                            if(rc0) mot_old = -1;
                        }
                        if( !mot_old && (mot==_open || m_tcmd.getTT()>=1000) ) {                // если есть задание        
                            pcmd->settask( 1 );                                                 // даем команду клапану
                            mot_old = mot;
                            mot += 10;
                        }
                    }
                    if( pvl->getname()=="FV11" ) \
                        cout<<" mot= "<<mot_old<<"|"<<mot<<" cmd= "<<cmd<<"|"<<dir<<" cnt= "<<cnt_old<<"|"<<cnt \
                            <<" pv="<<pv<<" task= "<<pvl->taskset()<<"|"<<pvl->gettask()<<"|"<<delta \
                            <<" lso= "<<lso_old<<"|"<<lso<<" lsc= "<<lsc_old<<"|"<<lsc<<" I="<<cur<<" fcQ="<<rc0<<" cvQ="<<rc2<<" pulseW="<<pulsewidth \
                            <<" t="<<m_tcmd.getTT()<<"|"<<m_tcmd.getPreset()<<"|"<<m_tcmd.isDone()<<endl;                        
                    
                    pvl->setproperty( "task_delta", delta );                    
                    pvl->setproperty( "motion", mot );
                    pvl->setproperty( "motion_old", mot_old );
//                    pvl->setproperty( "configured", iscfgd );
//                    pvl->setproperty( "count", cnt );                  
                    args[1]->setoldvalue( lso );   
                    args[2]->setoldvalue( lsc );

                    if( mode<_manual_pulse_open ) {
                        args[7]->setoldvalue( mode );
                    }
/*
                    if( 1 && m_name.substr(0,3)=="FV1" ) { 
                        cout<<m_raw;
                        cout<<" motion="<<m_motion<<endl;
                    }
*/
                }
                break;
            case _valveCalibrate:
                if( args.size() >= 8 && res.size() >= 1 ) {
                    cparam* pfc         = args[0];
                    cparam* pcmd        = args[3];
                    cparam* pdir        = args[4];
                    cparam* pvl         = res[0];
                    int16_t cnt         = pfc->getvalue();
                    int16_t cnt_old;
                    int16_t lso         = args[1]->getvalue();
                    int16_t lso_old     = args[1]->getoldvalue();
                    int16_t lsc         = args[2]->getvalue();
                    int16_t lsc_old     = args[2]->getoldvalue();
                    int16_t cmd         = pcmd->getvalue();
                    int16_t cmd_old     = pcmd->getoldvalue(); 
                    int16_t dir         = pdir->getvalue();
                    double  cur         = args[5]->getvalue();
                    int16_t sw          = args[6]->getvalue();                   
                    int16_t mode        = args[7]->getvalue();                   
                    int16_t mot, mot_old, iscfgd, nv;
                    double  pv          = pvl->getvalue();
                    double  task        = pvl->gettask();
                    double  delta, max_delta;
                    int16_t rc0         = (args[0]->getquality()!=OPC_QUALITY_GOOD);
                    int16_t rc2         = (args[3]->getquality()!=OPC_QUALITY_GOOD); 

                    pvl->getproperty( "configured", iscfgd );
                    pvl->getproperty( "valve", nv );
                    
                    if( (sw && sw!=nv) || !mode || iscfgd>=0 ) {
//                        if( pvl->getname()=="FV11" ) cout<<endl;
                        break; 
                    }
                    
                    if( pvl->getname()=="FV11" ) \
                        cout<<"vlv calbr "<<pvl->getname()<<" selV="<<sw<<" curV="<<nv<<" mode="<<mode<<" isCfg="<<iscfgd;
                    
                    pvl->getproperty( "max_task_delta", max_delta );   
                    pvl->getproperty( "task_delta", delta );   
                    pvl->getproperty( "motion", mot );
                    pvl->getproperty( "motion_old", mot_old );
                    pvl->getproperty( "count", cnt_old );                   
                    cout<<" mot= "<<mot_old<<"|"<<mot<<" cnt= "<<cnt_old<<" task="<<pvl->gettask() \
                        <<" lso= "<<lso_old<<"|"<<lso<<" lsc= "<<lsc_old<<"|"<<lsc<<" I="<<cur<<" fcQ="<<rc0<<" cvQ="<<rc2<<endl;
                   
                    if( mot==_no_motion ) {                                                     // if valve standby
                        if( mot_old==_no_motion && nqual==OPC_QUALITY_GOOD ) {                  // && old state same
                            if( pvl->taskset() && ( iscfgd==-1 || iscfgd==-2 ) && !cmd ) {      // if valve scale not calibrated
                                if( lsc || lso ) {                                              // if limit one switch is ON
                                    if( !cnt ) { 
                                        iscfgd=-3;
                                        mot = lsc ? _open : _close;
                                    }
                                    else args[0]->settask(0);                                   // сброс счетчика
                                }
                                else if( iscfgd==-1 ) {                                         // if valve in middle position
                                    iscfgd = -2;
                                    mot = _close;
                                }
                                else pvl->cleartask();
                            } 
                            if( mot ) {
                                m_tcmd.start( 300000 );                                         // start timer to check moving process
                                if( mot==_close ) pdir->settask( 1 );
                                args[6]->setvalue( nv );                                        // захватим управление
                            }
                        }
                        else if( !cmd || mot_old<_opening ) {                                   // valve state is idle && old state is moving 
                            if( iscfgd==-2 && lsc ) {                                           // если калибровка + нижний конечник
                                pvl->settask(task);                                             // запустим снова
                            }
                            if( iscfgd==-3 && (lso || lsc) && cnt>1000 ) { 
                                pvl->setrawscale( 0, cnt );
                                iscfgd=1;   
                                delta = cnt/1000;
                                delta = min( fabs(delta), max_delta );
                                pvl->setrawval( lsc ? 0: cnt );
                            }
                            m_tcmd.reset();
                            mot_old = mot;
                            args[6]->setvalue( 0 );   
                        }          
                    }
                    else if( mot ) {                                                            // if valve moving (motion != 0)
                        pvl->cleartask();
                        
                        bool stopOp = ( mot%10==_open  && ( lso && !lso_old ) ); 
                        bool stopCl = ( mot%10==_close && ( lsc && !lsc_old ) );

                        if( !rc2 && (m_tcmd.isDone() || (stopOp || stopCl) || rc0 ) ) {         // limit switch (open or close ) position reached
                            mot_old = mot;                                                      // or task completed                     
                            if(mot>=_opening) pcmd->settask( 0 );
                            if(mot%10==_close) pdir->settask( 0 );
                            m_tcmd.reset();
                            mot = 0;
                            if(rc0) mot_old = -1;
                        }
                        if( !mot_old && (mot==_open || m_tcmd.getTT()>1000) ) {                 // если есть задание        
                            pcmd->settask( 1 );                                                 // даем команду клапану
                            mot_old = mot;
                            mot += 10;
                        }
                    }
                    
                    pvl->setproperty( "task_delta", delta );                    
                    pvl->setproperty( "motion", mot );
                    pvl->setproperty( "motion_old", mot_old );
                    pvl->setproperty( "configured", iscfgd );
                    pvl->setproperty( "count", cnt );                  
                    args[1]->setoldvalue( lso );   
                    args[2]->setoldvalue( lsc );   
                   
/*
                    if( 1 && m_name.substr(0,3)=="FV1" ) { 
                        cout<<m_raw;
                        cout<<" motion="<<m_motion<<endl;
                    }
*/
                }
                break;
            // Расчет расхода по перепаду давления на сечении клапана   
            case _floweval:  
//                cout<<" eval flow for delta pressure size="<<args.size()<<" | "<<res.size()<<endl;
                if( args.size() >= 5 && res.size() >= 1 && nqual==OPC_QUALITY_GOOD ) {
                    cparam* pfv  = args[0];  // valve mm opened
                    cparam* ppt1 = args[1];  // давление в пласте
                    cparam* ppt2 = args[2];  // давление у насоса
                    cparam* pdt  = args[3];  // density 
                    cparam* pkv  = args[4];  // Kv factor

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
                    res[0]->setrawval(raw);   
                }
                else if(res.size()) res[0]->setquality(OPC_QUALITY_BAD);
                break;
            // Суммирование входных аргументов и запись в выходной
            case _summ:
//                cout<<" eval flow summary size="<<args.size()<<" | "<<res.size()<<endl;   
                if( args.size() >= 2 && res.size() >= 1 && nqual==OPC_QUALITY_GOOD ) { 
                    vector<double> arr_real;  
                    arr_real.resize( args.size() );
                    transform( args.begin(), args.end(), arr_real.begin(), getval );
                    raw = accumulate( arr_real.begin(), arr_real.end(), 0.0, plus<double>() );
                    res[0]->setrawval(raw);
                }
                else if(res.size()) res[0]->setquality(OPC_QUALITY_BAD);   
                break;
            // Вычисление перепада давления: sub входных аргументов, mul 1000 и запись в выходной
            case _sub:
//                cout<<" eval deltaP size="<<args.size()<<" | "<<res.size()<<endl;   
                if( args.size() >= 2 && res.size() >= 1 && nqual==OPC_QUALITY_GOOD ) { 
                    vector<double> arr_real;  
                    arr_real.resize( args.size() );
                    transform( args.begin(), args.end(), arr_real.begin(), getval );
//                    raw = accumulate( arr_real.begin(), arr_real.end(), 0.0, dPress );
                    res[0]->setrawval( dPress( arr_real[0], arr_real[1] ) );
                }
                else if( res.size() ) res[0]->setquality(OPC_QUALITY_BAD);   
                break;
            default: rc = EXIT_FAILURE;
        }
    }
    else rc = init();
    
    //    cout<<endl;
    return rc;
}

