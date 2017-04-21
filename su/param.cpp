#include "main.h"
//#include "param.h"
//#include "mbxchg.h"

#include <fstream>	
#include <sstream>	
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <string>
#include <string.h>
#include <stdlib.h>

#define _param_prc_delay    10000
#define _ten_thou           10000

using namespace std;

pthread_mutex_t         mutex_pub   = PTHREAD_MUTEX_INITIALIZER;
//pthread_cond_t  data_ready  = PTHREAD_COND_INITIALIZER;
//pthread_cond_t  pub_ready   = PTHREAD_COND_INITIALIZER;

pthread_mutex_t         mutex_param;// = PTHREAD_MUTEX_INITIALIZER;
pthread_mutexattr_t     mutex_param_attr;

paramlist tags;
bool fParamThreadInitialized;

cparam::cparam() {
    setproperty( string("raw"),         double(0)      );
    setproperty( string("value"),       double(0)   );
    setproperty( string("quality"),     int32_t(0)  );
    setproperty( string("timestamp"),   string("")  );  
    setproperty( string("deadband"),    double(0)   );
    setproperty( string("sec"),         int32_t(0)  );
    setproperty( string("msec"),        int32_t(0)  );
    setproperty( string("mineng"),      double(0)   );   
    setproperty( string("name"),        string("")  );
    setproperty( string("simenable"),   int(0)      );    
    setproperty( string("simvalue"),    int(0)      );    
    
    m_task = 0;
    m_task_go = false;
    m_task_delta = 1;
    m_sub = -2;
    m_quality = OPC_QUALITY_WAITING_FOR_INITIAL_DATA;
    m_readOff = -1; 
    m_readbit = -1; 
    m_connErr = -1;
    m_writeOff= -1;
    m_deadband= 0;
    m_raw = 0;
    m_raw_old = 0;
    m_rvalue = 0;
    m_rvalue_old = 0;
    m_firstscan = true;
    m_name.assign("");
    m_motion    = 0;
}

void cparam::init() {
    string sOff; 
//    cmbxchg     *mb = (cmbxchg *)p_conn;

    if( getproperty("readdata", sOff)==0 && !sOff.empty() ) {
        if( isdigit(sOff[0]) ) {
            vector<string> vc;
            int16_t nOff;
            strsplit(sOff, '.', vc);
            nOff = atoi(vc[0].c_str());
            if(nOff<cmbxchg::m_maxReadData) { 
                m_readOff = nOff;  
                if( vc.size() > 1 && isdigit(vc[1][0]) ) m_readbit = atoi(vc[1].c_str());
            }
        }
        else if(sOff[0]=='E') {
            m_readOff = -2;
        }
    }
    if( getproperty("writedata", sOff)==0 && !sOff.empty() ) {
        if( isdigit(sOff[0]) ) m_writeOff = atoi(sOff.c_str());
        else if(sOff[0]=='E') m_writeOff = -2;
    }
    
//    string  she;
//    int32_t nhe;
    int16_t bt;
    int rc = \
    getproperty( "minraw",  m_minRaw   ) | \
    getproperty( "maxraw",  m_maxRaw   ) | \
    getproperty( "mineng",  m_minEng   ) | \
    getproperty( "maxeng",  m_maxEng   ) | \
    getproperty( "flttime", m_fltTime  ) | \
    getproperty( "isbool",  m_isBool   ) | \
    getproperty( "hihi",    m_hihi     ) | \
    getproperty( "hi",      m_hi       ) | \
    getproperty( "lolo",    m_lolo     ) | \
    getproperty( "lo",      m_lo       ) | \
    getproperty( "deadband",m_deadband ) | \
    getproperty( "name",    m_name     ) | \
    getproperty( "topic",   m_topic    );
//    m_isBool = (bt!=0);

    int16_t nPortErrOff=0, nErrOff=0;
    
    cout<<"param::init "<<m_name<<" rc="<<rc<<" bool? "<<m_isBool<<" deadband "<<m_deadband<< \
        " maxE "<<m_maxEng<<" minE "<<m_minEng<<" maxR "<<m_maxRaw<<" minR "<<m_minRaw<< \
        " hihi "<<m_hihi<<" hi "<<m_hi<<" lolo "<<m_lolo<<" lo "<<m_lo<<endl;
    
    if( (m_readOff >= 0 || m_writeOff >= 0) && /*mb->getproperty("commanderror", nPortErrOff) == EXIT_SUCCESS &&*/ \
                getproperty("ErrPtr", nErrOff) == EXIT_SUCCESS ) {     // read errors of read modbus operations
         if( (nErrOff + nPortErrOff) < cmbxchg::m_maxReadData ) m_connErr = nErrOff + nPortErrOff;
    }

    if( m_readOff == -2 ) {
        if( m_name.find("FV") == 0 ) {   // if parameter (flow valve %)  must be evaluate from other
            string  scnt, sop, scl, scmop, scmcl;
            int16_t numop, numcl;
            
            numop = atoi( m_name.substr(2, 2).c_str() );
            numcl = numop + 1;
           
            scnt  = "FC"+to_string(numop);
            sop   = "ZV"+to_string(numop);
            scl   = "ZV"+to_string(numcl);
            scmop = "CV"+to_string(numop);
            scmcl = "CV"+to_string(numcl);
            
            m_fc    = getparam( scnt.c_str()  );  // FC
            m_lso   = getparam( sop.c_str()   );  // ZV opened
            m_lsc   = getparam( scl.c_str()   );  // ZV closed
            m_cmdo  = getparam( scmop.c_str() );  // CV open cmd
            m_cmdc  = getparam( scmcl.c_str() );  // CV close cmd
        }
        else
        if( m_name.find("FT") == 0 ) {            // if parameter (flow) must be evaluate from other
            if(m_name.size()>2 && m_name[2]=='3') {
                m_ft1 = getparam( "FT11" );
                m_ft2 = getparam( "FT21" );
            }
            else {
                string  s1, s2, s3, s4, s5;
                int16_t num;
                
                num = atoi( m_name.substr(2, 2).c_str() );
               
                s1 = "DT"+to_string(num);
                s2 = "KV"+to_string(num);
                s3 = "LV"+to_string(num);
                s4 = "PT"+to_string(num);
                s5 = "PT31"/*+to_string(num)*/;
               
                m_dt      = getparam( s1.c_str() );  // density 
                m_kv      = getparam( s2.c_str() );  // Kv factor
                m_fv      = getparam( s3.c_str() );  // valve % opened
                m_pt1     = getparam( s4.c_str() );  // давление в пласте
                m_pt2     = getparam( s5.c_str() );  // давление у насоса
            }
        }
        else
        if( m_name.find("LV") == 0 ) {   // if parameter (valve position mm) must be evaluate from other
            string  s1;
            int16_t num;
            
            num = atoi( m_name.substr(2, 2).c_str() );
           
            s1 = "FV"+to_string(num);
           
            m_pos = getparam( s1.c_str() );  // valve position in percent 
        }
   }
}
/*
//
//  Расчет / задание положения клапана по показанию датчика Холла, конечным выключателям и типу команды
//  
int16_t cparam::rawValveValueEvaluate() {
    int16_t lso, lso_old, lsc, lsc_old, cmdo, cmdc, cnt, cnt_old;
//    int16_t qual;
    time_t  t;
    int16_t rc, rc1, rc2;
    rc = (m_fc->m_quality != OPC_QUALITY_GOOD);
    rc1 = ( ( m_lso->m_quality | m_lsc->m_quality ) != OPC_QUALITY_GOOD );
    rc2 = ( ( m_cmdo->m_quality | m_cmdc->m_quality ) != OPC_QUALITY_GOOD );
  
    m_quality = (rc||rc1||rc2) ? OPC_QUALITY_BAD : OPC_QUALITY_GOOD;
        
    if( rc || rc1 || rc2 ) {
        return EXIT_FAILURE; 
    }
    else rc = EXIT_SUCCESS; 

    lso     = m_lso->m_rvalue;
    lsc     = m_lsc->m_rvalue;
    lso_old = m_lso->m_rvalue_old;
    lsc_old = m_lsc->m_rvalue_old;
    m_lso->m_rvalue_old = lso;
    m_lsc->m_rvalue_old = lsc;
    cmdo    = m_cmdo->m_rvalue;    
    cmdc    = m_cmdc->m_rvalue;
    cnt     = int(m_fc->m_rvalue);;
    cnt_old = int(m_fc->m_rvalue_old);;

    if( 0 && m_name.substr(0,3)=="FV1") {   
        cout<<m_name<<" CNT qual="<<rc<<" DI qual="<<rc1<<" DO qual="<<rc2<<" lso="<<lso_old<<" | "<<lso \
        <<" lsc="<<lsc_old<<" | "<<lsc<<" cmdo="<<cmdo<<" cmdc="<<cmdc<<" motion="<<m_motion<<\
        " task_go="<<m_task_go<<" cnt="<<cnt_old<<" | "<<cnt; 
    }

    if( (cmdo ^ cmdc) != 0 ) {
        m_raw = m_raw_old + (cmdo-cmdc)*(cnt - cnt_old);
        m_task_go = false;
    }
    else {
        if(cnt>20000) {
            m_fc->settask( 0 );
            cnt = 0;
        }
        if( m_motion && !m_task_go ) {
            if( m_minRaw==-3 && (lso || lsc) && cnt>1000 ) { 
                m_minRaw = 0; m_maxRaw = cnt; 
                m_task_delta = m_maxRaw/10; 
                m_raw = lso?cnt:0;
                m_raw_old = m_raw;
                m_fc->settask( 0 );
            } 
            else if(m_minRaw >= 0) {
                if( m_motion==1 ) m_task_delta -= (m_task - m_raw);
                if( m_motion==2 ) m_task_delta += (m_task - m_raw);
            }
            m_motion = 0;
        }
        else
        if( m_motion==0 ) { 
            m_raw = m_raw_old;
        }
    }
    
    if( (cmdo && cmdc) || m_tasktimer.isDone() ) {
        m_cmdo->settask( 0 );
        m_cmdc->settask( 0 );
        m_tasktimer.reset();
    }

    if( 0 && m_name.substr(0,3)=="FV1") {
        cout<<dec<<" motion="<<m_motion<<" delta="<<m_task_delta<<" task="<<m_task<<" raw="<<m_raw<<\
            " minr="<<m_minRaw<<" maxr="<<m_maxRaw<<endl;
    }   

    if( m_task_go && !cmdo && !cmdc ) {
        bool fGO = false;

        if( m_minRaw==-1 ) {
            cout<<" valve "<< m_cmdc->m_name <<" cmd go "<<m_task<<" %"<<endl;

            if( lsc || lso ) {
                m_fc->settask( 0 );
                if(!cnt) { 
                    m_minRaw=-3;
                    if(lsc) {
                        m_cmdo->settask( 1 ); 
                        m_motion = 1;                    
                    }
                    else {
                        m_cmdc->settask( 1 ); 
                        m_motion = 2;                    
                    }
                    fGO = true;
                }
            }
            else {
                m_cmdc->settask( 1 ); 
                m_motion = 2;
                m_minRaw = -2;
                fGO = true;
            }
        } else if( m_minRaw>=0 ) {
            if( m_task - m_raw > m_task_delta ) { m_cmdo->settask( 1 ); fGO = true; m_motion = 1; }
            if( m_raw - m_task > m_task_delta ) { m_cmdc->settask( 1 ); fGO = true; m_motion = 2; }
        }

        if(fGO) m_tasktimer.start( 180000 ); 
    }

    if( lso && !lso_old ) {
        if( cmdo ) {
            m_cmdo->settask( 0 );
            m_tasktimer.reset();
        }
        m_raw = m_maxRaw; m_raw_old = m_maxRaw; 
        if( m_minRaw>=0 ) m_fc->settask( 0 );
    }

    if( lsc ) {
        if( !lsc_old ) {
            if( cmdc ) {
                m_cmdc->settask( 0 );
                m_tasktimer.reset();
            }
            m_raw = 0; m_raw_old = 0; 
            m_fc->settask( 0 );
        }
        if(!cnt) {
            if(m_minRaw==-2) {
                m_minRaw=-3; 
                m_cmdo->settask( 1 ); 
                m_motion = 1;
                m_task_go = 1;
            }
        }
    }

    if( cmdo && m_raw > (m_task-m_task_delta) && m_minRaw>=0 )  {
        m_cmdo->settask( 0 ); m_tasktimer.reset();
    }
    if( cmdc && m_raw < (m_task+m_task_delta) && m_minRaw>=0 ) {
        m_cmdc->settask( 0 ); m_tasktimer.reset();
    }
    m_fc->m_rvalue_old = cnt;
    
    return rc;
}
*/
//
//  Расчет / задание положения клапана по показанию датчика Холла, конечным выключателям и типу команды
//  управление импульсом
//  
int16_t cparam::rawValveValueEvaluate() {
    int16_t lso, lso_old, lsc, lsc_old, cmdo, cmdc, cnt, cnt_old, cmdo_old;
//    int16_t qual;
    time_t  t;
    int16_t rc, rc0, rc1, rc2;
    rc0 = (m_fc->m_quality != OPC_QUALITY_GOOD);
    rc1 = ( ( m_lso->m_quality | m_lsc->m_quality ) != OPC_QUALITY_GOOD );
    rc2 = ( ( m_cmdo->m_quality | m_cmdc->m_quality ) != OPC_QUALITY_GOOD );
  
    m_quality = (rc||rc1||rc2) ? OPC_QUALITY_BAD : OPC_QUALITY_GOOD;

    lso     = m_lso->m_rvalue;
    lsc     = m_lsc->m_rvalue;
    lso_old = m_lso->m_rvalue_old;
    lsc_old = m_lsc->m_rvalue_old;
    m_lso->m_rvalue_old = lso;
    m_lsc->m_rvalue_old = lsc;
    cmdo    = m_cmdo->m_rvalue;    
    cmdo_old= m_cmdo->m_rvalue_old;    
    cmdc    = m_cmdc->m_rvalue;
    cnt     = int(m_fc->m_rvalue);
    cnt_old = int(m_fc->m_rvalue_old);
//    bool cmdTimeDone = m_cmdo->m_tasktimer.isDone();

    if( 1 && m_name.substr(0,3)=="FV1") {   
        cout<<m_name<<" CNT qual="<<rc0<<" DI qual="<<rc1<<" DO qual="<<rc2<<" lso="<<lso_old<<"|"<<lso \
        <<" lsc="<<lsc_old<<"|"<<lsc<<" cmdo="<<cmdo_old<<"|"<<cmdo<<" cmdc="<<cmdc\
        <<" motion="<<m_motion_old<<"|"<<m_motion<<" task_D="<<m_task_delta<<" task_go="<<m_task_go\
        <<" cnt="<<cnt_old<<"|"<<cnt<<" scale="<<m_minRaw<<"|"<<m_maxRaw<<" task="<<m_task\
        <<" timer="<<m_tasktimer.getTT()<<" raw="<<m_raw_old<<"|"; 
    }

    rc = ( rc0 || rc1 || rc2 ); 
    if( rc ) {                                        // if errors stopping the valve moving
        rc = EXIT_FAILURE; 
    }
    else rc = EXIT_SUCCESS; 
   
    if(!m_motion) {                                                     // if valve standby
        if( !m_motion_old && !rc ) {                                    // && old state same
            if( cnt>20000 && cnt_old ) {
                m_fc->settask( 0 );
                cnt = 0;
            }
            if( m_task_go && (m_minRaw==-1 || m_minRaw==-2) && !cmdo ) {// if valve scale not calibrated
                if( lsc || lso ) {                                      // if limit one switch is ON
                    if(!cnt) { 
                        m_minRaw=-3;
                        m_motion = lsc ? 1 : 2;
                    }
                    else m_fc->settask( 0 );
                }
                else if( m_minRaw==-1 ) {                               // if valve in middle position
                    m_minRaw = -2;
                    m_motion = 2;
                }
            } 
            if( m_task_go && m_minRaw>=0 && !cmdo ) {                   // if valve scale calibrated
                if( m_task-m_raw > m_task_delta/10 ) {                     // cmd to open
                    if(!lsc || !cnt) m_motion = 1;                      // if closed wait for reset counter
                }
                else
                if( m_raw-m_task > m_task_delta/10 ) {                     // cmd to close
                    if(!lso || !cnt) m_motion = 2;                      // if opened wait for reset counter
                }
                else m_task_go = false;
            }
            if(m_motion) {
                m_tasktimer.start( 300000 );                            // start timer to check moving process
                if( m_motion==2 ) m_cmdc->settask( 1 );
            }
            m_fc->m_rvalue_old  = cnt;
        }
        else if( m_cmdo->m_tasktimer.isDone() || m_motion_old<10 ) {   // state is idle && old state is moving 
            m_cmdo->m_tasktimer.reset();
            m_raw = m_raw_old + ((m_motion_old==11)-(m_motion_old==12))*(cnt - cnt_old);                 
            if( lsc || lso ) m_fc->settask( 0 );                        // if limit one switch is ON
                
            if(lso) m_raw = m_maxRaw; 
            if(lsc) m_raw = 0;

            if( m_minRaw==-2 && lsc ) { m_task_go = true; }
            if( m_minRaw==-3 && (lso || lsc) && cnt>1000 ) { 
                m_minRaw = 0; m_maxRaw = cnt; 
                m_task_delta = m_maxRaw/100; 
                m_raw = lso?cnt:0;
            }
            else
            if( m_minRaw >= 0 && !m_tasktimer.isDone() && m_motion_old>10 ) {
                if( m_motion_old==11 ) m_task_delta -= (m_task - m_raw);
                if( m_motion_old==12 ) m_task_delta += (m_task - m_raw);
            }
            m_tasktimer.reset();
            m_motion_old = m_motion;
            m_fc->m_rvalue_old  = cnt;
        }          
    }
    else if( m_motion ) {                                               // if valve moving (motion != 0)
        m_raw = m_raw_old + ((m_motion==11)-(m_motion==12))*(cnt - cnt_old);
        m_fc->m_rvalue_old  = cnt;
        m_task_go = false;
        
        bool stopOp = ( m_motion%10==1 && ( (lso && !lso_old) || (m_minRaw >= 0 && m_raw > (m_task-m_task_delta)) ) ); 
        bool stopCl = ( m_motion%10==2 && ( (lsc && !lsc_old) || (m_minRaw >= 0 && m_raw < (m_task+m_task_delta)) ) );
        
        if( !rc2 && (m_tasktimer.isDone() || \
            (stopOp || stopCl) && !cmdo || rc0 ) ) {   // limit switch (open or close ) position reached
            m_motion_old = m_motion;                   // or task completed                     
            if(m_motion>10) m_cmdo->settaskpulse( 1 );
            if(m_motion%10==2) m_cmdc->settask( 0 );
            m_tasktimer.reset();
            m_motion = 0;
            if(rc0) m_motion_old = -1;
        }
        if( !m_motion_old && (m_motion==1 || m_tasktimer.getTT()>1000) ) {        
            m_cmdo->settaskpulse( 1 );
            m_motion_old = m_motion;
            m_motion += 10;
        }
    }
    if( cmdo && !cmdo_old) {
        m_cmdo->settask( 0 );
    }
    m_cmdo->m_rvalue_old= cmdo;

    if( 1 && m_name.substr(0,3)=="FV1" ) { 
        cout<<m_raw;
        cout<<" motion="<<m_motion<<endl;
    }
    return rc;
}

//
//  Расчет мгновенного расхода по положению клапана и перепаду давления
//  
int16_t cparam::flowEvaluate() {
    int16_t rc=EXIT_SUCCESS, rc1;

//    cout<<m_name;
    if( m_name[2]=='3' ) {
        m_raw = m_ft1->m_rvalue + m_ft2->m_rvalue;
        m_quality = m_ft1->m_quality | m_ft2->m_quality;
//        cout<<" | "<<m_ft1->m_raw<<" + "<<m_ft2->m_raw<<" = "<<m_raw<<" | "<<m_quality;
    } 
    else {
        rc = (m_pt1->m_quality != OPC_QUALITY_GOOD);
        rc1 = (m_fv->m_quality != OPC_QUALITY_GOOD);
      
        m_quality = (rc||rc1) ? OPC_QUALITY_BAD : OPC_QUALITY_GOOD;
            
        if( rc || rc1 ) {
            if(m_name.substr(0,3)=="FT1") { 
                double rsim_en, rsim_v;
                getproperty("simen", rsim_en);
                getproperty("simva", rsim_v);            
//                cout<<"flow ev "<<m_name<<"  PT qual="<<rc<<" LT qual="<<rc1; \
                cout<<" pt name "<<m_pt1->m_name<<" pt val "<<m_pt1->m_rvalue<<" pt val "<<m_pt1->m_rvalue \
                    <<" pt simen "<<rsim_en<<" pt simval "<<rsim_v<<endl; 
            }
            m_raw = 0;
            return EXIT_FAILURE; 
        }
        else rc = EXIT_SUCCESS; 

        double sq, r1=1, r11=4, ht1 = m_fv->m_rvalue-1, _tan = 0.0448210728500398; 
//        cout<<"fv\tpt1\tpt2\tkv\tdt\tsq\tflow\n";
//        cout<<m_fv->m_name<<"\t"<<m_pt1->m_name<<"\t"<<m_pt2->m_name<<"\t"<<m_kv->m_name<<"\t"<<m_dt->m_name<<endl;
//        cout<<m_fv->m_rvalue<<"\t"<<m_pt1->m_rvalue<<"\t"<<m_pt2->m_rvalue<<"\t"<<m_kv->m_rvalue<<"\t"<<m_dt->m_rvalue;
        

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
        
        m_raw = m_kv->m_rvalue*sq*1e-6*sqrt((m_pt1->m_rvalue-m_pt2->m_rvalue)*2*1e6/ \
               ((m_dt->m_rvalue > 100 ) ? m_dt->m_rvalue : 1000) )*86400;
        
//        cout<<"\t"<<sq<<"\t"<<m_raw<<endl;
    }
//    cout<<endl;
    return rc;
}

int16_t cparam::getraw() {
    int16_t     rc  = EXIT_FAILURE;
//    cmbxchg     *mb = (cmbxchg *)p_conn;

    m_raw_old = m_raw;
    setproperty( "raw_old", m_raw_old );
//if(m_name.substr(0,3)=="FC1") cout<<" getraw off="<<m_readOff<<" bit="<<m_readbit<<" rawval="<<uppercase<<hex;
    
    m_quality_old = m_quality;

    if( m_readOff >= 0 ) {
        m_raw = cmbxchg::m_pReadData[m_readOff];
//        cout<<m_raw<<" ";
        if( m_readbit >= 0 ) {
            m_raw = (( int(m_raw) & ( 1 << m_readbit ) ) != 0);
            m_trigger = (( cmbxchg::m_pReadTrigger[m_readOff] & ( 1 << m_readbit ) ) != 0);
        }
//if(m_name.substr(0,3)=="FC1") cout<<m_raw<<" "<<dec<<endl;
        setproperty("raw", m_raw);
        if ( m_connErr >= 0 ) {
            m_quality = (cmbxchg::m_pReadData[m_connErr])?OPC_QUALITY_NOT_CONNECTED:OPC_QUALITY_GOOD;
        }
        rc=EXIT_SUCCESS;
    }
    else if( m_readOff == -2 ) {
        if(m_name.find("FV") == 0 ) {
            rc =rawValveValueEvaluate();
            setproperty("raw", m_raw);    
//            rc=EXIT_SUCCESS;
        }
        else
        if(m_name.find("LV") == 0 ) {
            m_raw = m_pos->m_raw;
            m_minRaw = m_pos->m_minRaw;
            m_maxRaw = m_pos->m_maxRaw;
            m_quality = m_pos->m_quality;
            setproperty("raw", m_raw); 
            rc=EXIT_SUCCESS;
        }
        else
        if(m_name.find("FT") == 0 ) {
            rc =flowEvaluate();
            setproperty("raw", m_raw);    
//            rc=EXIT_SUCCESS;
        }
    }
//    cout<<dec<<endl;
    return rc;
}

//
//  обработка параметров аналогового типа
//  приведение к инж. единицам, фильтрация, аналог==дискрет
//  анализ изменения значения сравнением с зоной нечувствительности
//
int16_t cparam::getvalue(double &rOut) {
    int16_t     nVal;
    double      rVal;
    timespec    tv;
    int32_t     nTime;
    int64_t     nctt;
    int64_t     nD;
    int64_t     nodt;             // time on previous step
    int16_t     rc=EXIT_FAILURE;
    double      rsim_en = 0, rsim_v;
//    cmbxchg     *mb = (cmbxchg *)p_conn;

    if(m_firstscan) {
        init();
    }
    clock_gettime(CLOCK_MONOTONIC,&tv);
    tv.tv_sec += dT;
    nctt = tv.tv_sec*_million + tv.tv_nsec/1000;
    nodt = m_ts.tv_sec*_million + m_ts.tv_nsec/1000;
    nD = abs(nctt-nodt);
   
    if( (rc = getproperty("simen", rsim_en) | \
            getproperty("simva", rsim_v)) == EXIT_SUCCESS && rsim_en != 0 ) {   // simulation mode switched ON 
        m_quality_old = m_quality;
        rVal  = rsim_v;
        rOut  = rsim_v;
        m_quality = OPC_QUALITY_GOOD;
        rc = EXIT_SUCCESS;
    }
    else { 
        rc = getraw();
    }

    if( rc==EXIT_SUCCESS ) {
        if( rsim_en == 0 ) {                                                    // simulation mode switched OFF
            if( m_maxRaw!=m_minRaw && m_maxEng!=m_minEng ) {
                rVal = (m_maxEng-m_minEng)/(m_maxRaw-m_minRaw)*(m_raw-m_minRaw)+m_minEng;
                nTime = m_fltTime*1000;
                if(rVal>m_maxEng) rVal = m_maxEng;
                if(rVal<m_minEng) rVal = m_minEng;               
                if( nD && nTime ) rVal = (m_rvalue*nTime+rVal*nD)/(nTime+nD); 
                if(m_isBool==1) rVal = (rVal >= m_hihi);                        // if it is a discret parameter
                if(m_isBool==2) rVal = (rVal < m_hihi);                         // if it is a discret parameter & inverse
                rOut = rVal;                                                    // current value
            }
        }
//      if( m_name.substr(0,4)=="FT11") \
            cout <<"getvalue name="<<m_name<<" oldT "<< nodt << " | curT " << nctt << " | dT " << nD \
                <<" |v "<<dec<<rVal<<" |vOld "<<m_rvalue<<" |vOldOld "<<m_rvalue_old \
                <<" |raw "<<m_raw<<" |d "<<m_deadband<<" maxE "<<m_maxEng<<" minE "<<m_minEng \
                <<" maxR "<<m_maxRaw<<" minR "<<m_minRaw<<" hihi "<<m_hihi<<" | mConnErrOff "<<m_connErr \
                <<" |qOld "<<int(m_quality_old)<<" |q "<<int(m_quality)<<endl;

//      save value if it (or quality) was changes
        if( fabs(rVal-m_rvalue)>=m_deadband || m_quality_old != m_quality || nD>60*_million) {
            m_ts.tv_sec = tv.tv_sec;
            m_ts.tv_nsec = tv.tv_nsec;
            m_valueupdated = true;
            setproperty("value", rVal);
            setproperty("quality", m_quality);
            setproperty("sec",  int32_t(tv.tv_sec));
            setproperty("msec", int32_t(tv.tv_nsec/_million));
//          if( m_name.substr(0,3)=="FV1") cout<<endl;
//                cout <<" |v "<< rVal<<" |vold "<<m_rvalue<< \
                " |d "<<m_deadband<<" | mConnErrOff "<<m_connErr<<" |q "<<int(nQual)<<" |dt "<<nD/_million<<endl;
            m_rvalue = rVal;
        } 
//        if(m_firstscan) { m_rvalue_old = m_rvalue; m_firstscan = false; }
    }    
    if(m_firstscan) { m_rvalue_old = m_rvalue; m_firstscan = false; }
   
    return rc;
}

int16_t cparam::setvalue() {
    int16_t rc = EXIT_FAILURE;
//    cmbxchg *mb;
    string  sOff;
    int16_t nOff;

    if( m_writeOff >= 0 ) {
//      if( m_name.substr(0,3)=="FC1")
//            cout<<"setvalue "<<m_name<<" task "<<m_task<<" off="<<m_writeOff<<" conn="<<int(mb)<<endl;      
        cmbxchg::m_pWriteData[m_writeOff] = m_task;
        cmbxchg::m_pLastWriteData[m_writeOff] = m_task-1;
        m_task_go = false; 
        rc = EXIT_SUCCESS;
    }
    else if( m_writeOff == -2 ) {
    }
    
    return rc;
}

//  
//	Чтение и парсинг конфигурационного файла
//
int16_t readCfg() {
	int16_t     rc = EXIT_FAILURE;
    int16_t     nI=0, i, j;
    cmbxchg     *mb = NULL;  
    upcon       *up = NULL;;    
    
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file("map.xml");    
    if(result) {                    // если формат файла корректен
        // парсим модбас порты и команды
        pugi::xpath_node_set tools = doc.select_nodes("//port[@name='modbusport']");
        for(pugi::xpath_node_set::const_iterator it = tools.begin(); it != tools.end(); ++it) {
            mb = new cmbxchg();
            conn.push_back(mb);
            mb->m_id = atoi(it->node().attribute("num").value());
            
            cout<<"port num="<<mb->m_id<<endl;

            for(pugi::xml_node tool = it->node().first_child(); tool; tool = tool.next_sibling()) {        
                if(string(tool.name())=="commands") {
                    for(pugi::xml_node cmd = tool.first_child(); cmd; cmd = cmd.next_sibling()) {   
                        std::vector<int16_t> result;
                        result.clear();
                        for(pugi::xml_attribute attr = cmd.first_attribute(); attr; attr = attr.next_attribute()) {
                            result.push_back(atoi(attr.value()));
                        }
                        ccmd cmd(result);
//                      cout<<"parse cmds count = "<<result.size()<<endl;
                        mb->mbCommandAdd(cmd);
                    }
                }
                else {
                    mb->setproperty( tool.name(), tool.text().get() );
                }
            }
        }
        string spar, sval;
        // парсим тэги
        tools = doc.select_nodes("//tags/tag");
        for(pugi::xpath_node_set::const_iterator it = tools.begin(); it != tools.end(); ++it) {
            cparam  p;
            string  s = it->node().text().get();
            cout<<"Tag "<<s;
            for (pugi::xml_attribute attr = it->node().first_attribute(); attr; attr = attr.next_attribute()) {
                spar = attr.name();
                sval = attr.value();
                p.setproperty( spar, sval );
                cout<<" "<<spar<<"="<<sval;
            } 
            cout<<endl;
            p.setproperty("name", s);
            /*if(p.getproperty("name", s)==EXIT_SUCCESS)*/ tags.push_back(make_pair(s, p));
        }

        // парсим соединения наверх
        tools = doc.select_nodes("//uplinks/up");
        for(pugi::xpath_node_set::const_iterator it = tools.begin(); it != tools.end(); ++it) {
            up = new upcon();
            upc.push_back(up);
            up->m_id = atoi(it->node().attribute("num").value());   
            
            cout<<"parse upcon num="<<up->m_id<<endl;
            for(pugi::xml_node tool = it->node().first_child(); tool; tool = tool.next_sibling()) {        
                up->setproperty( tool.name(), tool.text().get() );
                cout<<" "<<tool.name()<<"="<<tool.text().get();   
            }
            cout<<endl;   
        }
        
        // парсим описания дисплеев
        tools = doc.select_nodes("//displays/display[@num]");
        int16_t ndisp=0;
        cout<<"parse disp="<<ndisp<<" size="<<tools.size()<<endl;
        for(pugi::xpath_node_set::const_iterator it = tools.begin(); it != tools.end(); ++it) { 
            for(pugi::xml_attribute attr = it->node().first_attribute(); attr; attr = attr.next_attribute()) {
                spar = attr.name();
                sval = attr.value();
                cout<<" "<<spar<<"="<<sval;
                dsp.setproperty( ndisp, spar, sval );
            } 
            cout<<endl;
            pugi::xml_node nod = it->node().child("lines");
            for(pugi::xml_node tool = nod.first_child(); tool; tool = tool.next_sibling()) {        
               int16_t nrow = atoi(tool.text().get())-1;
               cout<<"row="<<nrow;
               for(pugi::xml_attribute attr = tool.first_attribute(); attr; attr = attr.next_attribute()) {
                    spar = attr.name();
                    sval = attr.value();
                    if(nrow>=0) {
                        dsp.definedspline( ndisp, nrow, spar.c_str(), sval );
                        cout<<" "<<spar<<"="<<sval;   
                    }
                }               
                cout<<endl;
           }   
            ndisp++;
        }
        
        rc = EXIT_SUCCESS;
    }
    else {
        cout << "Cfg load error: " << result.description() << endl;
    }
    
    return rc;
}

//
// ask value parameter by it name
//
int16_t getparam( const char* na, double& va, int16_t& qual, timespec* ts, int16_t trigger=0 ) {
    int16_t     rc=EXIT_FAILURE;

    pthread_mutex_lock( &mutex_param );
    cparam* pp = getparam( na );
    if( pp!=NULL ) {
        qual= pp->getquality();
        va = (!trigger) ? pp->getvalue() : pp->gettrigger();
        ts  = pp->getTS();
        rc=EXIT_SUCCESS;
    }
    pthread_mutex_unlock( &mutex_param );

    return rc;
}

//
// ask value parameter by it name
//
int16_t getparam(const char* na, std::string& va) {
    int16_t     rc=EXIT_FAILURE;
    
    pthread_mutex_lock( &mutex_param );
    cparam* pp = getparam( na );
    if( pp!=NULL ) {
        va = to_string( pp->getvalue() );
        if( pp->getquality() != OPC_QUALITY_GOOD ) va += " bad";
        rc=EXIT_SUCCESS;
    }
    pthread_mutex_unlock( &mutex_param );

    return rc;
}

//
// ask parameter addr by it name
//
cparam* getparam( const char*  na ) {
    cparam*     rc=NULL;

    pthread_mutex_lock( &mutex_param );
    paramlist::iterator ifi = find_if( tags.begin(), tags.end(), compareP<cparam>(na) );
    if( ifi != tags.end() ) {
        rc = &(ifi->second);
    }
    pthread_mutex_unlock( &mutex_param );
    if(!rc) {
        cout<<"getparam() не нашел параметр "<<na<<". Завершаю работу..."<<endl;
        exit(0);
    }
    return rc;
}

//
// возврат разницы между новым значением параметра и предыдущим считанным 
//
int16_t getparamcount( const char* na, int16_t& val ) {
    int16_t     rc=EXIT_FAILURE;
    double      rvc, rvo;

    pthread_mutex_lock( &mutex_param );
    cparam* pp = getparam( na );
    if( pp!=NULL && pp->getquality()==OPC_QUALITY_GOOD ) {
        rvc =  pp->getvalue();
        rvo =  pp->getoldvalue();
        pp->setoldvalue(rvc);
        val = rvc-rvo;
        rc=EXIT_SUCCESS;
    }
    pthread_mutex_unlock( &mutex_param );

    return rc;
}

int16_t getparamlimits( const char* na, double& emin, double& emax) {
    int16_t     rc=EXIT_FAILURE;
    double      rvc, rvo;

    pthread_mutex_lock( &mutex_param );
    cparam* pp = getparam( na );
    if( pp!=NULL && pp->getquality()==OPC_QUALITY_GOOD ) {
        pp->getlimits( emin, emax );
        rc=EXIT_SUCCESS;
    }
    pthread_mutex_unlock( &mutex_param );

    return rc;
}

/*
//
// task for writing value on parameter name
//
int16_t taskparam( std::string& na, std::string& fi, std::string& va ) {
    int16_t rc=EXIT_FAILURE;
    double  rval;
    string  val(va);
    
    reduce(val, (char *)" \t\n");
    cout<<"taskparam "<<na<<" field "<<fi<<" value "<<val<<endl;
    if( (val.length() && isdigit(val[0])) || (val.length()>1 && val[0]=='-' && isdigit(val[1])) ) {
        pthread_mutex_lock( &mutex_param );
        paramlist::iterator ifi = find_if( tags.begin(), tags.end(), compareP<cparam>(na) );
        if( ifi != tags.end() ) {
            std::istringstream(val) >> rval;
            if( fi.find("task") != std::string::npos ) ifi->second.settask( rval );
            else ifi->second.setproperty(fi, rval);
            rc=EXIT_SUCCESS;
        }
        pthread_mutex_unlock( &mutex_param );
    }
    return rc;
}
*/
//
// task for writing value on parameter full name 
//
int16_t taskparam( std::string& na, std::string& va ) {
    int16_t rc=EXIT_FAILURE;
    double  rval;
    string  val(va);
    
    reduce(val, (char *)" \t\n");
    cout<<"taskparam "<<na<<" value "<<val;
            
    std::vector<string> vc;
    std::string stag, sf;
    strsplit( na, '/', vc);
    if( (val.length() && isdigit(val[0])) || (val.length()>1 && val[0]=='-' && isdigit(val[1]))  
            && vc.size() > 3 ) {
        sf   = vc.back(); vc.pop_back();
        stag = vc.back(); vc.pop_back();
        cout<<" name "<<stag<<" field "<<sf<< " value "<<val<<" size "<<vc.size()<<endl;
            
//        pthread_mutex_lock( &mutex_param );
        paramlist::iterator ifi = find_if( tags.begin(), tags.end(), compareP<cparam>(stag) );
        if( ifi != tags.end() ) {
            std::istringstream(val) >> rval;
            if( sf.find("task") != std::string::npos ) ifi->second.settask( rval );
            else ifi->second.setproperty(sf, rval);
            rc=EXIT_SUCCESS;
        }
//        pthread_mutex_unlock( &mutex_param );
    }
    return rc;
}

//
// поток обработки параметров 
//
void* paramProcessing(void *args) {
    paramlist::iterator ih, iend;
    int16_t nRes, nRes1, nVal;
    uint8_t nQ;
    double  rVal;
    int32_t nCnt=0;

    cout << "start parameters processing " << args << endl;
    sleep(1);
    while ( fParamThreadInitialized ) {
        pthread_mutex_lock( &mutex_param );
        
//        pthread_cond_wait( &data_ready, &mutex_param );// start processing after data receive     
        ih   = tags.begin();
        iend = tags.end();
        while ( ih != iend ) {
            string  sOff;
            int16_t nu;
            cparam  &pp = ih->second;
            
            nRes1= pp.getvalue( rVal );
           
            if( pp.hasnewvalue() && (nu=pp.getpubcon())>=0 && nu<upc.size() ) {
                publish(pp);                        
            }

            if( pp.taskset() ) pp.setvalue();
            if( pp.gettask() > 0 && pp.m_tasktimer.isDone() ) {
                pp.settask(0);
//                    pp.m_tasktimer.reset();
            }
                
            if( pp.m_sub==-2 ) {
                if( (nu=pp.getsubcon())>=0 && nu<upc.size() ) {
                    upc[nu]->subscribe(pp);
                    pp.m_sub = nu;
                }
                else pp.m_sub = -1;
            }

            ++ih;
        }
        
        pthread_mutex_unlock( &mutex_param );
        usleep(_param_prc_delay);
    }     
    
    cout << "end parameters processing" << endl;
    
    return EXIT_SUCCESS;
}


