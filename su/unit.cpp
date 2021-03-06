#include "unit.h"
#include "algo.h"
#include "tagdirector.h"
#include "unitdirector.h"

vlvmode vmodes;
vlvmode vstatuses;
vlvmode pstatuses;

cunit::cunit() { 
    m_fInit = 0; 
    m_motion = _no_motion;
    m_cmd = _cmd_none;
    m_mode = _not_proc;
    m_mode_old = _not_proc;
    m_cmdstatus = _cmd_clear;
    m_status = 0;
    m_task   = 0;
    m_task_go= false;
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
    m_plso      = NULL;
    m_plsc      = NULL;
    m_pfc       = NULL;
    m_ppos      = NULL;
    m_pmod      = NULL;
    m_psel      = NULL;
    m_pcmdopen  = NULL;
    m_pcmdclose = NULL;
    m_pblock    = NULL;
    m_preg      = NULL;
    m_pother    = NULL;
    m_minAuto   = 0;
    m_maxAuto   = 100;
    m_direction = 0;
}

int16_t cunit::init() {
    int16_t rc = _exOK;
    if( !m_fInit ) {
        vector<string> snam;
        getproperty("type", m_nType);   
        
        switch( m_nType ) {
            case _valve: {
                m_delta = 1;
                m_error = 0;
                snam.resize(11);
                
                // получим имена переменных состояния/управления клапаном
                int res = 
                getproperty("lso",      snam[0]) | 
                getproperty("lsc",      snam[1]) | 
                getproperty("fc",       snam[2]) | 
                getproperty("pos",      snam[3]) | 
                getproperty("mode",     snam[4]) | 
                getproperty("selected", snam[5]) | 
                getproperty("cmd1",     snam[6]) | 
                getproperty("cmd2",     snam[7]) | 
                getproperty("block1",   snam[8]) | 
                getproperty("reg",      snam[9]) | 
                getproperty("ref",      snam[10]) | 
                getproperty("minAuto",  m_minAuto) | 
                getproperty("maxAuto",  m_maxAuto) | 
                getproperty("num",      m_num  ) | 
                getproperty("name",     m_name); 
                cout<<"unit "<<m_name<<" init ";
                for_each( snam.begin(), snam.end(), printdata<string> );
                cout<<" res="<<res<<endl;

                if(res) { rc=_exBadParam; cout<<" bad param\n"; break; }
                // получим адреса переменных состояния/управления клапаном
                                  m_plso      = getaddr( snam[0] );// cout<<"a"<<hex<<long(m_plso)<<" ";
                if( m_plso )      m_plsc      = getaddr( snam[1] );// cout<<"a"<<long(m_plsc)<<" ";
                if( m_plsc )      m_pfc       = getaddr( snam[2] );// cout<<"a"<<long(m_pfc)<<" ";
                if( m_pfc )       m_ppos      = getaddr( snam[3] );// cout<<"a"<<long(m_ppos)<<" ";
                if( m_ppos )      m_pmod      = getaddr( snam[4] );// cout<<"a"<<long(m_pmod)<<" ";
                if( m_pmod )      m_psel      = getaddr( snam[5] );// cout<<"a"<<long(m_psel)<<" ";
                if( m_psel )      m_pcmdopen  = getaddr( snam[6] );// cout<<"a"<<long(m_pcmdopen)<<" ";
                if( m_pcmdopen )  m_pcmdclose = getaddr( snam[7] );// cout<<"a"<<long(m_pcmdclose)<<" ";
                if( m_pcmdclose ) m_pblock    = getaddr( snam[8] );// cout<<"a"<<long(m_pblock)<<" ";
                if( m_pblock )    m_preg      = getaddr( snam[9] );// cout<<"a"<<long(m_preg)<<dec<<endl;
                if( m_preg )      m_pother    = getaddrunit( snam[10] );// cout<<"a"<<long(m_preg)<<dec<<endl;
                
                if( !m_pother ) { rc=_exBadAddr; cout<<" bad addr\n"; /*break;*/ }

                // получим настройки ПИД-регулятора
                res = getproperty("kp",       m_pid.kp ) | \
                      getproperty("ki",       m_pid.ki ) | \
                      getproperty("kd",       m_pid.kd ) | \
                      getproperty("period",   m_pid.period ) | \
                      getproperty("pid_dead", m_pid.dead ) | \
                      getproperty("pid_type", m_pid.type ); 
                m_pid.enable =  ( res == _exOK ); 

                if( !_lso && !_lsc ) {                                                  // если нет конечников
                    /*
                    string sval("value"); 
                    string snam = m_ppos->getname();
                    sval = getPersistData( snam, sval );  
                    m_ppos->setvalue( atof( sval.c_str() ) );
                    */
//                    m_ppos->setrawval( (m_ppos->getmaxraw()+m_ppos->getminraw())/2 );   // будем считать, что клапан посередине
                }
                else if( _lso && !_lsc )                                                // иначе - соответственно...
                    m_ppos->setrawval( m_ppos->getmaxraw() );                           // полностью открыт
                else if( !_lso && _lsc )
                    m_ppos->setrawval( m_ppos->getminraw() );                           // полностью закрыт  
                //else m_error = _2_ls;                                                   // разомкнуто 2 конечника
                
                if( m_error ) { 
                    rc=_exBadDippedHW; 
                    cout<<" bad dipped HW\n"; 
//                    break; 
                }
                    
                m_pid.sp =  m_preg->getvalue();
                m_pid.out=  m_ppos->getvalue();
                settask(0, false);
                m_delta = 1;            
                m_maxdelta = 10;           
                m_status = 0;
                m_fInit = true;
                m_plso->setoldvalue(0);
                m_plsc->setoldvalue(0);
                m_pfc->settask(0);
            }
            break;
            case _pump_station: {
                m_delta = 1;
                m_error = 0;
                snam.resize(11);
                
                // получим имена переменных состояния/управления клапаном
                int res = 
                getproperty("run",      snam[0]) | 
                getproperty("fault",    snam[1]) | 
                getproperty("ready",    snam[2]) | 
                getproperty("speed",    snam[3]) | 
                getproperty("mode",     snam[4]) | 
                getproperty("selected", snam[5]) | 
                getproperty("task",     snam[6]) | 
                getproperty("start",    snam[7]) | 
                getproperty("stop",     snam[8]) | 
                getproperty("reg",      snam[9]) | 
                getproperty("ref",      snam[10]) | 
                getproperty("minAuto",  m_minAuto) | 
                getproperty("maxAuto",  m_maxAuto) | 
                getproperty("num",      m_num  ) | 
                getproperty("name",     m_name); 
                cout<<"unit "<<m_name<<" init ";
                for_each( snam.begin(), snam.end(), printdata<string> );
                cout<<" res="<<res<<endl;

                if(res) { rc=_exBadParam; cout<<" bad param\n"; break; }
                // получим адреса переменных состояния/управления клапаном
                                  m_prun      = getaddr( snam[0] ); cout<<"a"<<hex<<long(m_prun)<<" ";
                if( m_prun )      m_pflt      = getaddr( snam[1] ); cout<<"a"<<long(m_pflt)<<" ";
                if( m_pflt )      m_pready    = getaddr( snam[2] ); cout<<"a"<<long(m_pready)<<" ";
                if( m_pready )    m_ppos      = getaddr( snam[3] ); cout<<"a"<<long(m_ppos)<<" ";
                if( m_ppos )      m_pmod      = getaddr( snam[4] ); cout<<"a"<<long(m_pmod)<<" ";
                if( m_pmod )      m_psel      = getaddr( snam[5] ); cout<<"a"<<long(m_psel)<<" ";
                if( m_psel )      m_ptask     = getaddr( snam[6] ); cout<<"a"<<long(m_ptask)<<" ";
                if( m_ptask )     m_pstart    = getaddr( snam[7] ); cout<<"a"<<long(m_pstart)<<" ";
                if( m_pstart )    m_pstop     = getaddr( snam[8] ); cout<<"a"<<long(m_pstop)<<" ";
                if( m_pstop )     m_preg      = getaddr( snam[9] ); cout<<"a"<<long(m_preg)<<dec<<endl;
                if( m_preg )      m_pother    = getaddrunit( snam[10] ); cout<<"a"<<long(m_pother)<<dec<<endl;
                
                if( !m_pother ) { rc=_exBadAddr; cout<<" bad addr\n"; /*break;*/ }

                // получим настройки ПИД-регулятора
                res = getproperty("kp",       m_pid.kp ) | \
                      getproperty("ki",       m_pid.ki ) | \
                      getproperty("kd",       m_pid.kd ) | \
                      getproperty("period",   m_pid.period ) | \
                      getproperty("pid_dead", m_pid.dead ) | \
                      getproperty("pid_type", m_pid.type ); 
                m_pid.enable =  ( res == _exOK ); 

                m_pid.sp  = m_preg->getvalue();
                m_pid.out = m_ptask->getvalue();
                settask(0, false);
                m_status = 0;
                m_fInit = true;
            }
            break;
        }
        m_twait.reset();
        m_tcmd.reset();
    }

    return rc;
}

void cunit::pidEval( void* pIn ) {
    if(pIn==NULL) return;    
    ctag*   sp        = (ctag*)pIn;
    double  pt        = sp->getvalue();       // значение контролируемого параметра
    double  pttask    = sp->gettask();        // задание контролируемого параметра
    
    if( sp!=m_preg ) m_preg=sp;

    if( m_pid.enable && (m_pid.timer.isDone() || !m_pid.timer.isTiming()) ) {
        double err, outv;

        outv = m_pid.out;
        err = (m_pid.type==2) ? pttask - pt : pt - pttask;                  // прямой (2) или обратный
        err = (fabs(err)>m_pid.dead) ? err : 0;                             // 
        outv =  outv + \
                m_pid.kp * ( err-m_pid.err1 + \
                m_pid.ki*err + \
                m_pid.kd*(err-m_pid.err1*2+m_pid.err2) );                   // вычислим задание для клапана
        outv = min( m_maxAuto, outv );                                      // загоним в границы
        outv = max( m_minAuto, outv );
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
    int16_t rc = _exOK;

    if( (m_pcmdopen->getquality() | m_pcmdclose->getquality()) != OPC_QUALITY_GOOD ) {
        rc = _exBadIO;
    }
    else {
        m_pcmdopen->settask( 0 );      
        m_pcmdclose->settask( 0 );
        m_cmd =_cmd_none;
        m_cmdstatus =_cmd_clear;
    }
    return rc;
}

int16_t cunit::getstate() {
    int16_t rc = _exOK;
    double  raw_set; 
    int16_t mot;   
    uint8_t _nQual=OPC_QUALITY_GOOD;
    //    cout<<"getstate call init "<<init()<<endl;
    rc = init();
    if( !m_fInit ) return _exInitFailed;

    m_position = m_ppos->getvalue();                                // get положение клапана (or pump speed) в инж. единицах
    setproperty("value", m_position);
    
    switch( m_nType ) {
        case _valve: 
        {
//
// -------- вычислим текущее состояние
//
            if( m_ppos->getname()=="FV11" && 0 ) { 
                cout<<dec<<' '<<m_ppos->getname()<<" mode="<<m_pmod->getoldvalue()<<"|"<<m_pmod->getvalue()
                    <<"|"<<m_mode_old<<"|"<<m_mode
                    <<" ucmd="<<m_cmd<<" ucmdst="<<m_cmdstatus
                    <<" mot= "<<m_motion<<" cmd= "<<_cmd_val<<"|"<<_dir_val<<" cnt= "<<_cnt_old<<"|"<<_cnt
                    <<" pv="<<m_position<<" task= "<<m_task<<"|"<<m_delta
                    <<" lso= "<<_lso_old<<"|"<<_lso<<"|"<<m_plso->getrawval()
                    <<" lsc= "<<_lsc_old<<"|"<<_lsc<<"|"<<m_plsc->getrawval()
                    <<" I="<<m_pblock->getvalue()<<" pulseW="<<m_pulsew
                    <<" scaleVLV="<<_minOpen<<"|"<<_maxOpen<<hex<<" stat="<<m_status<<dec
                    <<" t= "<<m_twait.getTT()<<" of "<<m_twait.getPreset()<<endl;                       
            }

            mot = m_direction;
/*          
            cout<<" valveEval="<<res[0]->getname()<<" cnt= "<<cnt_old<<"|"<<cnt<<" raw="<<raw<<" cmd = "<<cmd<<"|"<<dir \
            <<" lso="<<lso_old<<"|"<<lso<<" lsc="<<lsc_old<<"|"<<lsc\
            <<" fcQ="<<hex<<int16_t(args[1]->getquality())<<dec<<" isCfgd="<<iscfgd<<endl;                   
*/
            if( _cmd_tsk ) mot = 1;                                         // команда движения
            if( _cmd_tsk && _dir_tsk ) mot = -1;                            // команда обратного движения
            raw_set = _raw + mot*(_cnt - _cnt_old);                         // считаем положение клапана в импульсах

//            cout<<" motion="<<mot<<" raw="<<raw_set;
            if(_lso && mot>0) {
//                cout<<" {MAX} ";
                raw_set = m_ppos->getmaxraw();
            }
            if(_lsc && mot<0) {
//                cout<<" {MIN} ";
                raw_set = m_ppos->getminraw();
            }
//                        cout<<" newRaw= "<<raw;
            bool fReset = ((_lso || _lsc) && m_cmdstatus==_cmd_clear) || _cnt>20000;
            //cout<<" doReset?="<<fReset<<endl;
            if( m_cmd==_cmd_none && fReset && _cnt && m_pfc->getquality()==OPC_QUALITY_GOOD ) { 
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
//                cout<<"{ CLEAR }"; 
            } 
            if( !_lso && _lso_old ) m_plso->setoldvalue( _lso );            
            if( !_lsc && _lsc_old ) m_plsc->setoldvalue( _lsc );            
 
            m_pfc->setoldvalue( _cnt );   
            m_ppos->setrawval( raw_set );                                   // сохраним значение положения в сырых единицах
            _nQual = ((m_pfc->getquality() |  m_plso->getquality() | m_pcmdopen->getquality()) & OPC_STATUS_MASK);
            if( _nQual==OPC_QUALITY_GOOD ) {
                if( _lso && _lsc ) _nQual = OPC_QUALITY_DEVICE_FAILURE;
                if( m_pblock && m_pblock->getquality()==OPC_QUALITY_GOOD &&
                        m_pblock->getvalue()>=m_pblock->gethihi() ) _nQual = OPC_QUALITY_LOCAL_OVERRIDE;
                if( m_twait.isDone() && m_mode<_manual_pulse_open ) _nQual = OPC_QUALITY_OUT_OF_SERVICE;
            }

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

            if(m_cmdstatus==_cmd_requested && mot>=_opening) {
                m_cmdstatus = _cmd_accepted;
                m_tcmd.reset();
            }
            if(m_cmdstatus==_cmd_requested && m_tcmd.isDone()) {
                stop();
                m_cmdstatus = _cmd_failed;
                _nQual = OPC_QUALITY_COMM_FAILURE;
                m_tcmd.stop();
            }

            m_ppos->setquality( _nQual );
//            if( m_status<_vlv_fault_1 ) {
                if( _nQual==OPC_QUALITY_GOOD ) {
                   if(mot == _opening) m_status = _vlv_opening;    
                   else if(mot == _open) m_status = _vlv_open;    
                   else if(mot == _closing) m_status = _vlv_closing;    
                   else if(mot == _close || mot == _closeAck) m_status = _vlv_close;    
                   else if(_lso) m_status = _vlv_opened;    
                   else if(_lsc) m_status = _vlv_closed;    
                   else m_status = _vlv_ready; 
                } else {
                    if( _nQual == OPC_QUALITY_DEVICE_FAILURE ) m_status = _vlv_fault_2;
                    else if( _nQual == OPC_QUALITY_OUT_OF_SERVICE ) m_status = _vlv_fault_3;
                    else if( _nQual == OPC_QUALITY_COMM_FAILURE ) m_status = _vlv_fault_4;
                    else if( _nQual == OPC_QUALITY_LOCAL_OVERRIDE ) m_status = _vlv_override;
                    else m_status = _vlv_fault_1;
                }
//            }
        }
        break;

        case _pump_station:
            _nQual = m_prun->getquality();
            /*
            cout<<m_ppos->getname()<<" q="<<hex<<int16_t(_nQual)<<dec
                <<" run="<<m_prun->getvalue()<<" flt="<<m_pflt->getvalue()
                <<" rdy="<<m_pready->getvalue()<<" cmdStart="<<m_pstart->gettask(true)
                <<" cmdStop="<<m_pstop->gettask(true)
                <<" stat="<<m_status<<endl;
            */
//            if( m_status<_p_fault_2 ) {
                if( _nQual==OPC_QUALITY_GOOD ) {
                    if( m_pflt->getvalue() ) m_status = _p_fault_2;
                    else
                    if( m_prun->getvalue() ) {
                        if( int(m_pstop->gettask(true)) ) 
                            m_status = _p_stop;
                        else m_status = _p_running;
                    }
                    else
                    if( m_pstart->gettask(true) ) m_status = _p_start;
                    else if( m_pready->getvalue() ) m_status = _p_ready;
                    else m_status = _p_idle;
                } 
                else {
                    m_status = _p_fault_1;
                }
//            }
        break;
    }
    
    m_ppos->setproperty("status", m_status);

    if(m_ppos->taskset()) {                             // если было задание на положение/скорость, перепишем его в тэг юнита
        if( _nQual==OPC_QUALITY_GOOD ) {
            m_task_go = true;
            m_task = m_ppos->gettask();
            m_ppos->cleartask();
        }
    }
    
    if( m_mode!=_auto_pid ) {                                       // запись уставки регулируемого параметра не в авто режиме
        if(fabs(m_preg->getvalue()-m_preg->gettask())>m_preg->getdead()) 
            m_preg->settask(m_preg->getvalue(), false);
        m_pid.out = m_position;
    }

    if( (m_nType==_valve && m_status>=_vlv_fault_2) ||
        (m_nType==_pump_station && m_status>=_p_fault_2) ) {        // если device не в порядке или есть блокировки
        changemode(_not_proc);                                      // остановим его
    }
    else {
        int16_t nmode = m_pmod->getvalue();
        if( nmode!=m_mode ) {                                                       // если смена режима
            if(!m_mode) m_status = ((m_nType==_valve)?                              // ввод в работу
                    static_cast<int16_t>(_vlv_ready): 
                                    ((m_nType==_pump_station)? 
                                        static_cast<int16_t>(_p_ready): 
                                        static_cast<int16_t>(_vlv_override)));      
            /*
            if( nmode==_auto_time                           // если новый="по времени", у второго меняем тоже                        
                    && m_pother && m_pother->getmode()!=_auto_time) m_pother->changemode( nmode );      
            if( m_mode==_auto_time                          // если старый="по времени", у второго меняем на ручной
                    && m_pother ) m_pother->changemode( _manual );   
            */
            changemode(nmode);                                                      // меняем режим текущего 
        }
    }

    return rc;
}

int16_t cunit::control( void* pIn ) {
    int16_t rc = _exOK;
    uint8_t nqual;
    tagvector* _ptv = (tagvector*)pIn;
   
    if( getstate()!=_exOK ) return _exFail;
    
    if(m_mode!=_not_proc) {
        if( m_mode == _auto_pid && _ptv ) pidEval( (*_ptv)[0] );
        
        switch( m_nType ) {
            case _valve:
            {
                nqual = (getqual(m_pfc) | getqual(m_pcmdopen) | getqual(m_plso));
                
                if( m_motion==_no_motion ) {                                                // клапан стоит
                    if( m_cmdstatus==_cmd_completed || m_cmdstatus==_cmd_failed ) {         // и есть признак успешного завершения команды 
                        if( m_mode<_manual_pulse_open && !m_twait.isDone() && !_lsc && !_lso && m_cmdstatus!=_cmd_failed ) {
                            if( m_cmd == _cmd_open  ) m_delta -= (m_task - m_position);     // скорректируем 
                            if( m_cmd == _cmd_close ) m_delta += (m_task - m_position);
                            m_delta = min( fabs(m_delta), m_maxdelta );
                        }
                        if( m_mode>=_manual_pulse_open ) {                                  // вернемся из импульсного режима
                            revertmode();
                            settask( m_position, false );                                   // set task value without process start    
                            m_twait.reset();
                        }
    //                    m_twait.reset();
                        m_psel->setvalue( _no_valve );                                      // освободим управление 
                        m_cmdstatus = _cmd_clear;
                        m_cmd = _cmd_none; 
                        settask(m_position, false);
                        string _name = m_ppos->getname();                                   // сохраним положение в файл
                        string _attr = "value";
                        string _val = to_string( m_ppos->getvalue() );
                        setPersistData( _name, _attr, _val ); 
                    }
                    if( m_cmd==_cmd_none && m_cmdstatus==_cmd_clear ) {                    // и нет команд 
                        if( (m_mode==_auto_pid ) && m_pid.out!=m_task ) settask(m_pid.out, true); 
                        
                        bool fOpen = ( m_task_go );
                        fOpen = fOpen && !_lso;
                        fOpen = fOpen && ( m_task - m_position > m_delta || m_task==_maxOpen );
                        fOpen = fOpen || ( m_mode==_manual_pulse_open );
                        fOpen = fOpen && ( !(_lsc && _cnt) );
                        
                        bool fClose = ( m_task_go );   
                        fClose = fClose && !_lsc;
                        fClose = fClose && ( m_position - m_task > m_delta || m_task==_minOpen );
                        fClose = fClose || ( m_mode==_manual_pulse_close );
                        fClose = fClose && ( !(_lso && _cnt) );
                        
                        if( fOpen ) m_cmd = _cmd_open;  
                        else
                        if( fClose ) m_cmd = _cmd_close;
                        else m_task_go = false;
                        if( m_cmd ) {
                            m_status = _vlv_ready;
                            m_twait.reset();
                        }
    //                    if( m_ppos->getname()=="FV11" ) 
    //                        cout<<"fopen="<<fOpen<<" fclose="<<fClose;
                    }
                    if( m_cmd && nqual==OPC_QUALITY_GOOD && m_cmdstatus==_cmd_clear ) {          // есть команда?
                        m_task_go=false;   
                        m_cmdstatus = _cmd_requested; 
                        m_tcmd.start( _lag_start );
                        m_psel->setvalue( m_num );                                              // захватим управление  
                        if( m_cmd==_close ) m_pcmdclose->settask( 1 );                          // при ЗАКР команду направления даем заранее
                    }
                }
                
                if( m_cmdstatus==_cmd_requested && nqual==OPC_QUALITY_GOOD && \
                        (m_cmd==_open || m_motion==_closeAck) ) {                               // если есть задание
                    m_pcmdopen->settask( 1 );                                                   // даем команду движения клапану
                    m_twait.start( (m_mode>=_manual_pulse_open)? m_pulsew: _waitcomplete );     // засечем таймер
                }
                
                const bool rc0 = (getqual(m_pfc) == OPC_QUALITY_GOOD);                          // качество счетчика 1=гуд
                const bool rc2 = (getqual(m_pcmdopen) == OPC_QUALITY_GOOD);                     // качество управляющего модуля 1=гуд
                if( m_motion>=_opening && m_motion<_stopping && m_cmdstatus<_cmd_completed ) {  // if клапан движется
                    if( m_mode==_auto_pid && m_pid.out!=m_task ) settask(m_pid.out, false);     // if pid then выдадим на клапан новое задание
                    const bool lsup   = (_lso && !_lso_old);                                    // limit switch open
                    const bool lsdown = (_lsc && !_lsc_old);                                    // limit switch close
                    const bool openReached  = (m_mode<_manual_pulse_open) && 
                                                (m_task!=_maxOpen) && (m_position > (m_task-m_delta)); // or open task reached
                    const bool closeReached = (m_mode<_manual_pulse_open) && 
                                                (m_task!=_minOpen) && (m_position < (m_task+m_delta)); // or close task reached
                    const bool stopOp = ( m_motion%10==_open   && ( lsup   || openReached  ) ); // флаг останова открытия
                    const bool stopCl = ( m_motion%10==_close  && ( lsdown || closeReached ) ); // флаг останова закрытия                    
                    const bool fstop  = ( m_twait.isDone() || stopOp || stopCl );               // флаг останова
                    
                    if( rc2 && (fstop || !rc0) ) {
                        //m_ppos->cleartask();
                        if(m_motion>=_open) m_pcmdopen->settask( 0 );
                        if(m_motion%10==_close) m_pcmdclose->settask( 0 );
                        m_twait.stop();
                            
                        if( !rc0 || (m_twait.isDone() && m_mode<_manual_pulse_open ) ) m_cmdstatus = _cmd_failed;
                        else 
                            m_cmdstatus = _cmd_completed;   
                    }
                }
                if( m_ppos->getname()=="FV11" && 0) {
                    cout<<dec<<' '<<m_ppos->getname()<<" mode="<<m_mode<<" ucmd="<<m_cmd<<" ucmdst="<<m_cmdstatus \
                        <<" mot= "<<m_motion<<" cmd= "<<_cmd_val<<"|"<<_dir_val<<" cnt= "<<_cnt_old<<"|"<<_cnt \
                        <<" pv="<<m_position<<" task= "<<m_task<<"|"<<m_delta \
                        <<" lso= "<<_lso_old<<"|"<<_lso<<" lsc= "<<_lsc_old<<"|"<<_lsc<<" I="<<m_pblock->getvalue()\
                        <<" fcQ="<<rc0<<" cvQ="<<rc2<<" pulseW="<<m_pulsew<<" scaleVLV="<<_minOpen<<"|"<<_maxOpen\
                        <<" t= "<<m_twait.getTT()<<" of "<<m_twait.getPreset()<<endl;                        
                }
            }
            break;
            case _pump_station:
                nqual = m_prun->getquality();
                if( nqual!=OPC_QUALITY_GOOD ) break;
                if( (m_mode==_auto_pid ) && m_pid.out!=m_task ) settask(m_pid.out, true);   
                if( m_status == _p_running ) {
                    if( int(m_pstart->gettask(true)) ) {
                        m_pstart->settask( 0 );
                    }
                }
                else {
                    if( !m_prun->getvalue() && int(m_pstop->gettask(true)) ) {
                        m_pstop->settask( 0 );
                    }
                }
                if( m_status >= _p_fault_2 && !(int(m_pstop->gettask(true))) ) {
                    m_pstop->settask( 1 );
                    m_pstart->settask( 0 );
                }
                if( (m_status == _p_ready || m_status == _p_idle || m_status == _p_running) && m_task_go ) {
                    m_task = min( m_maxAuto, m_task );                                      // загоним в границы
                    m_task = max( m_minAuto, m_task );
//                    cout<<"UEE command ";
                    if( fabs(m_task) > fabs(m_minAuto) ) {
//                        cout<<" start ";
                        if( (int(m_pstop->gettask(true))) ) m_pstop->settask( 0 );
                        if( !m_prun->getvalue() && !(int(m_pstart->gettask(true))) ) m_pstart->settask( 1 );
                    }
                    else {
//                        cout<<" stop ";
                        if( !(int(m_pstop->gettask(true))) ) m_pstop->settask( 1 );
                        if( (int(m_pstart->gettask(true))) ) m_pstart->settask( 0 );
                    }
//                    cout<<" min="<<m_minAuto<<" max="<<m_maxAuto<<" task="<<m_task<<endl;
                    m_ptask->settask( m_task );
                    m_task_go = false;
                }
            break;
        }
    }
    return rc;
}

int16_t cunit::changemode(int16_t nv) {
    int16_t rc = _exOK;
    
    if( m_nType==_valve ) {    
        if( nv>=_not_proc && nv<=_manual_pulse_close && stop()==_exOK ) {
            if( nv!=m_mode ) {
                m_mode_old = m_mode;
                m_pmod->setoldvalue(m_mode);
            }
            m_pmod->setvalue(nv);
            m_mode = nv;
     //       cout<<" changemode "<<m_mode_old<<"|"<<m_pmod->getoldvalue()<<"|"<<m_mode<<endl;
            m_twait.reset();
        }
        else rc=_exBadIO;
    }
    
    if( m_nType==_pump_station ) {
        if( nv>=_not_proc && nv<=_auto_pid ) {
            if( nv==_not_proc && nv!=m_mode ) {
                m_pstop->settask( 1 );
                m_pstart->settask( 0 );
            }            
            if( nv!=m_mode ) {
                m_mode_old = m_mode;
                m_pmod->setoldvalue(m_mode);
            }
            m_pmod->setvalue(nv);
            m_mode = nv;
     //       cout<<" changemode "<<m_mode_old<<"|"<<m_pmod->getoldvalue()<<"|"<<m_mode<<endl;
            m_twait.reset();
        }
    }   
    return rc;
}

int16_t cunit::revertmode() {
 //   cout<<" revertmode to "<< m_mode_old<<endl;
    return changemode(m_mode_old);
}

void cunit::open() { 
    m_task = m_ppos->getmaxeng();
    m_ppos->settask( m_task, true );
}

void cunit::close() { 
    m_task = m_ppos->getmineng(); 
    m_ppos->settask( m_task, true );
}

void cunit::settask(double t, bool f=true) { 
    double task;
    
    task = min( t,    m_ppos->getmaxeng() );
    task = max( task, m_ppos->getmineng() );
    
    m_task = task; 
    m_ppos->settask(m_task, f);
}

