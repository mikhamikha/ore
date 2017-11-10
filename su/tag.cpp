
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

using namespace std;

pthread_mutex_t         mutex_pub   = PTHREAD_MUTEX_INITIALIZER;
//pthread_cond_t  data_ready  = PTHREAD_COND_INITIALIZER;
//pthread_cond_t  pub_ready   = PTHREAD_COND_INITIALIZER;

pthread_mutex_t         mutex_tag;// = PTHREAD_MUTEX_INITIALIZER;
pthread_mutexattr_t     mutex_tag_attr;

bool ftagThreadInitialized;

ctag::ctag() {
    setproperty( string("raw"),         double(0)  );
    setproperty( string("value"),       double(0)  );
    setproperty( string("task"),        double(0)  );
    setproperty( string("quality"),     int32_t(0) );
    setproperty( string("timestamp"),   string("") );  
    setproperty( string("dead"),        double(0)  );
    setproperty( string("sec"),         int32_t(0) );
    setproperty( string("msec"),        int32_t(0) );
    setproperty( string("mineng"),      double(0)  );   
    setproperty( string("name"),        string("") );
    setproperty( string("simenable"),   int(0)     );    
    setproperty( string("simvalue"),    double(0)  );    
    setproperty( string("mindev"),      double(0)  );   
    setproperty( string("maxdev"),      double(0)  );   
    
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
    getproperty( "minraw",  m_minRaw   ) | \
    getproperty( "maxraw",  m_maxRaw   ) | \
    getproperty( "mineng",  m_minEng   ) | \
    getproperty( "maxeng",  m_maxEng   ) | \
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
    
    cout<<"tag::init "<<m_name<<" rc="<<rc<<" bool? "<<m_type<<" deadband "<<m_deadband<< \
        " maxE "<<m_maxEng<<" minE "<<m_minEng<<" maxR "<<m_maxRaw<<" minR "<<m_minRaw<< \
        " hihi "<<m_hihi<<" hi "<<m_hi<<" lolo "<<m_lolo<<" lo "<<m_lo;
    
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
            cout<<"init "<<m_name<<" pos="<<hex<<long(m_pos)<<dec<<endl;
        }
    }
    cout<<endl;
    double rval;
    if( getproperty("default", rval)==_exOK ) { setvalue(rval); } 
    // подписка на команды
    uint16_t nu;
    if( (nu=getsubcon())>=0 && nu<upc.size() ) {
        upc[nu]->subscribe( this );
//      pp.m_sub = nu;
    }
}

int16_t ctag::getraw() {
    int16_t     rc  = _exOK;
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
int16_t ctag::getvalue(double &rOut) {
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
        rOut  = rsim_v;
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
                    rVal =  (m_maxEng-m_minEng)/(m_maxDev-m_minDev)*(rVal-m_minDev)+m_minEng;
                    rVal = min( rVal, m_maxEng );                                   // ограничим значение инженерной шкалой
                    rVal = max( rVal, m_minEng );
                }
                
                nTime = m_fltTime*1000;
                if( nD && nTime ) rVal = (m_rvalue*nTime+rVal*nD)/(nTime+nD); 
                if(m_type==1) rVal = (rVal >= m_hihi);                              // if it is a discret tag
                if(m_type==2) rVal = (rVal < m_hihi);                               // if it is a discret tag & inverse
                rOut = rVal;                                                        // current value
            }
            else {
                rOut = m_raw;
                rVal = m_raw;
            }
        }
      
/*        if( m_name.substr(0,4)=="MV11") \
            cout <<"getvalue name="<<m_name<<" TS= "<< nodt << "|" << nctt << " dT= " << nD <<" readOff="<<m_readOff \
                <<" val= "<<dec<<m_rvalue_old<<"|"<<m_rvalue<<"|"<<rVal \
                <<" raw= "<<m_raw_old<<"|"<<m_raw<<" dead= "<<m_deadband<<" engSc= "<<m_minEng<<"|"<<m_maxEng \
                <<" rawSc= "<<m_minRaw<<"|"<<m_maxRaw<<" hihi= "<<m_hihi<<" mConnErrOff= "<<m_connErr \
                <<" q= "<<int(m_quality_old)<<"|"<<int(m_quality)<<endl;*/

        //      save value if it (or quality or timestamp) was changes
        if( fabs(rVal-m_rvalue)>=m_deadband || m_quality_old != m_quality || nD>60*_million ) {
            m_ts.tv_sec = tv.tv_sec;
            m_ts.tv_nsec = tv.tv_nsec;
            m_valueupdated = true;
            setproperty("value", rVal);
            setproperty("quality", m_quality);
            setproperty("sec",  int32_t(tv.tv_sec));
            setproperty("msec", int32_t(tv.tv_nsec/_million));
/*          if( m_name.substr(0,3)=="MV1") cout<<endl;
                cout <<" |v "<< rVal<<" |vold "<<m_rvalue<< \
                " |d "<<m_deadband<<" | mConnErrOff "<<m_connErr<<" |q "<<int(nQual)<<" |dt "<<nD/_million<<endl;
*/
            m_rvalue = rVal;
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
        settask( rin, false );
        rc = _exOK;   
    }
    
    return rc;
}

int16_t ctag::settask(double rin, bool fgo) {
    int16_t rc = _exOK;
    double  rVal = rin;
   
   cout<<endl<<"settask "<<m_name<<" task=="<<rVal; 
   
   if( m_maxRaw-m_minRaw!=0 && m_maxEng-m_minEng!=0 ) {
        if( m_minDev!=m_maxDev )  {                                         // если задана шкала параметра < шкалы прибора,
            rVal = (m_maxDev-m_minDev)/(m_maxEng-m_minEng)*(rVal-m_minEng)+m_minDev;
            rVal = min( rVal, m_maxDev );                                   // ограничим значение инженерной шкалой
            rVal = max( rVal, m_minDev );
            cout<<" dev="<<rVal;
        }        
        rVal = (m_maxRaw-m_minRaw)/(m_maxEng-m_minEng)*(rVal-m_minEng)+m_minRaw;
        cout<<" raw="<<rVal;
   }
    m_task    = rVal;  
    if(fgo) m_task_go = fgo; 
    
//      if( m_name.substr(0,3)=="FC1")
    cout<<endl; 
    
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

int16_t publish(ctag &tag) {    
    int16_t         res = EXIT_FAILURE;
    string          topic;
    string          ts;
    string          sf;
    int16_t         rc;
//    int32_t         sec, msec;
   
    rc = upc[0]->getproperty("pubf", sf) == EXIT_SUCCESS/* && \
         tag.getproperty("name", name)    == EXIT_SUCCESS;
         tag.getproperty("topic", topic)  == EXIT_SUCCESS && \
         tag.getproperty("value", val)    == EXIT_SUCCESS && \
         tag.getproperty("quality", kval) == EXIT_SUCCESS && \
         tag.getproperty("sec", sec) == EXIT_SUCCESS && \
         tag.getproperty("msec", msec) == EXIT_SUCCESS*/;
         
    if(rc==0) {
        cout << "cfg: prop not found" << endl; 
    }
    else {
        timespec* tts;
        tts = tag.getTS();
//        ts = time2string(tts->tv_sec)+"."+to_string(int(tts->tv_nsec/_million));        
        ts = to_string( int32_t(tts->tv_sec*1000 + int(tts->tv_nsec/_million)) );
        replaceString(sf, "value", to_string(tag.getvalue()) );
        replaceString(sf, "quality", to_string(int(tag.getquality())) );
        replaceString(sf, "timestamp", ts);

        pthread_mutex_lock( &mutex_pub );
        if(pubs.size()<_pub_buf_max) {
            tag.getfullname(topic);
            pubs.push_back(make_pair(topic, sf));
            res = EXIT_SUCCESS;
        }
        pthread_mutex_unlock( &mutex_pub );
//        cout<<setfill(' ')<<setw(12)<<left<<topic+"/"+name<<" | "<<sf<<endl;
        tag.acceptnewvalue();
    }
    return res;
}

