#include "algo.h"

alglist algos;
vlvmode vmodes;

double getval(ctag* p) {
    double rval=-1;
    if( p ) rval = p->getvalue();

    return rval;
}

int8_t getqual(ctag* p) {
    int8_t nval=0;
    if ( p ) nval = p->getquality();

    return nval;
}

ctag* getaddr(string& str) {
    ctag* p = tagdir.gettag( str.c_str() );
    cout<<"gettag "<<hex<<long(p);
    if(p) cout<<" name="<< p->getname();
    cout<<endl;

    return p;
}

bool testaddr(ctag* x) {
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
        transform( arr_in.begin(), arr_in.end(), args.begin(), getaddr );               // получим ссылки на тэги
        transform( arr_out.begin(), arr_out.end(), res.begin(), getaddr );              // получим ссылки на тэги
        cout<<"args"; for( tagvector::iterator it=args.begin(); it!=args.end(); ++it ) cout<<' '<<hex<<*it; cout<<endl;
        cout<<"res"; for( tagvector::iterator it=res.begin(); it!=res.end(); ++it ) cout<<' '<<*it; cout<<dec<<endl;        
        
        switch(algtype(nType)) {
            case _valveEval:
//                res[0]->setproperty( "count", args[0]->getvalue() );
//               res[0]->setproperty( "configured", 1 );   
               setproperty( "motion", _no_motion );
               break;
            case _valveProcessing: {
                double d;
                int16_t rc=-100;
                res[0]->setproperty( "count", 0 );            
                res[0]->setproperty( "motion", _no_motion );
                res[0]->setproperty( "motion_old", _no_motion ); 
//                res[0]->setproperty( "configured", 1 );   
                res[0]->setproperty( "sp", args[8]->getvalue() );
                res[0]->setproperty( "err1", 0 );
                res[0]->setproperty( "err2", 0 );
                if( (rc=res[0]->getproperty( "kp", d ))!=EXIT_SUCCESS ) res[0]->setproperty( "kp", 1 );
                if( res[0]->getproperty( "kd", d )!=EXIT_SUCCESS ) res[0]->setproperty( "kd", 0 );
                if( res[0]->getproperty( "ki", d )!=EXIT_SUCCESS ) res[0]->setproperty( "ki", 0.01 );
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
                    
                    if( fEn /*&& iscfgd>0*/ ) {
                        int16_t cnt     = args[0]->getvalue();                     // текущий счетчик
                        int16_t cnt_old = args[0]->getoldvalue();                  // предыдущий счетчик
                        int16_t lso     = args[1]->getvalue();                     // конечный открытия
                        int16_t lsc     = args[2]->getvalue();                     // конечный закрытия
                        int16_t lso_old = args[1]->getoldvalue();                  // конечный открытия
                        int16_t lsc_old = args[2]->getoldvalue();                  // конечный закрытия
                        int16_t cmd     = args[3]->gettask(true);                  // команда движения
                        int16_t dir     = args[4]->gettask(true);                  // команда обратного движения
                        ctag* pmode     = args[5]; 
                        double  raw     = res[0]->getrawval();
                        double  delta, max_delta;
                        
                        res[0]->getproperty( "max_task_delta", max_delta );   
                        res[0]->getproperty( "task_delta", delta );   
                       
                        int16_t mot;
                        
                        getproperty( "motion", mot );                               // вспомним, была ли движуха
//                        res[0]->getproperty( "count", cnt_old ); 
                        
//                        cout<<" valveEval="<<res[0]->getname()<<" cnt= "<<cnt_old<<"|"<<cnt<<" raw="<<raw<<" cmd = "<<cmd<<"|"<<dir \
                            <<" lso="<<lso_old<<"|"<<lso<<" lsc="<<lsc_old<<"|"<<lsc\
                            <<" fcQ="<<hex<<int16_t(args[0]->getquality())<<dec<<" isCfgd="<<iscfgd<<endl;                   
                        if( cmd ) mot = 1;                                          // команда движения
                        if( cmd && dir ) mot = -1;                                  // команда обратного движения
                        if(cnt) raw = raw + mot*(cnt - cnt_old);                    // считаем положение клапана в импульсах
                        if(lso && mot>0) raw = res[0]->getmaxraw();
                        if(lsc && mot<0) raw = 0; 
//                        cout<<" newRaw= "<<raw;
                        lso = (lso /*&& !lso_old*/) || (lsc /*&& !lsc_old*/) || cnt>20000;
                        if( !cmd && lso && cnt && args[0]->getquality()==OPC_QUALITY_GOOD && !res[0]->taskset() ) { 
                            if( iscfgd==-3 ) {
                                iscfgd=1;
                                res[0]->setrawscale( 0, cnt );
                                iscfgd=1;   
                                delta = cnt/1000;
                                delta = min( fabs(delta), max_delta );
                                res[0]->setrawval( lsc ? 0: cnt );
                                pmode->setvalue( pmode->getoldvalue() );
                            }
                            args[0]->settask(0);                                    // сброс счетчика
                        } 
//                        cout<<" newRaw= "<<raw<<endl;
                        setproperty( "motion", mot );                               // сохраним данные о движении до следующего опроса  
//                        res[0]->setproperty( "count", cnt ); 
                        if( iscfgd>0 ) {  
                            args[0]->setoldvalue( cnt );   
                            res[0]->setrawval( raw );                                   // сохраним значение положения в сырых единицах
                        }
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
                if( args.size() >= 9 && res.size() >= 1 ) {

                    pthread_mutex_lock( &mutex_tag );
                    
                    ctag* pfc         = args[0];
                    ctag* pcmd        = args[3];
                    ctag* pdir        = args[4];  
                    ctag* pvl         = res[0];
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
                    const double task   = pvl->gettask();
                    const double maxOpen= pvl->getmaxeng();
                    const double minOpen= pvl->getmineng();

                    pvl->getproperty( "configured", iscfgd );
                    pvl->getproperty( "valve", nv );
                    pvl->getproperty( "out", outv );
                   
                    pthread_mutex_unlock( &mutex_tag );

                    if( !mode /*|| iscfgd<=0*/ )  {
/*                        if(mode_old) {
                            pvl->setproperty( "motion", _no_motion );
                            pvl->setproperty( "motion_old", _no_motion );                       
                            pcmd->settask( 0 );
                            pdir->settask( 0 );
                            args[7]->setoldvalue( 0 );
                        }
*/
                        break; 
                    } 
   
//                    if( pvl->getname()=="FV11" ) 
                        cout<<"vlv proc "<<pvl->getname()<<" selV="<<sw<<" curV="<<nv<<" mode="<<mode_old<<"|"<<mode<<" isCfg="<<iscfgd;
                    switch(mode) {
                        case _auto_pid:                                                             // PID
                            pvl->getproperty( "pollT", pollT );
                            if( (m_twait.isDone() || !m_twait.isTiming()) ) {
                                double sp, err, err1, err2, kp, kd, ki, hi, lo, piddead;
                                int16_t pidtype;
                                pvl->getproperty( "err1", err1 );
                                pvl->getproperty( "err2", err2 );
                                pvl->getproperty( "kp", kp );
                                pvl->getproperty( "kd", kd );
                                pvl->getproperty( "ki", ki );
                                pvl->getproperty( "hi", hi );
                                pvl->getproperty( "lo", lo );
                                getproperty( "pidtype", pidtype );
                                getproperty( "piddead", piddead );   
                                err = (pidtype==2) ? pttask - pt : pt - pttask;
                                err = (fabs(err)>piddead) ? err : 0;
                                outv = outv + kp * ( err-err1 + ki*err + kd*(err-err1*2+err2) );    // вычислим задание для клапана
                                outv = min( hi, outv );                                             // загоним в границы
                                outv = max( lo, outv );
                                pvl->setproperty( "out", outv );                                    // сохраним значение задания
                                pvl->setproperty( "err1", err );
                                pvl->setproperty( "err2", err1 );
                                m_twait.start( pollT );
                                cout<<"pid "<<pvl->getname()<<" pt="<<pt<<" pttask="<<pttask<<" err="<<err2<<" | "<<err1<<" | "<<err\
                                    <<" kp="<<kp<<" ki="<<ki<<" kd="<<kd<<" out="<<outv<<endl;
                            }
                            break;
                        case _auto_time:                                                            // автомат по времени
                            outv = task;
                            break;
                        default:                                                                    // если ручной режим
                            pvl->setproperty( "out", pvl->gettask() );
                            if( args[8]->gettask()!=pt ) args[8]->settask( pt, false );             // значение сохраним в задание
                    }

                    if( sw && sw!=nv ) {
                        cout<<endl;
                        break;
                    }
                    
                    pvl->getproperty( "max_task_delta", max_delta );   
                    pvl->getproperty( "task_delta", delta );   
                    pvl->getproperty( "motion", mot );
                    pvl->getproperty( "motion_old", mot_old );
                    getproperty( "pulsewidth", pulsewidth );
                    const int _lag_start=2000;
                        
                    if( mot==_no_motion ) {                                                     // if valve standby
                        if( mot_old==_no_motion && nqual==OPC_QUALITY_GOOD ) {                  // && old state same
                            if( (mode==_auto_pid /*|| mode==_auto_time*/) && outv!=task ) pvl->settask(outv);                   
                            if( (pvl->taskset() || mode>=_manual_pulse_open) && !cmd ) {        // if task set
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
                                        if( task - pv > delta || task==maxOpen ) {              // cmd to open
                                            if(!lsc || !cnt) mot = _open;                       // if closed wait for reset counter
                                        }                                                
                                        else                                             
                                        if( pv - task > delta || task==minOpen ) {              // cmd to close
                                            if(!lso || !cnt) mot = _close;                      // if opened wait for reset counter
                                        }
                                        else pvl->cleartask();
                                }
                            }
                            if( mot ) {
                                args[6]->setvalue( nv );                                        // захватим управление  
                                if( mode>=_manual_pulse_open ) {
                                    m_tcmd.start( ( (mot==_open)?pulsewidth:pulsewidth+_lag_start ) );
                                }
                                else
                                    m_tcmd.start( 500000 );                                     // start timer to check moving process
                                if( mot==_close ) pdir->settask( 1 );
                            }
                        }
                        else if( !cmd || mot_old<_opening || m_tcmd.isDone()) {                 // valve state is idle && old state is moving
                            if( mode<_manual_pulse_open && fabs(pv-pvl->gettask()) > delta) {   // if задание не достигнуто
//                                pvl->settask(pvl->gettask());                                   // запустим снова
                            }
                            if( mode<_manual_pulse_open && !m_tcmd.isDone() && mot_old>=_opening && !lsc && !lso) {
                                if( mot_old==_opening ) delta -= (pvl->gettask() - pv);
                                if( mot_old==_closing ) delta += (pvl->gettask() - pv);
                                delta = min( fabs(delta), max_delta );
                            }
                            if( mode>=_manual_pulse_open ) {
                                args[7]->setvalue( mode_old );
                                mode = mode_old; 
                                pvl->settask( pv, false );                                      // set task value without process start    
                            }
                            if(lsc) pvl->settask( pvl->getmineng(), false );
                            if(lso) pvl->settask( pvl->getmaxeng(), false );
                            m_tcmd.reset();
                            mot_old = mot;
                            args[6]->setvalue( 0 );                                             // освободим управление 
                        }          
                    }
                    else if( mot ) {                                                            // if valve moving (motion != 0)
                        if( mode==_auto_pid && outv!=task ) pvl->settask(outv, false);          // выдадим на клапан новое расчетное задание
                        const bool lsup = (lso && !lso_old);
                        const bool lsdown = (lsc && !lsc_old);
                        const bool openReached = (mode<_manual_pulse_open) && (task!=maxOpen) && (pv > (task-delta));
                        const bool closeReached = (mode<_manual_pulse_open)&& (task!=minOpen) && (pv < (task+delta));
                      
                        const bool stopOp = ( mot%10==_open   && ( lsup   || openReached  ) ); 
                        const bool stopCl = ( mot%10==_close  && ( lsdown || closeReached ) );
                        const bool fstop  = ( m_tcmd.isDone() || stopOp || stopCl );

                        if( !rc2 && (fstop || rc0) ) {                                          // limit switch (open or close) position reached
                            mot_old = mot;                                                      // or task completed                     
                            pvl->cleartask();
                            if(mot>=_opening) pcmd->settask( 0 );
                            if(mot%10==_close) pdir->settask( 0 );
                            m_tcmd.reset();
                            mot = 0;
                            if(rc0) mot_old = -1;
                        }
                        if( !mot_old && (mot==_open || m_tcmd.getTT()>=_lag_start) ) {          // если есть задание        
                            pcmd->settask( 1 );                                                 // даем команду клапану
                            mot_old = mot;
                            mot += 10;
                        }
                    }
//                    if( pvl->getname()=="FV11" ) 
                        cout<<" mot= "<<mot_old<<"|"<<mot<<" cmd= "<<cmd<<"|"<<dir<<" cnt= "<<cnt_old<<"|"<<cnt \
                            <<" pv="<<pv<<" task= "<<pvl->taskset()<<"|"<<pvl->gettask()<<"|"<<delta \
                            <<" lso= "<<lso_old<<"|"<<lso<<" lsc= "<<lsc_old<<"|"<<lsc<<" I="<<cur\
                            <<" fcQ="<<rc0<<" cvQ="<<rc2<<" pulseW="<<pulsewidth<<" scaleVLV="<<minOpen<<"|"<<maxOpen\
                            <<" t="<<m_tcmd.getTT()<<"|"<<m_tcmd.getPreset()<<"|"<<m_tcmd.isDone()<<endl;                        
                    
                    pvl->setproperty( "task_delta", delta );                    
                    pvl->setproperty( "motion", mot );
                    pvl->setproperty( "motion_old", mot_old );
                    args[1]->setoldvalue( lso );   
                    args[2]->setoldvalue( lsc );

                    if( mode != mode_old && mode<_manual_pulse_open && mode!=_auto_time && mode_old!=_auto_time \
                            && mot==_no_motion && mot_old==_no_motion ) {
                        args[7]->setoldvalue( mode );
                        cout<<"valve proc change mode "<<mode_old<<"|"<<mode<<endl; 
                    }
                }
                break;
            case _valveCalibrate:                                                       // режим конфигурирования
                if( args.size() >= 8 && res.size() >= 1 ) {
                    ctag* pfc         = args[0];
                    ctag* pcmd        = args[3];
                    ctag* pdir        = args[4];
                    ctag* pvl         = res[0];
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
                    int16_t mode_old    = args[7]->getoldvalue();                   
                    int16_t nv;
                    static int16_t iscfgd;
                   
                    double  pv          = pvl->getvalue();
                    double  rtaskmot;   
                    int16_t rc0         = (args[0]->getquality()!=OPC_QUALITY_GOOD);
                    int16_t rc2         = (args[3]->getquality()!=OPC_QUALITY_GOOD); 
                    const double task   = pvl->gettask();
                    const double maxOpen= pvl->getmaxeng();
                    const double minOpen= pvl->getmineng();


//                    pvl->getproperty( "configured", iscfgd );
                    pvl->getproperty( "valve", nv );
                    
                    if( (sw && sw!=nv) || mode!=_calibrate /* || iscfgd>=0*/ ) {        // если не режим конфигурирования, уходим
//                        if( pvl->getname()=="FV11" ) cout<<endl;
                        break; 
                    }
                    
                    if( mode_old!=_calibrate ) {
                        args[7]->setoldvalue(_calibrate);   
                        iscfgd = -1;
                    }
                   
                    if( pvl->getname()=="FV11" ) \
                        cout<<"vlv calbr "<<pvl->getname()<<" selV="<<sw<<" curV="<<nv<<" mode="<<mode<<" isCfg="<<iscfgd;
                    
                    cout<<" cnt= "<<cnt_old<<" task="<<pvl->gettask() \
                        <<" lso= "<<lso_old<<"|"<<lso<<" lsc= "<<lsc_old<<"|"<<lsc<<" I="<<cur<<" fcQ="<<rc0<<" cvQ="<<rc2<<endl;
                   
/*                    if( mot==_no_motion ) {                                                     // if valve standby
                        if( mot_old==_no_motion && nqual==OPC_QUALITY_GOOD ) {                  // && old state same
                            if( pvl->taskset() && ( iscfgd==-1 || iscfgd==-2 ) && !cmd ) {      // if valve scale not calibrated
*/
                            if( !sw && ( iscfgd==-1 || iscfgd==-2 ) ) {
                                if( lsc || lso ) {                                              // if one limitswitch is ON
                                    if( !cnt ) { 
                                        iscfgd=-3;
                                        rtaskmot = lsc ? maxOpen : minOpen;
                                    }
                                }
                                else if( iscfgd==-1 ) {                                         // if valve in middle position
                                    iscfgd = -2;
                                    rtaskmot = minOpen;
                                }
                                else rtaskmot=-1;
                                if(rtaskmot>=0 && !pvl->taskset()) pvl->settask(rtaskmot);
                            } 
//                    pvl->setproperty( "configured", iscfgd );
                }
                break;
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
            // Расчет расхода по перепаду давления на сечении клапана   
            case _floweval:  
//                cout<<" eval flow for delta pressure size="<<args.size()<<" | "<<res.size()<<endl;
                if( args.size() >= 5 && res.size() >= 1 && nqual==OPC_QUALITY_GOOD ) {
                    ctag* pfv  = args[0];  // valve mm opened
                    ctag* ppt1 = args[1];  // давление в пласте
                    ctag* ppt2 = args[2];  // давление у насоса
                    ctag* pdt  = args[3];  // density 
                    ctag* pkv  = args[4];  // Kv factor

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

