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
    m_init = false;
    m_name.assign("");
}

void cparam::init() {
    string sOff; 
    cmbxchg     *mb = (cmbxchg *)p_conn;

    if( getproperty("readdata", sOff)==0 && !sOff.empty() ) {
        if( isdigit(sOff[0]) ) {
            vector<string> vc;
            int16_t nOff;
            strsplit(sOff, '.', vc);
            nOff = atoi(vc[0].c_str());
            if(nOff<mb->m_maxReadData) { 
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
    getproperty( "minraw",  m_minRaw    ) | \
    getproperty( "maxraw",  m_maxRaw    ) | \
    getproperty( "mineng",  m_minEng    ) | \
    getproperty( "maxeng",  m_maxEng    ) | \
    getproperty( "flttime", m_fltTime   ) | \
    getproperty( "isbool",  m_isBool    ) | \
    getproperty( "hihi",    m_hihi      ) | /*getproperty( "hihi", she ) | getproperty( "hihi", nhe ) |*/ \
    getproperty( "hi",      m_hi        ) | \
    getproperty( "lolo",    m_lolo      ) | \
    getproperty( "lo",      m_lo        ) | \
    getproperty( "deadband",m_deadband  ) | \
    getproperty( "name",    m_name      ) | \
    getproperty( "topic",   m_topic     );
//    m_isBool = (bt!=0);

    int16_t nPortErrOff, nErrOff;
//    cout<<"param::init "<<m_name<<" rc="<<rc<<" bool? "<<m_isBool<<" deadband "<<m_deadband<< \
        " maxE "<<m_maxEng<<" minE "<<m_minEng<<" maxR "<<m_maxRaw<<" minR "<<m_minRaw<< \
        " hihi "<<m_hihi<<" hi "<<m_hi<<" lolo "<<m_lolo<<" lo "<<m_lo<<endl;
    if( (m_readOff >= 0 || m_writeOff >= 0) && mb->getproperty("commanderror", nPortErrOff) == EXIT_SUCCESS && \
                getproperty("ErrPtr", nErrOff) == EXIT_SUCCESS ) {     // read errors of read modbus operations
         if( (nErrOff + nPortErrOff) < mb->m_maxReadData ) m_connErr = nErrOff + nPortErrOff;
    }

    if( m_readOff == -2 ) {
        if( m_name.find("FV") == 0 ) {   // if parameter must be evaluate from other
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
        if( m_name.find("FT") == 0 ) {   // if parameter must be evaluate from other
            string  s1, s2, s3, s4, s5;
            int16_t num;
            
            num = atoi( m_name.substr(2, 2).c_str() );
           
            s1 = "DT"+to_string(num);
            s2 = "KV"+to_string(num);
            s3 = "LT"+to_string(num);
            s4 = "PT"+to_string(num);
            s5 = "PT31"/*+to_string(num)*/;
           
            m_dt      = getparam( s1.c_str() );  // density 
            m_kv      = getparam( s2.c_str() );  // Kv factor
            m_fv      = getparam( s3.c_str() );  // valve % opened
            m_pt1     = getparam( s4.c_str() );  // давление в пласте
            m_pt2     = getparam( s5.c_str() );  // давление у насоса
        }
        else
        if( m_name.find("LT") == 0 ) {   // if parameter must be evaluate from other
            string  s1;
            int16_t num;
            
            num = atoi( m_name.substr(2, 2).c_str() );
           
            s1 = "FV"+to_string(num);
           
            m_pos = getparam( s1.c_str() );  // valve position in percent 
        }
   }

    m_init=true;
}

//
//  Расчет / задание положения клапана по показанию датчика Холла, конечным выключателям и типу команды
//  
int16_t cparam::rawValveValueEvaluate() {
    int16_t lso, lso_old, lsc, lsc_old, cmdo, cmdc, cnt;
//    int16_t qual;
    time_t  t;
    int16_t rc, rc1, rc2;
    rc = m_fc->m_quality != OPC_QUALITY_GOOD;
    rc1 = ( ( m_lso->m_quality | m_lsc->m_quality ) != OPC_QUALITY_GOOD );
    rc2 = ( ( m_cmdo->m_quality | m_cmdc->m_quality ) != OPC_QUALITY_GOOD );
  
    m_quality = (rc||rc1||rc2) ? OPC_QUALITY_BAD : OPC_QUALITY_GOOD;
        
    if( rc || rc1 || rc2 ) {
        cout<<"ICP qual="<<rc<<" DI qual="<<rc1<<" DO qual="<<rc2<<endl; 
        return EXIT_FAILURE; 
    }
    else rc = EXIT_SUCCESS; 

    lso     = m_lso->m_rvalue;
    lsc     = m_lsc->m_rvalue;
    lso_old = m_lso->m_rvalue_old;
    lsc_old = m_lsc->m_rvalue_old;
    cmdo    = m_cmdo->m_rvalue;    
    cmdc    = m_cmdc->m_rvalue;
    cnt     = int(m_fc->m_rvalue)%_ten_thou;

//    m_raw = m_raw_old + (cmdo - cmdc)*(cnt%_ten_thou - m_raw_old);

    if( (cmdo ^ cmdc) != 0 ) {
        m_raw = m_raw_old + (cmdo-cmdc)*(cnt - m_cnt_old);
    }
    else m_raw = m_raw_old;
    
/*    
    if( m_name.substr(0,3)=="FV1") {
        cout<<dec<<" LSOold= "<<lso_old<<" LSO= "<<lso<<" LSCold= "<<lsc_old<<" LSC= "<<lsc \
            <<" cmdOpen= "<<cmdo<<" cmdClose= "<<cmdc \
            <<" cnt="<<m_cnt_old<<"|"<<cnt<<" raw_old="<<m_raw_old<<" raw_new="<<m_raw<<" task="<<m_task<<endl;    
    }
*/
    if( (cmdo && cmdc) || m_tasktimer.isDone() ) {
        m_cmdo->settask( 0 );
        m_cmdc->settask( 0 );
        m_tasktimer.reset();
    }

    if( m_task_go && cmdo==0 && cmdc==0 ) {
        bool fGO = false;

        if( m_minRaw==-1 ) {
            cout<<" valve "<< m_cmdc->m_name <<" cmd go "<<m_task<<" %"<<endl;
            m_cmdc->settask( 1 ); 
            fGO = true;
        } else if( m_minRaw>=0 ) {
            if( m_task - m_raw > m_task_delta ) { m_cmdo->settask( 1 ); fGO = true; }
            if( m_raw - m_task > m_task_delta ) { m_cmdc->settask( 1 ); fGO = true; }
        }
        m_task_go = false;

        if(fGO) m_tasktimer.start(180000);
    }

    if( lso && !lso_old ) {
       if( cmdo ) {
            m_cmdo->settask( 0 );
            m_tasktimer.reset();
            if( m_minRaw==-2 ) { m_minRaw = 0; m_maxRaw = cnt; m_task_delta = m_maxRaw/10; }
        }
        m_raw = m_maxRaw; m_raw_old = m_maxRaw; 
        m_fc->settask( /*( int(m_fc->m_rvalue) < _ten_thou ) ? _ten_thou :*/ 0 );
    }

    if( lsc ) {
        if( !lsc_old ) {
            if( cmdc ) {
                m_cmdc->settask( 0 );
                m_tasktimer.reset();
            }
            m_raw = 0; m_raw_old = 0; 
            m_fc->settask( /*( int(m_fc->m_rvalue) < _ten_thou ) ? _ten_thou :*/ 0 );
        }
        if( m_minRaw==-1 && !cnt ) { m_minRaw=-2; m_cmdo->settask( 1 ); }
    }

    if( cmdo && m_raw > (m_task-m_task_delta) && m_minRaw>=0 )  {
        m_cmdo->settask( 0 ); m_tasktimer.reset();
    }
    if( cmdc && m_raw < (m_task+m_task_delta) && m_minRaw>=0 ) {
        m_cmdc->settask( 0 ); m_tasktimer.reset();
    }
    m_cnt_old = cnt;
    
    return rc;
}

//
//  Расчет мгновенного расхода по положению клапана и перепаду давления
//  
int16_t cparam::flowEvaluate() {

    int16_t rc, rc1;

    rc = (m_pt1->m_quality != OPC_QUALITY_GOOD);
    rc1 = (m_fv->m_quality != OPC_QUALITY_GOOD);
  
    m_quality = (rc||rc1) ? OPC_QUALITY_BAD : OPC_QUALITY_GOOD;
        
    if( rc || rc1 ) {
        if(m_name.substr(0,3)=="FT1") { 
            double rsim_en, rsim_v;
            getproperty("simen", rsim_en);
            getproperty("simva", rsim_v);            
            cout<<"flow ev "<<m_name<<"  PT qual="<<rc<<" LT qual="<<rc1;
            cout<<" pt name "<<m_pt1->m_name<<" pt val "<<m_pt1->m_rvalue<<" pt val "<<m_pt1->m_rvalue \
                <<" pt simen "<<rsim_en<<" pt simval "<<rsim_v<<endl; 
        }
        return EXIT_FAILURE; 
    }
    else rc = EXIT_SUCCESS; 

    double sq, r1=1, r11=4, ht1 = m_fv->m_rvalue-1, _tan = 0.0448210728500398; 
    cout<<"fv\tpt1\tpt2\tkv\tdt\tsq\tflow\n";
    cout<<m_fv->m_name<<"\t"<<m_pt1->m_name<<"\t"<<m_pt2->m_name<<"\t"<<m_kv->m_name<<"\t"<<m_dt->m_name<<endl;
    cout<<m_fv->m_rvalue<<"\t"<<m_pt1->m_rvalue<<"\t"<<m_pt2->m_rvalue<<"\t"<<m_kv->m_rvalue<<"\t"<<m_dt->m_rvalue;
    

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
    
    cout<<"\t"<<sq<<"\t"<<m_raw<<endl;
    return rc;
}

int16_t cparam::getraw() {
    int16_t     rc  = EXIT_FAILURE;
    cmbxchg     *mb = (cmbxchg *)p_conn;

    m_raw_old = m_raw;
    setproperty( "raw_old", m_raw_old );
//if(m_name.substr(0,3)=="FC1") cout<<" getraw off="<<m_readOff<<" bit="<<m_readbit<<" rawval="<<uppercase<<hex;
    
    m_quality_old = m_quality;

    if( m_readOff >= 0 ) {
        m_raw = mb->m_pReadData[m_readOff];
//        cout<<m_raw<<" ";
        if( m_readbit >= 0 ) {
            m_raw = (( int(m_raw) & ( 1 << m_readbit ) ) != 0);
        }
//if(m_name.substr(0,3)=="FC1") cout<<m_raw<<" "<<dec<<endl;
        setproperty("raw", m_raw);
        rc=EXIT_SUCCESS;

        if ( m_connErr >= 0 ) {
            m_quality = (mb->m_pReadData[m_connErr])?OPC_QUALITY_NOT_CONNECTED:OPC_QUALITY_GOOD;
        }
    }
    else if( m_readOff == -2 ) {
        if(m_name.find("FV") == 0 ) {
            rc =rawValveValueEvaluate();
            setproperty("raw", m_raw);    
        }
        else
        if(m_name.find("LT") == 0 ) {
            rc=EXIT_SUCCESS;
            m_raw = m_pos->m_rvalue;
            m_quality = m_pos->m_quality;
            setproperty("raw", m_raw);    
        }
        else
        if(m_name.find("FT") == 0 ) {
            rc =flowEvaluate();
            setproperty("raw", m_raw);    
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
    cmbxchg     *mb = (cmbxchg *)p_conn;

    if(!m_init) {
        init();
    }
    clock_gettime(CLOCK_MONOTONIC,&tv);
    tv.tv_sec += dT;
    nctt = tv.tv_sec*_million + tv.tv_nsec/1000;
    nodt = m_ts.tv_sec*_million + m_ts.tv_nsec/1000;
    nD = abs(nctt-nodt);
   
    if( (rc = getproperty("simen", rsim_en) | \
            getproperty("simva", rsim_v)) == EXIT_SUCCESS && rsim_en != 0 ) { // simulation mode switched ON 
        rVal  = rsim_v;
        rOut  = rsim_v;
        m_quality = OPC_QUALITY_GOOD;
        rc = EXIT_SUCCESS;
    }
    else { 
        rc = getraw();
    }

    if( rc==EXIT_SUCCESS ) {
        if( rsim_en == 0 ) {                                      // simulation mode switched OFF
            if( m_maxRaw!=m_minRaw && m_maxEng!=m_minEng ) {
                rVal = (m_maxEng-m_minEng)/(m_maxRaw-m_minRaw)*(m_raw-m_minRaw)+m_minEng;
                nTime = m_fltTime*1000;
                if(rVal>m_maxEng) rVal = m_maxEng;
                if(rVal<m_minEng) rVal = m_minEng;               
                if( nD && nTime ) rVal = (m_rvalue*nTime+rVal*nD)/(nTime+nD); 
                if(m_isBool==1) rVal = (rVal >= m_hihi);                // if it is a discret parameter
                if(m_isBool==2) rVal = (rVal < m_hihi);                 // if it is a discret parameter & inverse
                rOut = rVal;                                            // current value
            }
        }
        if( m_name.substr(0,4)=="FT11") \
            cout <<"getvalue name="<<m_name<<" oldT "<< nodt << " | curT " << nctt << " | dT " << nD \
                <<" |v "<<dec<<rVal<<" |vOld "<<m_rvalue<<" |vOldOld "<<m_rvalue_old \
                <<" |raw "<<m_raw<<" |d "<<m_deadband<<" maxE "<<m_maxEng<<" minE "<<m_minEng \
                <<" maxR "<<m_maxRaw<<" minR "<<m_minRaw<<" hihi "<<m_hihi<<" | mConnErrOff "<<m_connErr \
                <<" |qOld "<<int(m_quality_old)<<" |q "<<int(m_quality)<<endl;

        m_rvalue_old = m_rvalue;            
        // save value if it (or quality) was changes
        if( fabs(rVal-m_rvalue)>=m_deadband || m_quality_old != m_quality || nD>60*_million) {
            m_ts.tv_sec = tv.tv_sec;
            m_ts.tv_nsec = tv.tv_nsec;
            m_valueupdated = true;
            setproperty("value", rVal);
            setproperty("quality", m_quality);
            setproperty("sec",  int32_t(tv.tv_sec));
            setproperty("msec", int32_t(tv.tv_nsec/_million));
//            if( m_name.substr(0,3)=="FV1") cout<<endl;
//                cout <<" |v "<< rVal<<" |vold "<<m_rvalue<< \
                " |d "<<m_deadband<<" | mConnErrOff "<<m_connErr<<" |q "<<int(nQual)<<" |dt "<<nD/_million<<endl;
            m_rvalue = rVal;
       } 
    }    
    
    return rc;
}

int16_t cparam::setvalue() {
    int16_t rc = EXIT_FAILURE;
    cmbxchg *mb;
    string  sOff;
    int16_t nOff;

    if( m_writeOff >= 0 ) {
//        if( m_name.substr(0,3)=="FC1")
//            cout<<"setvalue "<<m_name<<" task "<<m_task<<" off="<<m_writeOff<<" conn="<<int(mb)<<endl;      
        mb->m_pWriteData[m_writeOff] = m_task;
        mb->m_pLastWriteData[m_writeOff] = m_task-1;
        m_task_go = false; 
        rc = EXIT_SUCCESS;
    }
    else if( m_writeOff == -2 ) {
    }
    
    return rc;
}

int16_t parseBuff(std::fstream &fstr, int8_t type, void *obj=NULL) {    
    std::string             line;
    std::string             lineL;                  // line in low register
    std::string             lineorig;
    int16_t                 nTmp;
    settings                prop;
    cmbxchg                 *mb = (cmbxchg *)obj;
    upcon                   *up = (upcon *)obj;
    std::string::size_type  found;
    int16_t                 nline;

//    cout << "parsebuff = " << fstr << " type = " << (int)type << " obj = " << obj << endl<< endl;
    while( std::getline( fstr, line ) ) {
//        cout << line.c_str() << endl;
        lineorig = line;
        removeCharsFromString(line, (char *)" \t\r");
        lineL = line;
        std::transform(lineL.begin(), lineL.end(), lineL.begin(), easytolower);
        if(lineL.length()==0 || lineL[0]=='#') continue;
        if(lineL.find("start",0,5)!=std::string::npos) {
            continue;
        }
        if(lineL.find("end",0,3)!=std::string::npos) {
            continue;//break; 
        }
        if(lineL[0]=='[' && lineL[lineL.length()-1]==']') {
            found = lineL.find("modbusport");
            if(found != std::string::npos) {
                found = lineL.find("commands");
                if (obj && found != std::string::npos) {
                    if(getnumfromstr(lineL, "modbusport", "commands") == ((cmbxchg *)obj)->m_id) {
                        parseBuff(fstr, _parse_mbcmd, obj);
                    }
                }
                else 
                if (obj && (found=lineL.find("ai")) != std::string::npos){
                    if(getnumfromstr(lineL, "modbusport", "ai")== ((cmbxchg *)obj)->m_id) {
                        parseBuff(fstr, _parse_ai, obj);
                    }
                }
                else {
                    mb = new cmbxchg();
                    conn.push_back(mb);
                    mb->m_id = getnumfromstr(lineL, "modbusport", "]");
                    parseBuff(fstr, _parse_mbport, (void *)mb);
                }
            }
            else if(lineL.find("connection") == 1) {
                up = new upcon();
                upc.push_back(up);
                up->m_id = getnumfromstr(lineL, "connection", "]");
                parseBuff(fstr, _parse_upcon, (void *)up);
            }
            else if(lineL.find("display") == 1 && lineL.find("view")==std::string::npos) {
                int32_t num = getnumfromstr(lineL, "view", "]");
                nline = 0;
                parseBuff(fstr, _parse_display_def, (void *)num );
//                cout << "parse disp 1 " << num << endl;       
            }
           else if(lineL.find("display") == 1 && lineL.find("view")) {
                int32_t num = getnumfromstr(lineL, "view", "]");
                nline = 0;
                parseBuff(fstr, _parse_display, (void *)num);
//                cout << "parse disp 1 " << num << endl;       
            }
        }
        else if( obj ) {
            switch ( type ) {
                case _parse_display_def: {
                        int32_t ndisp  = int32_t(obj);
                        std::string sval, stag;
                        std::istringstream iss( lineorig );

                        if( std::getline( iss, stag, '=') ) {
                            removeCharsFromString(stag, (char*)(" "));
                            std::getline( iss, sval );
                            cout<<"cfg "<<stag<<" = "<<sval<<endl;
                            dsp.setproperty( stag, sval );
                        }
                    }
                    break;
               case _parse_display: {
                        int32_t         ndisp  = int32_t(obj);
                        vector<string>  vc;
                        std::string     sval, stag;
                       
//                        cout<<"|| "<<lineorig<<" ||"<<endl;
                        strsplit( lineorig, ';', vc );
                        if( vc.size() > 1 ) {
                            sval = vc[0]; nline = atoi( sval.c_str() );
//                            cout << "1parse disp " << ndisp << " " << sval << " | " <<vc[1]<<endl;
                            dsp.definedspline( ndisp, nline, vc[1] );
                        }
                        else {
                            strsplit( lineorig, '=', vc );
                            if( vc.size() > 1 ) {
                                stag = vc[0];
                                sval = vc[1];
                                removeCharsFromString(stag, (char*)(" "));
                                removeCharsFromString(sval, (char*)(" "));
//                                cout << "2parse disp " << ndisp << " " << stag << " | " <<sval<<" s="<<vc.size()<<endl;
                                if(ndisp) dsp.setproperty( ndisp-1, stag, sval );
                            }
                        }
                    }
                    break;
                case _parse_mbport: 
                case _parse_upcon: {
                        std::istringstream iss( line );
                        std::string sTag;
                        std::string sVal;
                        if( std::getline( iss, sTag, '=') ) {
                            std::getline( iss, sVal );
                            if( type == _parse_upcon )
                                up->setproperty( sTag, sVal );
                            else
                                mb->setproperty( sTag, sVal );
                        }
                    }
                    break;                   
                case _parse_mbcmd: {
                        std::istringstream iss( line+";" );
                        std::vector<int16_t> result;
                        std::string sVal;
                        while( std::getline( iss, sVal, ';') ) {
                            result.push_back(atoi(sVal.c_str()));
                        }
                        ccmd cmd(result);
//                        cout<<"parse cmds count = "<<result.size()<<endl;
                        mb->mbCommandAdd(cmd);
                    }
                    break;
                case _parse_ai: {
                        std::string sVal;
                        std::istringstream iss( line+";" );
                        if(lineL.find("topic") == 0) {
                            while( std::getline( iss, sVal, ';') ) {
                                prop.push_back(std::make_pair(sVal, content("")));
                            }
                        }
                        else {
                            cparam  p;
                            int32_t nI=0;
                            string s, n;
                            while( std::getline( iss, sVal, ';') ) {
                                p.setproperty( prop[nI].first, sVal );
//                                cout<<prop[nI].first<<"="<<sVal<<"| ";
                                nI++;
                            }
//                            cout<<endl;
                            nI=0; 
                            while(nI < p.getpropertysize()) {
                                p.getproperty(nI, n, s);
//                                cout<<n<<" "<<s<<"| ";
                                nI++;
                            }
//                            cout<<endl;
                           
                            p.p_conn = obj;
                            if(p.getproperty("name", s)==EXIT_SUCCESS) tags.push_back(make_pair(s, p));
                        }
                    }
                    break;
            }
        }
    }
    return EXIT_SUCCESS;
}
//  
//	Чтение и парсинг конфигурационного файла
//	name;mqtt;type;address;
//
int16_t readCfg() {
	int16_t     res=0;
    int16_t     nI=0, i, j;
    cmbxchg     *mb=NULL;  

//    cout << "conn size = " << conn.size() << endl;
//    cout << "readcfg" << endl;
	std::fstream filestr("map.cfg");
    parseBuff(filestr, _parse_root, (void *)mb);

/*  // print config  
//    cout << "conn size = " << conn.size() << endl;
    for(i=0; i<conn.size(); i++) {
        mb = conn[i];
//        cout << mb << " | port settings " << mb->portPropertyCount() << endl;
        for(j=0; j < mb->getpropertysize(); ++j) {
//            cout << setfill(' ') << setw(2) << j << " | " <<  mb->portProperty2Text(j) << endl;
        }
//        cout << "mb commands " << mb->mbCommandsCount() << endl;
        for(j=0; j < mb->mbCommandsCount(); ++j) {
//            cout << mb->mbCommand(j)->ToString() << endl;
        }
    }
    */
    /*
    cout << tags.size() << endl;
    for(i=0; i<tags.size(); i++) {
        cout << tags[i].first<<" | ";
        for(j=0; j<tags[i].second.getpropertysize(); j++) {
            string va; tags[i].second.property2text(j, va);            
            cout << " " << va;
        }  
        cout << endl;
    }
    */
/*    
//    cout << "upc props =" << upc[0]->getpropertysize() << endl;
    for(i=0; i<upc.size(); i++) {
//        cout << tags[i].getPropertySize() << endl;
        for(j=0; j<upc[i]->getpropertysize(); j++) {
            string s; upc[i]->property2text(j, s);
//            cout << s << endl;
        }    
    }
    cout << endl;
*/
    return res;
}

//
// ask value parameter by it name
//
int16_t getparam( const char* na, double& va, int16_t& qual, timespec* ts ) {
    int16_t     rc=EXIT_FAILURE;

    pthread_mutex_lock( &mutex_param );
    cparam* pp = getparam( na );
    if( pp!=NULL ) {
        qual= pp->getquality();
        va  = pp->getvalue();
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
    
    removeCharsFromString(val, (char *)" \t\n");
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
    
    removeCharsFromString(val, (char *)" \t\n");
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
    sleep(2);
    while ( fParamThreadInitialized ) {
        pthread_mutex_lock( &mutex_param );
        
//        pthread_cond_wait( &data_ready, &mutex_param );// start processing after data receive     
        ih   = tags.begin();
        iend = tags.end();
        while ( ih != iend ) {
            string  sOff;
            int16_t nu;
            cparam  &pp = ih->second;
//            int rc = pp.getproperty("readdata", sOff);
//            if( !rc && !sOff.empty() ) {
//                nRes = pp.getraw( nVal );
                nRes1= pp.getvalue( rVal );
               
                if( pp.hasnewvalue() && (nu=pp.getpubcon())>=0 && nu<upc.size() ) {
                    /*upc[nu]->*/publish(pp);                        
                }

//            }
//            else {
//                rc = pp.getproperty("writedata", sOff);

                if( pp.taskset() ) pp.setvalue();
//            }

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


