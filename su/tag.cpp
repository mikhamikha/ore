
#include <fstream>	
#include <sstream>	
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <string>
#include <string.h>
#include <stdlib.h>

#include "tagdirector.h"
#include "mbxchg.h"
#include "upcon.h"
#include "unit.h"
#include "hist.h"
#include "const.h"

using namespace std;

pthread_mutex_t         mutex_pub   = PTHREAD_MUTEX_INITIALIZER;
//pthread_cond_t  data_ready  = PTHREAD_COND_INITIALIZER;
//pthread_cond_t  pub_ready   = PTHREAD_COND_INITIALIZER;

pthread_mutex_t         mutex_tag;// = PTHREAD_MUTEX_INITIALIZER;
pthread_mutexattr_t     mutex_tag_attr;

bool ftagThreadInitialized;

ctag::ctag() {

    setproperty( string("raw"),         g_fZero );
    setproperty( string("value"),       g_fZero );
    setproperty( string("task"),        g_fZero );
    setproperty( string("quality"),     g_nZero );
    setproperty( string("timestamp"),   g_sZero );  
    setproperty( string("dead"),        g_fZero );
    setproperty( string("sec"),         g_nZero );
    setproperty( string("msec"),        g_nZero );
    setproperty( string("mineng"),      g_fZero );   
    setproperty( string("name"),        g_sZero );
    setproperty( string("simenable"),   g_nZero );    
    setproperty( string("simvalue"),    g_fZero );    
    setproperty( string("mindev"),      g_fZero );   
    setproperty( string("maxdev"),      g_fZero );   
    
    m_task = 0;
    m_task_go = false;
    m_task_delta = 1;
//    m_sub = -2;
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
    m_minDev = 0;
    m_maxDev = 0;
    m_ppub = 0l;
    m_psub = 0l;
    m_subscribed = false;   
//    m_motion    = 0;
}

void ctag::init() {
    string sOff; 
//    cmbxchg     *mb = (cmbxchg *)p_conn;

    if( getproperty("rAddr", sOff)==0 ) {
        if( !sOff.empty() && isdigit(sOff[0]) ) {
            vector<string> vc;
            int16_t nOff;
            strsplit(sOff, '.', vc);
            nOff = atoi(vc[0].c_str());
            if(nOff<cmbxchg::m_maxReadData) { 
                m_readOff = nOff;  
                if( vc.size() > 1 && isdigit(vc[1][0]) ) m_readbit = atoi(vc[1].c_str());
            }
        }
        else /*if(sOff[0]=='E')*/ {
            m_readOff = -2;
        }
    }
    if( getproperty("wAddr", sOff)==0 ) {
        if( !sOff.empty() && isdigit(sOff[0]) ) m_writeOff = atoi(sOff.c_str());
        else /*if(sOff[0]=='E')*/ m_writeOff = -2;
    }
    
//    string  she;
//    int32_t nhe;
    int rc = \
    getproperty( "minr",  m_minRaw   ) | \
    getproperty( "maxr",  m_maxRaw   ) | \
    getproperty( "mine",  m_minEng   ) | \
    getproperty( "maxe",  m_maxEng   ) | \
    getproperty( "flttime", m_fltTime  ) | \
    getproperty( "type",    m_type   ) | \
    getproperty( "hihi",    m_hihi     ) | \
    getproperty( "hi",      m_hi       ) | \
    getproperty( "lolo",    m_lolo     ) | \
    getproperty( "lo",      m_lo       ) | \
    getproperty( "dead",    m_deadband ) | \
    getproperty( "name",    m_name     ) | \
    getproperty( "topic",   m_topic    ) | \
    getproperty( "mindev",  m_minDev   ) | \
    getproperty( "maxdev",  m_maxDev   );

    int16_t nPortErrOff=0, nErrOff=0;
    
    if( (m_readOff >= 0 || m_writeOff >= 0) && /*mb->getproperty("commanderror", nPortErrOff) == EXIT_SUCCESS &&*/ \
                getproperty("ErrPtr", nErrOff) == _exOK ) {     // read errors of read modbus operations
         if( (nErrOff + nPortErrOff) < cmbxchg::m_maxReadData ) m_connErr = nErrOff + nPortErrOff;
    }

    if( m_readOff == -2 ) {
        m_quality = OPC_QUALITY_GOOD;

        if( m_name.find("LV") == 0 ) {              // if tag (valve position mm) must be evaluate from other
            string  s1;
            int16_t num;
            
            num = atoi( m_name.substr(2, 2).c_str() );
            s1 = "FV"+to_string(num);
            m_pos = getaddr( s1 );                  // valve position in percent
//            cout<<"init "<<m_name<<" pos="<<hex<<long(m_pos)<<dec<<endl;
        }
    }
    double rval;
    if( getproperty("default", rval)==_exOK ) { setvalue(rval); } 

    // подписка на команды
    int16_t nu;
    // получим адрес соединения для подписки на команды
    if( (getproperty("sub", nu))==_exOK && nu && nu<=int(upc.size()) ) {
        m_psub = upc[nu-1];
    }
    // получим адрес соединения для публикации данных
    if( (getproperty("pub", nu))==_exOK && nu && nu<=int(upc.size()) ) {
        m_ppub = upc[nu-1];
    }
    cout<<"tag::init "<<m_name<<" rc="<<rc<<" bool? "<<m_type<<" deadband "<<m_deadband
        <<" maxE "<<m_maxEng<<" minE "<<m_minEng<<" maxR "<<m_maxRaw<<" minR "<<m_minRaw
        <<" hihi "<<m_hihi<<" hi "<<m_hi<<" lolo "<<m_lolo<<" lo "<<m_lo<<" pos="<<hex<<long(m_pos)
        <<" sub="<<m_psub<<" pub="<<m_ppub<<dec<<endl;
}

int16_t ctag::getraw() {
    int16_t     rc  = _exOK;
//    cmbxchg     *mb = (cmbxchg *)p_conn;

    m_raw_old = m_raw;
    setproperty( "raw_old", m_raw_old );
    //if(m_name.substr(0,3)=="FC1") cout<<" getraw off="<<m_readOff<<" bit="<<m_readbit<<" rawval="<<uppercase<<hex;
    
    m_quality_old = m_quality;

    if( m_readOff >= 0 ) {
        int16_t _tmp16[2];
        int32_t _tmp32;
            switch( m_type ) {
            case _real_i32:
//                    if(m_name.substr(0,4)=="PT11") 
//                        cout<<m_name<<" "<<hex<<cmbxchg::m_pReadData[m_readOff]<<"|"<<cmbxchg::m_pReadData[m_readOff+1]<<dec<<endl;
                    _tmp16[1] = cmbxchg::m_pReadData[m_readOff];
                    _tmp16[0] = cmbxchg::m_pReadData[m_readOff+1];
                    memcpy( ((char *)&_tmp32), ((char *)&_tmp16[0]), sizeof(int32_t));
                    m_raw = _tmp32;
//                    if(m_name.substr(0,4)=="PT11") 
//                        cout<<" getraw off="<<m_readOff<<" src="<<hex<<*((int32_t*)(cmbxchg::m_pReadData+m_readOff))
//                            <<" tmp="<<_tmp32<<"|"<<dec<<_tmp32<<" raw="<<m_raw
//                            <<" sizeof="<<sizeof(_tmp32)<<endl;
                break;
           case _real_ui32: 
                    _tmp16[1] = cmbxchg::m_pReadData[m_readOff];
                    _tmp16[0] = cmbxchg::m_pReadData[m_readOff+1];
                    memcpy( ((char *)&_tmp32), ((char *)&_tmp16[0]), sizeof(int32_t));
                    m_raw = (uint32_t)_tmp32;
                break;
           case _real_wt_foton:
                    m_raw = m_maxRaw-cmbxchg::m_pReadData[m_readOff];
                    break;
           default: 
                m_raw = cmbxchg::m_pReadData[m_readOff];
        }
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
//        rc=EXIT_SUCCESS;
    }
    else 
    if( m_readOff == -2 ) {
        if(m_name.find("LV") == 0 ) {
            setrawval( m_pos->getvalue() );
            setrawscale( m_pos->m_minEng, m_pos->m_maxEng );
            m_quality = m_pos->m_quality;
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
int16_t ctag::evaluate() {                         
    double      rVal = m_rvalue;
    timespec    tv;
    int32_t     nTime;
    int64_t     nctt;
    int64_t     nD;
    int64_t     nodt;             // time on previous step
    int16_t     rc=_exFail;
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

    if( (rc = getproperty("simenable", rsim_en) | \
            getproperty("simvalue", rsim_v)) == _exOK && rsim_en != 0 ) {   // simulation mode switched ON 
        m_quality_old = m_quality;
        rVal  = rsim_v;
//        rOut  = rsim_v;
        m_quality = OPC_QUALITY_GOOD;
    }
    else { 
//       if( m_name.find("FV") ==std::string::npos ) 
           rc = getraw();
    }

//    if( m_name.substr(0,4)=="IT01")  cout<<to_text()<<endl;

    if( rc==_exOK ) {
        if( rsim_en == 0 ) {                                                        // simulation mode switched OFF
            if( m_maxRaw!=m_minRaw && m_maxEng!=m_minEng ) {
                rVal = (m_maxEng-m_minEng)/(m_maxRaw-m_minRaw)*(m_raw-m_minRaw)+m_minEng;
                rVal = min( rVal, m_maxEng );                                       // ограничим значение инженерной шкалой
                rVal = max( rVal, m_minEng );
                
                if( m_minDev!=m_maxDev )  {                                         // если задана шкала параметра < шкалы прибора,
                    rVal = (m_maxEng-m_minEng)/(m_maxDev-m_minDev)*(rVal-m_minDev)+m_minEng;
                    rVal = min( rVal, m_maxEng );                                   // ограничим значение инженерной шкалой
                    rVal = max( rVal, m_minEng );
                }
                
                nTime = m_fltTime*1000;
                if( nD && nTime ) rVal = (m_rvalue*nTime+rVal*nD)/(nTime+nD); 
                if(m_type==1) rVal = (rVal >= m_hihi);                              // if it is a discret tag
                if(m_type==2) rVal = (rVal < m_hihi);                               // if it is a discret tag & inverse
//                rOut = rVal;                                                        // current value
            }
            else {
//                rOut = m_raw;
                rVal = m_raw;
            }
        }
      
        
        // подписка на команды
        if( m_psub ) {                              // если разрешено получать команды
            if( ((upcon*)m_psub)->connected() ) {   // если подключение есть и не подписаны, пробуем подписать тэг
                if( !subscribed() ) {
                    cout<<" attempt 2 sub "<<m_name<<endl;
                    subscribed( (((upcon*)m_psub)->subscribe( this )==_exOK) );          
                }
            }
            else {                                  // если подключения нет, снимаем флаг о подписке
                subscribed( false );
            }
        }
        
        // save value if it (or quality or timestamp) was changes
        if( fabs(rVal-m_rvalue)>=m_deadband || m_quality_old != m_quality || nD>60*_million ) {
            int32_t nsec = int32_t(tv.tv_sec);
            int32_t nmsec= int32_t(tv.tv_nsec/_million);
            m_ts.tv_sec = tv.tv_sec;
            m_ts.tv_nsec = tv.tv_nsec;
            setproperty("value", rVal);
            setproperty("quality", m_quality);
            setproperty("sec",  nsec);
            setproperty("msec", nmsec);
/*
            if( m_name.substr(0,3)=="ZV1") \
                cout <<"getvalue name="<<m_name<<" TS= "<< nodt << "|" << nctt << " dT= " << nD <<" readOff="<<m_readOff \
                    <<" val= "<<dec<<m_rvalue_old<<"|"<<m_rvalue<<"|"<<rVal \
                    <<" raw= "<<m_raw_old<<"|"<<m_raw<<" dead= "<<m_deadband<<" engSc= "<<m_minEng<<"|"<<m_maxEng \
                    <<" rawSc= "<<m_minRaw<<"|"<<m_maxRaw<<" hihi= "<<m_hihi<<" mConnErrOff= "<<m_connErr \
                    <<" q= "<<int(m_quality_old)<<"|"<<int(m_quality)
                    <<" pub="<<m_ppub<<" isConn="<<(m_ppub ? ((upcon*)m_ppub)->connected():0)<<endl;
*/
            m_rvalue = rVal;
            if( m_ppub && ((upcon*)m_ppub)->connected() ) ((upcon*)m_ppub)->valueChanged( *this );
        } 

        // выдача задания на модули вв
        // в случае, если было импульсное задание
        if( m_quality==OPC_QUALITY_GOOD ) {
            if( isbool() && m_tasktimer.isDone() ) {      // если было импульсное задание и вышел таймер
                settask( !gettask() );                    // инвертируем выход
                m_tasktimer.reset();
            }
        }

    }    
    if(m_firstscan) { m_rvalue_old = m_rvalue; m_firstscan = false; }

  
    return rc;
}

int16_t ctag::setvalue( double rin=0 ) {
    int16_t rc = _exFail;
//    cmbxchg *mb;
    string  sOff;
    
    if( m_writeOff == -2 ) {
        m_rvalue = rin;
        m_raw = rin;
        m_quality = OPC_QUALITY_GOOD;
        if( m_ppub && ((upcon*)m_ppub)->connected() ) ((upcon*)m_ppub)->valueChanged( *this );
        //settask( rin, false );
        setproperty( "task", rin );

        rc = _exOK;   
    }
    
    return rc;
}
// get task value; if fraw==true then unscaled task
double ctag::gettask( bool fraw ) {        
    double val;
    if( !fraw && m_maxRaw-m_minRaw!=0 && m_maxEng-m_minEng!=0 ) { 
        val = (m_maxEng-m_minEng)/(m_maxRaw-m_minRaw)*(m_task-m_minRaw)+m_minEng;
        val = min( val, m_maxEng );         // ограничим значение инженерной шкалой
        val = max( val, m_minEng );
        
        if( m_minDev!=m_maxDev )  {         // если задана шкала параметра < шкалы прибора,
            val = (m_maxEng-m_minEng)/(m_maxDev-m_minDev)*(val-m_minDev)+m_minEng;
            val = min( val, m_maxEng );     // ограничим значение инженерной шкалой
            val = max( val, m_minEng );
        }        
    }
    else val = m_task;
    return val;    
}

int16_t ctag::settask(double rin, bool fgo) {
    int16_t rc = _exOK;
    double  rVal = rin;
   
//    cout<<endl<<"settask "<<m_name<<" task=="<<rVal<<" go="<<fgo; 

    if( m_maxRaw-m_minRaw!=0 && m_maxEng-m_minEng!=0 ) {
        if( m_minDev!=m_maxDev )  {                                         // если задана шкала параметра < шкалы прибора,
            rVal = (m_maxDev-m_minDev)/(m_maxEng-m_minEng)*(rVal-m_minEng)+m_minDev;
            rVal = min( rVal, m_maxDev );                                   // ограничим значение инженерной шкалой
            rVal = max( rVal, m_minDev );
//            cout<<" dev="<<rVal;
        }        
        rVal = (m_maxRaw-m_minRaw)/(m_maxEng-m_minEng)*(rVal-m_minEng)+m_minRaw;
//        cout<<" raw="<<rVal;
    }
    m_task    = rVal;  
    if(fgo) m_task_go = fgo; 
    
    if( m_name.substr(0,4)=="PT11") \
            cout<<"settask "<<m_task<<endl; 
    
    if( m_writeOff >= 0 ) {                                     // если это modbus
/*      if( m_name.substr(0,3)=="FC1") \
            cout<<"setvalue "<<m_name<<" task "<<m_task<<" off="<<m_writeOff<<" conn="<<int(mb)<<endl;   
            */
        pthread_mutex_lock( &mutex_tag );
        cmbxchg::m_pWriteData[m_writeOff] = int(m_task)&USHRT_MAX;
        // изменим старое значение, чтобы "железно" записалось
        cmbxchg::m_pLastWriteData[m_writeOff] = cmbxchg::m_pWriteData[m_writeOff]-1;  
        pthread_mutex_unlock( &mutex_tag );
        m_task_go = false; 
    }
    setproperty( "task", rin );
    return rc;
}

int16_t ctag::settaskpulse(double rin, int32_t pre) {
    int16_t rc = settask( rin, true ); 
    m_task_go = true;    
    m_tasktimer.start(pre);
    return rc;
}

// 
// получить текстовое описание по значению
//
int16_t ctag::getdescvalue( string& name, string& sval )
{
    int16_t rc;
    int32_t ival;
    rc=getproperty( name, ival );
    if( !rc && m_name.substr(0,2)=="MV" && name=="task" && ival < int(vmodes.size()) ) 
        vmodes[ival].getproperty( "display", sval );
    if( !rc && m_name.substr(0,2)=="FV" && name=="status" && ival < int(vstatuses.size()) ) 
        vstatuses[ival].getproperty( "display", sval );  
    return rc;
}

double ctag::gettrigger()  {
//        cmbxchg     *mb = (cmbxchg *)p_conn;

    if( m_trigger && m_readOff >= 0 ) {
        if( m_readbit >= 0 ) {
            cmbxchg::m_pReadTrigger[m_readOff] &= (0xFFFF ^ ( 1 << m_readbit ));
        }   
    }
    return m_trigger; 
}
// 
// получить отметку времени в милисекундах
// 
int32_t ctag::getmsec() {
    timespec* tts = getTS();
    return  int32_t(tts->tv_sec*1000 + int(tts->tv_nsec/_million));
}

//
// Получить значение
//
double getval(ctag* p) {
    double rval=-1;
    if( p ) rval = p->getvalue();

    return rval;
}
//
// Получить качество тэга
//
uint8_t getqual(ctag* p) {
    uint8_t nval=0;
    if ( p ) nval = p->getquality();

    return nval;
}

