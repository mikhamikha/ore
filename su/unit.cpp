#include "unit.h"

unitlist units;
vlvmode vmodes;

// Получить значение
double getval(ctag* p) {
    double rval=-1;
    if( p ) rval = p->getvalue();

    return rval;
}

// Получить качество тэга
int8_t getqual(ctag* p) {
    int8_t nval=0;
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
// проверка на NULL
bool testaddr(ctag* x) {
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

int16_t cunit::init() {
    int16_t rc = EXIT_SUCCESS;

    if( !fInit ) {
        string snam;
        getproperty("type", m_ntype);   
        switch( m_ntype ) {
            case _valve: {
                getproperty("lso", snam);       m_plso      = getaddr(snam);
                getproperty("lsc", snam);       m_plsc      = getaddr(snam);
                getproperty("fc", snam);        m_pfc       = getaddr(snam);
                getproperty("pos", snam);       m_ppos      = getaddr(snam);
                getproperty("mode", snam);      m_pmod      = getaddr(snam);
                getproperty("selected", snam);  m_psel      = getaddr(snam);
                getproperty("cmd1", snam);      m_pcmdopen  = getaddr(snam);
                getproperty("cmd2", snam);      m_pcmdclose = getaddr(snam);
                getproperty("block1", snam);    m_pblock    = getaddr(snam);
                getproperty("reg", snam);       m_preg      = getaddr(snam);
                getproperty("name", m_name);
                
                int16_t lso     = m_plso->getvalue();                                   // конечный открытия
                int16_t lsc     = m_plsc->getvalue();                                   // конечный закрытия
                if( !lso && !lsc )                                                      // если нет конечников
                    m_ppos->setrawval( (m_ppos->getmaxraw()+m_ppos->getminraw())/2 );   // будем считать, что клапан посередине
                else if( lso && !lsc )                                                  // иначе - соответственно...
                    m_ppos->setrawval( m_ppos->getmaxraw() );   
                else if( !lso && lsc )
                    m_ppos->setrawval( m_ppos->getminraw() );   
                
                double d;
                int16_t rc=-100;
                /*
                m_ppos->setproperty( "count", 0 );            
                m_ppos->setproperty( "motion", _no_motion );
                m_ppos->setproperty( "motion_old", _no_motion ); 
                m_ppos->setproperty( "sp", m_preg->getvalue() );
                m_ppos->setproperty( "err1", 0 );
                m_ppos->setproperty( "err2", 0 );
                */
                setproperty( "count", 0 );            
                setproperty( "motion", _no_motion );
                setproperty( "motion_old", _no_motion ); 
                setproperty( "sp", m_preg->getvalue() );
                setproperty( "err1", 0 );
                setproperty( "err2", 0 );
                if( (rc=getproperty( "kp", d ))!=EXIT_SUCCESS ) setproperty( "kp", 1 );
                if( getproperty( "kd", d )!=EXIT_SUCCESS ) setproperty( "kd", 0 );
                if( getproperty( "ki", d )!=EXIT_SUCCESS ) setproperty( "ki", 0.01 );
                setproperty( "pollT", 10000.0 );
                setproperty( "out", m_ppos->getvalue() );
//                setproperty( "pulsewidth", 2000 );
                setproperty( "configured", 1 );
                setproperty( "task_delta", double(1) );            
                setproperty( "max_task_delta", double(10) );           
                if( m_name.size()>1 && isdigit(m_name[1]) ) {
                    setproperty( "valve", m_name.substr(1,1).c_str() );               
                    m_num = atoi(m_name.substr(1,1).c_str());
                }
                m_status = 0;
            }
            break;
        }

        fInit = true;
    }

    return rc;
}

int16_t cunit::getstate() {
    int16_t rc = EXIT_SUCCESS;
    double  raw_set;        
    init();

/*
    int16_t cnt     = m_pfc->getvalue();                    // текущий счетчик
    int16_t cnt_old = m_pfc->getoldvalue();                 // предыдущий счетчик
    int16_t lso     = m_plso->getvalue();                   // конечный открытия
    int16_t lso_old = m_plso->getoldvalue();                // конечный открытия
    int16_t lsc     = m_plsc->getvalue();                   // конечный закрытия
    int16_t lsc_old = m_plsc->getoldvalue();                // конечный закрытия
    int16_t cmd_tsk = m_pcmdopen->gettask(true);            // команда движения
    int16_t dir_tsk = m_pcmdclose->gettask(true);           // команда обратного движения
    double  raw     = m_ppos->getrawval();                  // текущее "сырое" значение положения
*/
   
    double  delta, max_delta;
   
    switch( m_ntype ) {
        case _valve:
//
// -------- вычислим текущее положение
//
            m_ppos->getproperty( "max_task_delta", max_delta );   
            m_ppos->getproperty( "task_delta", delta );   
           
            int16_t mot;
            
            getproperty( "motion", mot );                               // вспомним, была ли движуха
//                        res[0]->getproperty( "count", cnt_old ); 
            
//                        cout<<" valveEval="<<res[0]->getname()<<" cnt= "<<cnt_old<<"|"<<cnt<<" raw="<<raw<<" cmd = "<<cmd<<"|"<<dir \
                <<" lso="<<lso_old<<"|"<<lso<<" lsc="<<lsc_old<<"|"<<lsc\
                <<" fcQ="<<hex<<int16_t(args[1]->getquality())<<dec<<" isCfgd="<<iscfgd<<endl;                   
            if( _cmd_tsk ) mot = 1;                                          // команда движения
            if( _cmd_tsk && _dir_tsk ) mot = -1;                              // команда обратного движения
            if(_cnt) raw_set = _raw + mot*(_cnt - _cnt_old);                    // считаем положение клапана в импульсах
            if(_lso && mot>0) raw_set = m_ppos->getmaxraw();
            if(_lsc && mot<0) raw_set = m_ppos->getminraw(); 
//                        cout<<" newRaw= "<<raw;
            bool fReset = (_lso && _lso_old) || (_lsc && _lsc_old) || _cnt>20000;
            if( !_cmd_tsk && fReset && _cnt && m_pfc->getquality()==OPC_QUALITY_GOOD && !m_ppos->taskset() ) { 
                if( iscfgd==-3 ) {                                      // если режим калибровки
                    iscfgd=1;
                    m_ppos->setrawscale( 0, _cnt );
                    iscfgd=1;   
                    delta = _cnt/1000;
                    delta = min( fabs(delta), max_delta );
                    m_ppos->setrawval( _lsc ? 0: _cnt );
                    m_pmod->setvalue( m_pmod->getoldvalue() );
                }
                m_pfc->settask(0);                                      // сброс счетчика
            } 
//                        cout<<" newRaw= "<<raw<<endl;
            setproperty( "motion", mot );                               // сохраним данные о движении до следующего опроса  
//                        res[0]->setproperty( "count", cnt ); 
//            if( iscfgd>0 ) {  
                m_pfc->setoldvalue( _cnt );   
                m_ppos->setrawval( raw_set );                           // сохраним значение положения в сырых единицах
//            }
    }
    return rc;
}

int16_t cunit::control( void ) {
    switch( m_ntype ) {
        case _valve:
            const int32_t   lag_start=2000;
            int32_t         pulsewidth;
            int16_t         max_delta, delta, mot, mot_old, nv;
            uint8_t         nqual = (getqual(m_pfc) | getqual(m_pcmdopen) | getqual(m_plso));

            getproperty( "max_task_delta", max_delta );   
            getproperty( "task_delta", delta );   
            getproperty( "motion", mot );
            getproperty( "motion_old", mot_old );
            getproperty( "pulsewidth", pulsewidth );

            if( mot==_no_motion ) {                                                         // if valve standby
                if( mot_old==_no_motion && nqual==OPC_QUALITY_GOOD ) {                      // && old state same
                    if( (_mode==_auto_pid ) && outv!=_task )  m_ppos->settask(outv);                   
                    if( (m_ppos->taskset() || _mode>=_manual_pulse_open) && !_cmd_tsk ) {   // if task set
                        switch(_mode) {                                            
                            case _manual_pulse_open:                                  
                                mot = _open;                            
                                cout<<" imp mot open="<<mot<<endl;
                                break;                                           
                            case _manual_pulse_close:                                  
                                mot = _close;                            
                                cout<<" imp mot close="<<mot<<endl;
                                break;                                                             
                            default:        
                                if( _task - _value > delta || _task==_maxOpen ) {       // cmd to open
                                    if(!_lsc || !_cnt) mot = _open;                     // if closed wait for reset counter
                                }                                                
                                else                                             
                                if( _value - _task > delta || _task==_minOpen ) {       // cmd to close
                                    if(!_lso || !_cnt) mot = _close;                    // if opened wait for reset counter
                                }
                                else m_ppos->cleartask();
                        }
                    }
                    if( mot ) {
                        m_psel->setvalue( m_num );                                      // захватим управление  
                        if( _mode>=_manual_pulse_open ) {
                            m_tcmd.start( ( (mot==_open)?pulsewidth:pulsewidth+_lag_start ) );
                        }
                        else
                            m_tcmd.start( 500000 );                                     // start timer to check moving process
                        if( mot==_close ) m_pcmdclose->settask( 1 );                    // если закрыть, то команду напрваления даем заранее
                    }
                }
                else if( !cmd || mot_old<_opening || m_tcmd.isDone()) {                 // valve state is idle && old state is moving
                    if( _mode<_manual_pulse_open && fabs(pvv-pvl->gettask()) > delta) { // if задание не достигнуто
//                                pvl->settask(pvl->gettask());                         // запустим снова
                    }
                    if( mode<_manual_pulse_open && !m_tcmd.isDone() && mot_old>=_opening && !lsc && !lso) {
                        if( mot_old==_opening ) delta -= (pvl->gettask() - pvv);
                        if( mot_old==_closing ) delta += (pvl->gettask() - pvv);
                        delta = min( fabs(delta), max_delta );
                    }
                    if( _mode>=_manual_pulse_open ) {
                        m_pmod->setvalue( _mode_old );
                        _mode = mode_old; 
                        m_ppos->settask( _value, false );                               // set task value without process start    
                    }
                    if(_lsc) pvl->settask( pvl->getmineng(), false );
                    if(_lso) pvl->settask( pvl->getmaxeng(), false );
                    m_tcmd.reset();
                    mot_old = mot;
                    m_psel->setvalue( 0 );                                              // освободим управление 
                }          
            }
            else if( mot ) {                                                            // if valve moving (motion != 0)
                if( _mode==_auto_pid && outv!=task ) m_ppos->settask(outv, false);      // выдадим на клапан новое расчетное задание
                const bool lsup = (_lso && !_lso_old);
                const bool lsdown = (_lsc && !_lsc_old);
                const bool openReached = (_mode<_manual_pulse_open) && (_task!=maxOpen) && (_value > (_task-delta));
                const bool closeReached = (_mode<_manual_pulse_open)&& (_task!=minOpen) && (_value < (_task+delta));
                const bool stopOp = ( mot%10==_open   && ( lsup   || openReached  ) ); 
                const bool stopCl = ( mot%10==_close  && ( lsdown || closeReached ) );
                const bool fstop  = ( m_tcmd.isDone() || stopOp || stopCl );
                const bool rc0 = (getqual(m_pfc) == OPC_QUALITY_GOOD);                  // качество счетчика 1=гуд
                const bool rc0 = (getqual(m_pcmdopen) == OPC_QUALITY_GOOD);             // качество управляющего модуля 1=гуд
                
                if( rc2 && (fstop || !rc0) ) {                                          // limit switch (open or close) or position reached
                    mot_old = mot;                                                      // or task completed                     
                    pvl->cleartask();
                    if(mot>=_opening) m_pcmdopen->settask( 0 );
                    if(mot%10==_close) m_pcmdclose->settask( 0 );
                    m_tcmd.reset();
                    mot = 0;
                    if(!rc0) mot_old = -1;
                }
                if( !mot_old && (mot==_open || m_tcmd.getTT()>=_lag_start) ) {          // если есть задание        
                    m_pcmdopen->settask( 1 );                                           // даем команду клапану
                    mot_old = mot;
                    mot += 10;
                }
            }
//                    if( pvl->getname()=="FV11" ) 
                cout<<" mot= "<<mot_old<<"|"<<mot<<" cmd= "<<cmd<<"|"<<dir<<" cnt= "<<cnt_old<<"|"<<cnt \
                    <<" pv="<<pvv<<" task= "<<pvl->taskset()<<"|"<<pvl->gettask()<<"|"<<delta \
                    <<" lso= "<<lso_old<<"|"<<lso<<" lsc= "<<lsc_old<<"|"<<lsc<<" I="<<cur\
                    <<" fcQ="<<rc0<<" cvQ="<<rc2<<" pulseW="<<pulsewidth<<" scaleVLV="<<minOpen<<"|"<<maxOpen\
                    <<" t="<<m_tcmd.getTT()<<"|"<<m_tcmd.getPreset()<<"|"<<m_tcmd.isDone()<<endl;                        
            
            setproperty( "task_delta", delta );                    
            setproperty( "motion", mot );
            setproperty( "motion_old", mot_old );
            m_plso->setoldvalue( _lso );   
            m_plsc->setoldvalue( _lsc );

            if( _mode != _mode_old && _mode<_manual_pulse_open && _mode!=_auto_time && _mode_old!=_auto_time \
                    && mot==_no_motion && mot_old==_no_motion ) {
                m_pmod->setoldvalue( _mode );
                cout<<"valve proc change mode "<<mode_old<<"|"<<mode<<endl; 
            }
            break;
    }
    return rc;
}
    // Управление клапаном
            case _valveProcessing:
                if( args.size() >= 9 && res.size() >= 1 ) {

                    pthread_mutex_lock( &mutex_tag );
                    ctag* pmode       = args[0];  
                    ctag* psv         = args[1];  
                    ctag* pfc         = args[2];
                    ctag* plso        = args[3];
                    ctag* plsc        = args[4];
                    ctag* pcmd        = args[5];
                    ctag* pdir        = args[6]; 
                    ctag* ppvp        = args[8];  
                    ctag* pvl         = res[0];
                    int16_t cnt         = pfc->getvalue();
                    int16_t cnt_old     = pfc->getoldvalue();
                    int16_t lso         = plso->getvalue();
                    int16_t lso_old     = plso->getoldvalue();
                    int16_t lsc         = plsc->getvalue();
                    int16_t lsc_old     = plsc->getoldvalue();
                    int16_t cmd         = pcmd->getvalue();
                    int16_t cmd_old     = pcmd->getoldvalue(); 
                    int16_t dir         = pdir->getvalue();
                    double  cur         = args[5]->getvalue();
                    int16_t sw          = psv->getvalue();                   
                    int16_t mode        = pmode->getvalue();
                    int16_t mode_old    = pmode->getoldvalue();
                    double  pt          = ppvp->getvalue();
                    double  pttask      = ppvp->gettask();   
                    int16_t mot, mot_old, iscfgd, nv;
                    double  pvv         = pvl->getvalue();
                    double  delta, max_delta, outv;
                    int16_t rc0         = (pfc->getquality()!=OPC_QUALITY_GOOD);
                    int16_t rc2         = (pcmd->getquality()!=OPC_QUALITY_GOOD); 
                    int16_t pulsewidth=0;
                    int32_t pollT;
                    const double task   = pvl->gettask();
                    const double maxOpen= pvl->getmaxeng();
                    const double minOpen= pvl->getmineng();

                    pvl->getproperty( "configured", iscfgd );
                    pvl->getproperty( "valve", nv );
                    pvl->getproperty( "out", outv );
                   
                    pthread_mutex_unlock( &mutex_tag );

   
//                    if( pvl->getname()=="FV11" ) 
//                        cout<<"vlv proc "<<pvl->getname()<<" selV="<<sw<<" curV="<<nv<<" mode="<<mode_old<<"|"<<mode<<" isCfg="<<iscfgd<<endl;

                    switch(mode) {
                        case _auto_pid:                                                             // PID
                            break;
                        case _auto_time:                                                            // автомат по времени
                            outv = task;
                            break;
                        default:                                                                    // если ручной режим
                            pvl->setproperty( "out", pvl->getvalue() );
                            if( pttask!=pt ) ppvp->settask( pt, false );             // значение сохраним в задание
                    }
                    if( !mode  )  {
/                        break; 
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
                            if( (mode==_auto_pid ) && outv!=task ) pvl->settask(outv);                   
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
                                        if( task - pvv > delta || task==maxOpen ) {              // cmd to open
                                            if(!lsc || !cnt) mot = _open;                       // if closed wait for reset counter
                                        }                                                
                                        else                                             
                                        if( pvv - task > delta || task==minOpen ) {              // cmd to close
                                            if(!lso || !cnt) mot = _close;                      // if opened wait for reset counter
                                        }
                                        else pvl->cleartask();
                                }
                            }
                            if( mot ) {
                                psv->setvalue( nv );                                        // захватим управление  
                                if( mode>=_manual_pulse_open ) {
                                    m_tcmd.start( ( (mot==_open)?pulsewidth:pulsewidth+_lag_start ) );
                                }
                                else
                                    m_tcmd.start( 500000 );                                     // start timer to check moving process
                                if( mot==_close ) pdir->settask( 1 );
                            }
                        }
                        else if( !cmd || mot_old<_opening || m_tcmd.isDone()) {                 // valve state is idle && old state is moving
                            if( mode<_manual_pulse_open && fabs(pvv-pvl->gettask()) > delta) {  // if задание не достигнуто
//                                pvl->settask(pvl->gettask());                                 // запустим снова
                            }
                            if( mode<_manual_pulse_open && !m_tcmd.isDone() && mot_old>=_opening && !lsc && !lso) {
                                if( mot_old==_opening ) delta -= (pvl->gettask() - pvv);
                                if( mot_old==_closing ) delta += (pvl->gettask() - pvv);
                                delta = min( fabs(delta), max_delta );
                            }
                            if( mode>=_manual_pulse_open ) {
                                pmode->setvalue( mode_old );
                                mode = mode_old; 
                                pvl->settask( pvv, false );                                     // set task value without process start    
                            }
                            if(lsc) pvl->settask( pvl->getmineng(), false );
                            if(lso) pvl->settask( pvl->getmaxeng(), false );
                            m_tcmd.reset();
                            mot_old = mot;
                            psv->setvalue( 0 );                                             // освободим управление 
                        }          
                    }
                    else if( mot ) {                                                            // if valve moving (motion != 0)
                        if( mode==_auto_pid && outv!=task ) pvl->settask(outv, false);          // выдадим на клапан новое расчетное задание
                        const bool lsup = (lso && !lso_old);
                        const bool lsdown = (lsc && !lsc_old);
                        const bool openReached = (mode<_manual_pulse_open) && (task!=maxOpen) && (pvv > (task-delta));
                        const bool closeReached = (mode<_manual_pulse_open)&& (task!=minOpen) && (pvv < (task+delta));
                      
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
                            <<" pv="<<pvv<<" task= "<<pvl->taskset()<<"|"<<pvl->gettask()<<"|"<<delta \
                            <<" lso= "<<lso_old<<"|"<<lso<<" lsc= "<<lsc_old<<"|"<<lsc<<" I="<<cur\
                            <<" fcQ="<<rc0<<" cvQ="<<rc2<<" pulseW="<<pulsewidth<<" scaleVLV="<<minOpen<<"|"<<maxOpen\
                            <<" t="<<m_tcmd.getTT()<<"|"<<m_tcmd.getPreset()<<"|"<<m_tcmd.isDone()<<endl;                        
                    
                    pvl->setproperty( "task_delta", delta );                    
                    pvl->setproperty( "motion", mot );
                    pvl->setproperty( "motion_old", mot_old );
                    plso->setoldvalue( lso );   
                    plsc->setoldvalue( lsc );

                    if( mode != mode_old && mode<_manual_pulse_open && mode!=_auto_time && mode_old!=_auto_time \
                            && mot==_no_motion && mot_old==_no_motion ) {
                        pmode->setoldvalue( mode );
                        cout<<"valve proc change mode "<<mode_old<<"|"<<mode<<endl; 
                    }
                }
                break;
    return rc;
}
/*
            case _valveCalibrate:                                                       // режим конфигурирования
                
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
            case _pidValveControl:                                              // алгоритм управления клапанами по PID
                if( args.size() >= 4 && res.size() >= 1 ) {
                    ctag*   pmode         = args[0];                            // режим клапана 
                    ctag*   plso          = args[1];                            // конечник открыто клапана 
                    ctag*   plsc          = args[2];                            // конечник открыто клапана 

                    double  pt            = args[3]->getvalue();                // значение контролируемого параметра
                    double  pttask        = args[3]->gettask();                 // задание контролируемого параметра

                    ctag*   pvl           = res[0];                             // клапан 

                    int32_t pollT; 
                    const int16_t rc2     = (args[3]->getquality()!=OPC_QUALITY_GOOD); 
                    const int16_t lso     = plso->getvalue();
                    const int16_t lsc     = plsc->getvalue();
                   
                    if( pmode->getvalue()!=_auto_pid || rc2 ) break;

                    getproperty( "period", pollT );

                    if( (m_twait.isDone() || !m_twait.isTiming()) ) {
                        double sp, err, err1, err2, kp, kd, ki, hi, lo, piddead, outv;
                        int16_t pidtype;
                    
                        pvl->getproperty( "out", outv );
                        pvl->getproperty( "err1", err1 );
                        pvl->getproperty( "err2", err2 );
                        pvl->getproperty( "kp", kp );
                        pvl->getproperty( "kd", kd );
                        pvl->getproperty( "ki", ki );
                        pvl->getproperty( "hi", hi );
                        pvl->getproperty( "lo", lo );
                        getproperty( "pidtype", pidtype );
                        getproperty( "piddead", piddead );   
                        err = (pidtype==2) ? pttask - pt : pt - pttask;                     // прямой (2) или обратный
                        err = (fabs(err)>piddead) ? err : 0;                                // 
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
                    res[0]->setrawval( raw );   
                    res[0]->setquality( OPC_QUALITY_GOOD );
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
                    res[0]->setquality( OPC_QUALITY_GOOD );
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
                    res[0]->setquality( OPC_QUALITY_GOOD );
                }
                else if( res.size() ) res[0]->setquality(OPC_QUALITY_BAD);   
                break;
            default: rc = EXIT_FAILURE;
        }
    }
    else rc = init();
*/    
    //    cout<<endl;
    return rc;
}

