#include "unit.h"
#include "algo.h"

vlvmode vmodes;

cunit::cunit() { 
    m_fInit = 0; 
    m_motion = _no_motion;
    m_cmd = 0;
    m_lastcmd = 0;
    m_cmdstatus = 0;
    m_status = 0;
    m_motion = 0;
    m_task   = 0;
    m_pid.kp = 1;
    m_pid.ki = 0.1;
    m_pid.kd = 0;
    m_pid.out = 0;
    m_pid.period = 10000;
    m_pid.err1 = 0;
    m_pid.err2 = 0;
    m_pid.sp = 0;
    m_pid.enable = false;
    m_pulsew = 2000;
    m_plsc      = NULL;
    m_pfc       = NULL;
    m_ppos      = NULL;
    m_pmod      = NULL;
    m_psel      = NULL;
    m_pcmdopen  = NULL;
    m_pcmdclose = NULL;
    m_pblock    = NULL;
    m_preg      = NULL;
}

int16_t cunit::init() {
    int16_t rc = _exOK;

    if( !m_fInit ) {
        string snam[10];
        getproperty("type", m_nType);   
        /*
        switch( m_nType ) {
            case _valve: {
                m_delta = 1;
                m_error = 0;
                // получим имена переменных состояния/управления клапаном
                int res = { getproperty("lso",      snam[0]) | \
                getproperty("lsc",      snam[1]) | \
                getproperty("fc",       snam[2]) | \
                getproperty("pos",      snam[3]) | \
                getproperty("mode",     snam[4]) | \
                getproperty("selected", snam[5]) | \
                getproperty("cmd1",     snam[6]) | \
                getproperty("cmd2",     snam[7]) | \
                getproperty("block1",   snam[8]) | \
                getproperty("reg",      snam[9]) | \
                getproperty("num",      m_num  ) | \
                getproperty("name",     m_name); }
                
                if(res) { rc=_exBadParam; break; }
                // получим адреса переменных состояния/управления клапаном
                                    m_plso      = getaddr( snam[0] );
                if( !m_plso )       m_plsc      = getaddr( snam[1] );
                if( !m_plsc )       m_pfc       = getaddr( snam[2] );
                if( !m_pfc )        m_ppos      = getaddr( snam[3] );
                if( !m_ppos )       m_pmod      = getaddr( snam[4] );
                if( !m_pmod )       m_psel      = getaddr( snam[5] );
                if( !m_psel )       m_pcmdopen  = getaddr( snam[6] );
                if( !m_pcmdopen )   m_pcmdclose = getaddr( snam[7] );
                if( !m_pcmdclose )  m_pblock    = getaddr( snam[8] );
                if( !m_pblock )     m_preg      = getaddr( snam[9] );
                
                if( !m_preg ) { rc=_exBadAddr; break; }

                // получим настройки ПИД-регулятора
                int res = { getproperty("kp",       pid.kp ) | \
                            getproperty("ki",       pid.ki ) | \
                            getproperty("kd",       pid.kd ) | \
                            getproperty("period",   pid.period ) | \
                            getproperty("pid_dead", pid.dead ) | \
                            getproperty("pid_type", pid_type ); }
                pid.enable = { res == _exOK; }

                if( !_lso && !_lsc )                                                    // если нет конечников
                    m_ppos->setrawval( (m_ppos->getmaxraw()+m_ppos->getminraw())/2 );   // будем считать, что клапан посередине
                else if( _lso && !_lsc )                                                // иначе - соответственно...
                    m_ppos->setrawval( m_ppos->getmaxraw() );                           // полностью открыт
                else if( !_lso && _lsc )
                    m_ppos->setrawval( m_ppos->getminraw() );                           // полностью закрыт  
                else m_error = _2_ls;                                                   // сработаны 2 конечника
                
                if( m_error ) { rc=_exBadDippedHW; return; }
                    
                m_pid.sp =  m_preg->getvalue();
                m_pid.out=  m_ppos->getvalue();
                m_delta = 1;            
                m_maxdelta = 10;           
                m_status = 0;
            }
            break;
        }
        m_fInit = true;
        */
    }

    return rc;
}

void cunit::pidEval( void* pIn ) {
    if(pIn==NULL) return;    
    ctag*   sp        = (ctag*)pIn;
    double  pt        = sp->getvalue();       // значение контролируемого параметра
    double  pttask    = sp->gettask();        // задание контролируемого параметра

    if( m_pid.enable && (m_pid.timer.isDone() || !m_pid.timer.isTiming()) ) {
        double err, err1, err2, hi, lo, piddead, outv;

        outv = m_pid.out;
        err = (m_pid.type==2) ? pttask - pt : pt - pttask;                  // прямой (2) или обратный
        err = (fabs(err)>m_pid.dead) ? err : 0;                             // 
        outv =  outv + \
                m_pid.kp * ( err-m_pid.err1 + \
                m_pid.ki*err + \
                m_pid.kd*(err-m_pid.err1*2+m_pid.err2) );                   // вычислим задание для клапана
        outv = min( hi, outv );                                             // загоним в границы
        outv = max( lo, outv );
        m_pid.out =  outv;                                                  // сохраним значение задания
        m_pid.err2 = m_pid.err1;
        m_pid.err1 = err;
        m_pid.timer.start( m_pid.period );
        cout<<"pid "<<m_preg->getname()<<" pt="<<pt<<" pttask="<<pttask<<\
            " err="<<err<<" | "<<m_pid.err1<<" | "<<m_pid.err2<<\
            " kp="<<m_pid.kp<<" ki="<<m_pid.ki<<" kd="<<m_pid.kd<<" out="<<outv<<endl;
    }
}

int16_t cunit::stop() {
    int16_t         rc = _exOK;;
//    vector<uint8_t> arr_qual;
//    uint8_t         nqual;

//        arr_qual.resize(args.size());
//        transform( args.begin(), args.end(), arr_qual.begin(), getqual );                          // get quality 
//        uint8_t nqual = accumulate( arr_qual.begin(), arr_qual.end(), 0, bit_or<uint8_t>() );      // summary quality evaluate
//        cout<<" nType="<<nType<<" quality="<<hex<<(int16_t(nqual)&0x00ff)<<dec; 
    if( (m_pcmdopen->getquality() | m_pcmdclose->getquality()) != OPC_QUALITY_GOOD ) {
        rc = _exBadIO;
    }
    else {
        m_pcmdopen->settask( 0 );      
        m_pcmdclose->settask( 0 );
    }
    return rc;
}

int16_t cunit::getstate() {
    int16_t rc = EXIT_SUCCESS;
    double  raw_set;        
    init();

    double  delta, max_delta;
   
    switch( m_nType ) {
        case _valve: 
            {
//
// -------- вычислим текущее положение
//
           
            int16_t mot = m_direction;
            
//          getproperty( "motion", mot );                                 // вспомним, была ли движуха
//          res[0]->getproperty( "count", cnt_old ); 
            
//          cout<<" valveEval="<<res[0]->getname()<<" cnt= "<<cnt_old<<"|"<<cnt<<" raw="<<raw<<" cmd = "<<cmd<<"|"<<dir \
            <<" lso="<<lso_old<<"|"<<lso<<" lsc="<<lsc_old<<"|"<<lsc\
            <<" fcQ="<<hex<<int16_t(args[1]->getquality())<<dec<<" isCfgd="<<iscfgd<<endl;                   
            if( _cmd_tsk ) mot = 1;                                         // команда движения
            if( _cmd_tsk && _dir_tsk ) mot = -1;                            // команда обратного движения
            if(_cnt) raw_set = _raw + mot*(_cnt - _cnt_old);                // считаем положение клапана в импульсах
            if(_lso && mot>0) raw_set = m_ppos->getmaxraw();
            if(_lsc && mot<0) raw_set = m_ppos->getminraw(); 
//                        cout<<" newRaw= "<<raw;
            bool fReset = (_lso && _lso_old) || (_lsc && _lsc_old) || _cnt>20000;
            if( !_cmd_tsk && fReset && _cnt && m_pfc->getquality()==OPC_QUALITY_GOOD && !m_ppos->taskset() ) { 
                /*
                if( iscfgd==-3 ) {                                          // если режим калибровки
                    iscfgd=1;
                    m_ppos->setrawscale( 0, _cnt );
                    iscfgd=1;   
                    delta = _cnt/1000;
                    delta = min( fabs(delta), max_delta );
                    m_ppos->setrawval( _lsc ? 0: _cnt );
                    m_pmod->setvalue( m_pmod->getoldvalue() );
                }
                */
                m_pfc->settask( 0 );                                        // сброс счетчика
                m_plso->setoldvalue( _lso );
                m_plsc->setoldvalue( _lsc );
            } 
//            cout<<" newRaw= "<<raw<<endl;
//            setproperty( "motion", mot );                                 // сохраним данные о движении до следующего опроса  
//            res[0]->setproperty( "count", cnt ); 
//            if( iscfgd>0 ) {  
                
            m_pfc->setoldvalue( _cnt );   
            m_ppos->setrawval( raw_set );                                   // сохраним значение положения в сырых единицах
//            }
            m_direction = mot;                                              // сохраним данные о движении до следующего опроса
            
            if(_cmd_val && _dir_val && _dir_tsk) mot = _closing;
            else if(_dir_val && _dir_tsk) mot = _closeAck;
            else if(!_dir_val && _dir_tsk) mot = _close;
            else if(_cmd_val && _cmd_tsk) mot = _opening;
            else if(!_cmd_val && _cmd_tsk) mot = _open;
            else if(_cmd_val && !_cmd_tsk) mot = _stopping;
            else mot = _no_motion;
            m_motion = mot;
            }
            break;

        case _pump_station:
            break;
    }
    return rc;
}

int16_t cunit::control( void ) {
    int16_t rc = _exOK;

    switch( m_nType ) {
        case _valve:
            uint8_t nqual = (getqual(m_pfc) | getqual(m_pcmdopen) | getqual(m_plso));

            if( m_motion==_no_motion ) {                                                // клапан стоит
                if( m_cmdstatus==_cmd_completed || m_cmdstatus==_cmd_failed ) {       // и есть признак успешного завершения команды 
                    if( _mode<_manual_pulse_open && !m_tcmd.isDone() && !_lsc && !_lso && m_cmdstatus!=_cmd_failed ) {
                        if( m_cmd == _cmd_open  ) m_delta -= (_task - _value);          // скорректируем 
                        if( m_cmd == _cmd_close ) m_delta += (_task - _value);
                        m_delta = min( fabs(m_delta), m_maxdelta );
                    }
                    if( _mode>=_manual_pulse_open ) {
                        m_pmod->setvalue( _mode_old );
                        m_ppos->settask( _value, false );                               // set task value without process start    
                    }
                    m_tcmd.reset();
                    m_psel->setvalue( _no_valve );                                      // освободим управление 
                    m_cmdstatus = _cmd_clear;
                    m_cmd = _cmd_none;
                    /*
                    if( _mode<_manual_pulse_open && fabs(pvv-pvl->gettask()) > delta) { // if задание не достигнуто
//                                pvl->settask(pvl->gettask());                         // запустим снова
                    }
                    if(_lsc) pvl->settask( pvl->getmineng(), false );
                    if(_lso) pvl->settask( pvl->getmaxeng(), false );
                    else if( !cmd || mot_old<_opening || m_tcmd.isDone()) {             // valve state is idle && old state is moving
                    }          
                    */
                }
                if( m_cmd==_cmd_none && m_cmdstatus==_cmd_clear ) {                    // и нет команд 
                    if( (_mode==_auto_pid ) && m_pid.out!=_task ) m_ppos->settask(m_pid.out); 
                    bool fOpen = m_ppos->taskset();
                    fOpen |=  (_mode==_manual_pulse_open );
                    fOpen |=  ( (!_lsc || !_cnt) && ( _task - _value > m_delta || _task==_maxOpen ) && !_lso );
                    bool fClose = m_ppos->taskset();
                    fClose |= (_mode==_manual_pulse_close);
                    fClose |= ( (!_lso || !_cnt) && ( _value - _task > m_delta || _task==_minOpen ) && !_lsc );
                    if( fOpen ) m_cmd = _cmd_open;  
                    else
                    if( fOpen ) m_cmd = _cmd_close;
                    else m_ppos->cleartask();
                }
                if( m_cmd && nqual==OPC_QUALITY_GOOD ) {                                // есть команда?
                    m_cmdstatus = _cmd_accepted; 
                    m_psel->setvalue( m_num );                                          // захватим управление  
                    if( _mode>=_manual_pulse_open ) {                                   // если импульсный режим
                        m_tcmd.start( ( (m_cmd==_open)?m_pulsew:m_pulsew+_lag_start ) );
                    }
                    else
                        m_tcmd.start( _waitcomplete );                                  // start timer to check moving process
                    if( m_cmd==_close ) m_pcmdclose->settask( 1 );                      // если закрыть, то команду направления даем заранее
                }
            }
            
            if( (m_motion==_open || m_motion==_closeAck) && nqual==OPC_QUALITY_GOOD ) { // если есть задание
                    /*(m_motion==_close && m_tcmd.getTT()>=_lag_start)*/        
                m_pcmdopen->settask( 1 );                                               // даем команду движения клапану
            }
            
            const bool rc0 = (getqual(m_pfc) == OPC_QUALITY_GOOD);                  // качество счетчика 1=гуд
            const bool rc2 = (getqual(m_pcmdopen) == OPC_QUALITY_GOOD);             // качество управляющего модуля 1=гуд
            if(  m_motion>=_opening ) {                                                // клапан движется
                if( _mode==_auto_pid && m_pid.out!=_task ) m_ppos->settask(m_pid.out, false);// выдадим на клапан новое расчетное задание
                const bool lsup   = (_lso && !_lso_old);                                // limit switch (open or close) or position reached
                const bool lsdown = (_lsc && !_lsc_old);
                const bool openReached  = (_mode<_manual_pulse_open) && (_task!=_maxOpen) && (_value > (_task-m_delta));// or task completed
                const bool closeReached = (_mode<_manual_pulse_open) && (_task!=_minOpen) && (_value < (_task+m_delta));
                const bool stopOp = ( m_motion%10==_open   && ( lsup   || openReached  ) ); 
                const bool stopCl = ( m_motion%10==_close  && ( lsdown || closeReached ) );
                const bool fstop  = ( m_tcmd.isDone() || stopOp || stopCl );
                
                if( rc2 && (fstop || !rc0) ) {                                          
                    m_ppos->cleartask();
                    if(m_motion>=_opening) m_pcmdopen->settask( 0 );
                    if(m_motion%10==_close) m_pcmdclose->settask( 0 );
                    m_tcmd.reset();
                    if(!rc0) m_cmdstatus = _cmd_failed;
                    m_cmdstatus = _cmd_completed;   
                }
            }
            if( m_ppos->getname()=="FV11" ) 
                cout<<" mot= "<<m_motion<<" cmd= "<<_cmd_val<<"|"<<_dir_val<<" cnt= "<<_cnt_old<<"|"<<_cnt \
                    <<" pv="<<_value<<" task= "<<m_ppos->taskset()<<"|"<<_task<<"|"<<m_delta \
                    <<" lso= "<<_lso_old<<"|"<<_lso<<" lsc= "<<_lsc_old<<"|"<<_lsc<<" I="<<m_pblock->getvalue()\
                    <<" fcQ="<<rc0<<" cvQ="<<rc2<<" pulseW="<<m_pulsew<<" scaleVLV="<<_minOpen<<"|"<<_maxOpen\
                    <<" t= "<<m_tcmd.getTT()<<" of "<<m_tcmd.getPreset()<<"|done="<<m_tcmd.isDone()<<endl;                        
/*            
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
*/
            break;
    }
    return rc;
}

