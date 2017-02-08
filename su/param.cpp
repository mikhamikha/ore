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

#define _param_prc_delay    1000

using namespace std;

pthread_mutex_t mutex_pub   = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_param = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  data_ready  = PTHREAD_COND_INITIALIZER;
pthread_cond_t  pub_ready   = PTHREAD_COND_INITIALIZER;

paramlist tags;
bool fParamThreadInitialized;

cparam::cparam() {
    setproperty( string("raw"),         int(0)      );
    setproperty( string("value"),       double(0)   );
    setproperty( string("quality"),     int32_t(0)  );
    setproperty( string("timestamp"),   string("")  );  
    setproperty( string("deadband"),    double(0)   );
    setproperty( string("sec"),         int32_t(0)  );
    setproperty( string("msec"),        int32_t(0)  );
    m_task = 0;
    m_task_go = false;
    m_task_delta = 10;
    m_sub = -2;
    m_quality = OPC_QUALITY_WAITING_FOR_INITIAL_DATA;
    m_readOff = -1; 
    m_readbit = -1; 
    m_connErr = -1;
    m_writeOff= -1;
    m_deadband= 0;
    m_raw = 0;
    m_raw_old = 0;
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
    getproperty( "minraw",  m_minRaw    );
    getproperty( "maxraw",  m_maxRaw    );
    getproperty( "mineng",  m_minEng    );
    getproperty( "maxeng",  m_maxRaw    ); 
    getproperty( "flttime", m_fltTime   ); 
    int16_t bt;
    getproperty( "isbool",  bt          ); m_isBool = (bt!=0);
    getproperty( "hihi",    m_hihi      );
    getproperty( "hi",      m_hi        );
    getproperty( "lolo",    m_lolo      );
    getproperty( "lo",      m_lo        );
    getproperty( "deadband",m_deadband  );
    getproperty( "name",    m_name      );
       
    int16_t nPortErrOff, nErrOff;
    if( mb->getproperty("commanderror", nPortErrOff) == EXIT_SUCCESS && \
                getproperty("ErrPtr", nErrOff) == EXIT_SUCCESS ) {     // read errors of read modbus operations
         if( (nErrOff + nPortErrOff) < mb->m_maxReadData ) m_connErr = nErrOff + nPortErrOff;
    }
}

//
//  Расчет / задание положения клапана по показанию датчика Холла, конечным выключателям и типу команды
//  
int16_t cparam::rawValveValueEvaluate() {
    double  ncnt, nop, ncl, ncmop, ncmcl;
    int16_t qual;
    time_t  t;
    int16_t rc = EXIT_SUCCESS;
    string  scnt, sop, scl, scmop, scmcl;
    int16_t numop, numcl;

    numop = atoi( string(m_name, 2, 2).c_str() );
    numcl = numop + 1;
   
    scnt  = "FC"+to_string(numop);
    sop   = "ZV"+to_string(numop);
    scl   = "ZV"+to_string(numcl);
    scmop = "CV"+to_string(numop);
    scmcl = "CV"+to_string(numcl);
    
    getparam( scnt , ncnt,  qual, &t );  // FC
    getparam( sop  , nop,   qual, &t );  // ZV opened
    getparam( scl  , ncl,   qual, &t );  // ZV closed
    getparam( scmop, ncmop, qual, &t );  // CV open cmd
    getparam( scmcl, ncmcl, qual, &t );  // CV close cmd
    
    string _reset("0");
    string _set("1");
    string _task("task");
    
    m_raw = m_raw_old + (ncmop-ncmcl)*(ncnt-m_raw_old);

    if( (ncmcl && ncmop) || m_tasktimer.isDone() ) {
        taskparam( scmop, _task, _reset );
        taskparam( scmcl, _task, _reset );
        m_tasktimer.reset();
    }

    if( m_task_go && ncmop==0 && ncmcl==0 ) {
        bool    fGO = false;

        if( m_minRaw==-1 ) {
            taskparam( scmcl, _task, _set ); 
            fGO = true;
        } else if( m_minRaw>=0 ) {
            if( m_task - m_raw > m_task_delta ) { taskparam( scmop, _task, _set ); fGO = true; }
            if( m_raw - m_task > m_task_delta ) { taskparam( scmcl, _task, _set ); fGO = true; }
        }
        m_task_go = false;
        if(fGO) m_tasktimer.start(180000);
    }

    if( (nop+ncl) != 0 ) {
        if( nop != 0 ) {
           if( ncmop != 0 ) {
                taskparam( scmop, _task, _reset );
                m_tasktimer.reset();
                if( m_minRaw==-2 && ncnt > 5000 ) { m_minRaw = 0; m_maxRaw = ncnt; }
            }
            m_raw = m_maxRaw;
        }
        if( ncl != 0 ) {
            m_raw = 0;
            if( ncmcl != 0 ) {
                taskparam( scmcl, _task, _reset );
                m_tasktimer.reset();
                if( m_minRaw==-1 ) { m_minRaw = -2; taskparam( scmop, _task, _set ); }
            }
        }
        taskparam( scnt, _task, _reset );                
    }
    if( abs(m_raw-m_task) < m_task_delta ) {
        if( ncmop != 0 ) { taskparam( scmop, _task, _reset ); m_tasktimer.reset(); }
        if( ncmcl != 0 ) { taskparam( scmcl, _task, _reset ); m_tasktimer.reset(); }
    }

    return rc;
}

int16_t cparam::getraw(int16_t &nOut) {
    int16_t     rc  = EXIT_FAILURE;
    cmbxchg     *mb = (cmbxchg *)p_conn;

    m_raw_old = m_raw;
    setproperty( "raw_old", m_raw_old );

    if( m_readOff >= 0 ) {
        m_raw = mb->m_pReadData[m_readOff];
        if( m_readbit >= 0 ) {
            m_raw = ( m_raw & ( 1 << m_readbit ) ) != 0;
        }
        nOut = m_raw;
        setproperty("raw", m_raw);
        rc=EXIT_SUCCESS;
    }
    else if( m_readOff == -2 && m_name.find("FV") == 0 ) {
        rc = rawValveValueEvaluate();
        nOut = m_raw;        
        setproperty("raw", m_raw);    
    }
    return rc;
}
//
//  обработка параметров аналогового типа
//  приведение к инж. единицам, фильтрация, аналог==дискрет
//  анализ изменения значения сравнением с зоной нечувствительности
//
int16_t cparam::getvalue(double &rOut, uint8_t &nQual) {
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

    clock_gettime(CLOCK_MONOTONIC,&tv);
    tv.tv_sec += dT;
    nctt = tv.tv_sec*_million + tv.tv_nsec/1000;
    nodt = m_ts.tv_sec*_million + m_ts.tv_nsec/1000;
    nD = abs(nctt-nodt);
//   cout << nodt << " | " << nctt << " | " << nD << endl; 
    
    if( (rc = getproperty("simen", rsim_en) | \
            getproperty("simva", rsim_v)) == EXIT_SUCCESS && rsim_en != 0 ) { // simulation mode switched ON 
        rVal  = rsim_v;
        rOut  = rsim_v;
        nQual = OPC_QUALITY_GOOD;
        rc = EXIT_SUCCESS;
    }
    else  
        rc = getraw(nVal);

    if( rc==EXIT_SUCCESS ) {
//        double  lraw, hraw, leng, heng;
//        int16_t isbool, nstep;
//        double  dead=0;
        if( rsim_en == 0 ) {                                      // simulation mode switched OFF
            if( m_maxRaw!=m_minRaw && m_maxEng!=m_minEng ) {
                rVal = (m_maxEng-m_minEng)/(m_maxRaw-m_minRaw)*(nVal-m_minRaw)+m_minEng;
                nTime = m_fltTime*1000;
                rVal = (m_rvalue*nTime+rVal*nD)/(nTime+nD); 
                if(m_isBool) rVal = (rVal > m_hihi);                      // if it is a discret parameter
                rOut = rVal;                                            // current value

                if ( m_connErr >= 0 ) {
                    nQual = (mb->m_pReadData[m_connErr])?OPC_QUALITY_NOT_CONNECTED:OPC_QUALITY_GOOD;
                }
            }
        }
        // save value if it (or quality) was changes
        if( m_deadband == 0 ||
               fabs(rVal-m_rvalue)>m_deadband || m_quality!=nQual || nD>60*_million) {
            m_rvalue = rVal;
            m_ts.tv_sec = tv.tv_sec;
            m_ts.tv_nsec = tv.tv_nsec;
            m_valueupdated = true;
            m_quality = nQual;
            setproperty("value", rVal);
            setproperty("quality", nQual);
            setproperty("sec",  int32_t(tv.tv_sec));
            setproperty("msec", int32_t(tv.tv_nsec/_million));
//                cout << getproperty("name")->_s<<" |v "<< getproperty("value")->ToString()<<" |v "<<m_rvalue<< \
                " |d "<<dead<<" |ds "<<getproperty("dead")->_s<< " |dt "<< getproperty("dead")->_t<< \
                " |q "<<int(nQual)<<" |q "<<int(m_quality)<<" |dt "<<nD/_million<<endl;
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
        mb->m_pWriteData[m_writeOff] = m_task;
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
            else if(lineL.find("display") == 1 && lineL.find("view")) {
                int32_t num = getnumfromstr(lineL, "view", "]");
                nline = 0;
                parseBuff(fstr, _parse_display, (void *)num);
//                cout << "parse disp 1 " << num << endl;       
            }
        }
        else if( obj ) {
            switch ( type ) {
                case _parse_display: {
                        int32_t ndisp  = int32_t(obj);
                        std::string sval, stag;
                        std::istringstream iss( lineorig );

                        if( std::getline( iss, sval, ';') ) {
                            removeCharsFromString(sval, (char*)(" "));
                            nline = atoi( sval.c_str() );
                            std::getline( iss, sval );
                            dsp.definedspline( ndisp, nline, sval );
//                        cout << "parse disp 2 " << ndisp << " " << nline << " " << sval << endl;
                        }
                        else
                        if( std::getline( iss, stag, '=') ) {
                            removeCharsFromString(stag, (char*)(" "));
                            std::getline( iss, sval );
                            removeCharsFromString(sval, (char*)(" "));
                            dsp.setproperty( ndisp, stag, sval );
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
                            string s;
                            while( std::getline( iss, sVal, ';') ) {
                                p.setproperty( prop[nI].first, sVal );
                                nI++;
                            }
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
//    cout << tags.size() << endl;
    for(i=0; i<tags.size(); i++) {
//        cout << tags[i].getPropertySize() << endl;
        for(j=0; j<tags[i].second.getpropertysize(); j++) {
//            cout << tags[i].getproperty(j)->ToText() << endl;
        }    
    }
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
int16_t getparam( std::string& na, double& va, int16_t& qual, time_t* ts ) {
    int16_t     rc=EXIT_FAILURE;
    double      rval;
    uint8_t     kval;
    
        pthread_mutex_lock( &mutex_param );
        paramlist::iterator ifi = find_if( tags.begin(), tags.end(), compareP<cparam>(na) );
        if( ifi != tags.end() ) {
            ifi->second.getvalue( rval, kval );
            qual = kval;
            va = rval;
            ts = ifi->second.getTS();
            rc=EXIT_SUCCESS;
        }
        pthread_mutex_unlock( &mutex_param );

    return rc;
}

//
// ask value parameter by it name
//
int16_t getparam(std::string& na, std::string& va) {
    int16_t     rc=EXIT_FAILURE;
    double      rval;
    uint8_t     kval;
    
        pthread_mutex_lock( &mutex_param );
        paramlist::iterator ifi = find_if( tags.begin(), tags.end(), compareP<cparam>(na) );
        if( ifi != tags.end() ) {
            ifi->second.getvalue( rval, kval );
            va = to_string(rval);
            if(kval!=OPC_QUALITY_GOOD) va += " bad";
            rc=EXIT_SUCCESS;
        }
        pthread_mutex_unlock( &mutex_param );

    return rc;
}

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
        cout<<" name "<<stag<<" field "<<sf<< " value "<<val<<" size "<<val.size()<<endl;
            
        pthread_mutex_lock( &mutex_param );
        paramlist::iterator ifi = find_if( tags.begin(), tags.end(), compareP<cparam>(stag) );
        if( ifi != tags.end() ) {
            std::istringstream(val) >> rval;
            if( sf.find("task") != std::string::npos ) ifi->second.settask( rval );
            else ifi->second.setproperty(sf, rval);
            rc=EXIT_SUCCESS;
        }
        pthread_mutex_unlock( &mutex_param );
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
    ih   = tags.begin(); iend = tags.end();
    while ( ih != iend ) {
        ih->second.init();
        ++ih;   
    }
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
                nRes = pp.getraw( nVal );
                nRes1= pp.getvalue( rVal, nQ );
                
                if( pp.hasnewvalue() && (nu=pp.getpubcon())>=0 && nu<upc.size() ) {
                    upc[nu]->publish(pp);                        
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


