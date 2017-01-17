#include "main.h"
#include "param.h"
#include "mbxchg.h"

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

cparam::cparam()
{
    addproperty( string("raw"),         int(0)      );
    addproperty( string("value"),       double(0)   );
    addproperty( string("quality"),     int32_t(0)  );
    addproperty( string("timestamp"),   string("")  );  
    addproperty( string("deadband"),    double(0)   );
    addproperty( string("sec"),         int32_t(0)  );
    addproperty( string("msec"),        int32_t(0)  );
    m_task = 0;
    m_task_go = false;
}

void cparam::addproperty(std::string na, std::string v)
{
    settings::iterator ifi = find_if(m_prop.begin(), m_prop.end(), compareP<content>(na));
    if(ifi == m_prop.end()) {
        m_prop.push_back(std::make_pair(na, content(v)));
    }
    return;
}

void cparam::addproperty(std::string na, int32_t v=0)
{
    settings::iterator ifi = find_if(m_prop.begin(), m_prop.end(), compareP<content>(na));
    if(ifi == m_prop.end()) {
        m_prop.push_back(std::make_pair(na, content(v)));
    }
    return;
}

void cparam::addproperty(std::string na, double v=0)
{
    settings::iterator ifi = find_if(m_prop.begin(), m_prop.end(), compareP<content>(na));
    if(ifi == m_prop.end()) {
        m_prop.push_back(std::make_pair(na, content(v)));
    }
    return;
}

int16_t cparam::getraw(int16_t &nOut)
{
    int16_t res=EXIT_FAILURE;
    cmbxchg *mb;
    int16_t nOff;

    settings::iterator ifi = find_if(m_prop.begin(), m_prop.end(), compareP<content>("readdata"));
    if(ifi != m_prop.end()) {
        mb = (cmbxchg *)p_conn;
        nOff=(*ifi).second.ToInt();
        if(nOff<mb->m_maxReadData) {
            m_raw = mb->m_pReadData[nOff];
            setproperty("raw", m_raw);
            nOut = m_raw;
            res=EXIT_SUCCESS;
        }
    }
    return res;
}
//
//  обработка параметров аналогового типа
//  приведение к инж. единицам, фильтрация, аналог==дискрет
//  анализ изменения значения сравнением с зоной нечувствительности
//
int16_t cparam::getvalue(double &rOut, uint8_t &nQual)
{
    int16_t     res=EXIT_FAILURE;
    cmbxchg     *mb = (cmbxchg *)p_conn;
    int16_t     nVal;
    double      rVal;
    timespec    tv;
    int32_t     nTime;
    int64_t     nctt;
    int64_t     nD;
    int64_t     nodt;             // time on previous step
    int16_t     rc;

    clock_gettime(CLOCK_MONOTONIC,&tv);
    tv.tv_sec += dT;
    nctt = tv.tv_sec*_million + tv.tv_nsec/1000;
    nodt = m_ts.tv_sec*_million + m_ts.tv_nsec/1000;
    nD = abs(nctt-nodt);
//   cout << nodt << " | " << nctt << " | " << nD << endl; 
    if((res = getraw(nVal))==EXIT_SUCCESS) {
        double  lraw, hraw, leng, heng;
        int16_t isbool, nstep;
        double  dead=0;
        res = getproperty("minraw", lraw)   \
            | getproperty("maxraw", hraw)   \
            | getproperty("mineng", leng)   \
            | getproperty("maxeng", heng)   \
            | getproperty("flttime", nTime) \
            | getproperty("isbool", isbool) \
            | getproperty("hihi", nstep);
        if(res == EXIT_SUCCESS && hraw!=lraw && heng!=leng) {
            rVal = (heng-leng)/(hraw-lraw)*(nVal-lraw)+leng;
            nTime = nTime*1000;
            rVal = (m_rvalue*nTime+rVal*nD)/(nTime+nD); 
            if(isbool) rVal = (rVal>nstep);                         // if it is a discret parameter
            rOut = rVal;                                            // current value

            int16_t nPortErrOff, nErrOff;
            nPortErrOff = mb->portProperty("commanderror")._n;

            if (getproperty("ErrPtr", nErrOff)==EXIT_SUCCESS) {     // read errors of read modbus operations
                nErrOff += nPortErrOff;
                if(nErrOff<mb->m_maxReadData) {
                    nQual = (mb->m_pReadData[nErrOff])?OPC_QUALITY_NOT_CONNECTED:OPC_QUALITY_GOOD;
                }
            }
            
            // save value if it (or quality) was changes
            if((rc=getproperty("deadband", dead))==EXIT_FAILURE ||
                   fabs(rVal-m_rvalue)>dead || m_quality!=nQual || nD>60*_million) {
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
    }
    return res;
}

        
int16_t cparam::setvalue(int16_t nIn)
{
    m_task = nIn;
    m_task_go = true;
    return EXIT_SUCCESS;
}

int16_t cparam::taskprocess()
{
    int16_t res=EXIT_FAILURE;
    cmbxchg *mb;
    int16_t nOff;

    settings::iterator ifi = find_if(m_prop.begin(), m_prop.end(), compareP<content>("writedata"));
    if(ifi != m_prop.end()) {
        mb = (cmbxchg *)p_conn;
        nOff=(*ifi).second.ToInt();
        if(nOff<mb->m_maxWriteData) {
            mb->m_pWriteData[nOff] = m_task;
            m_task_go = false; 
            res=EXIT_SUCCESS;
        }
    }
    return res;
}

content* cparam::getproperty(int16_t n)
{
    content *res=NULL;
    if(n<m_prop.size()) {
        res=&m_prop[n].second;
    }
    return res;
}

content* cparam::getproperty(std::string s)
{
    content *res=NULL;
    settings::iterator ifi = find_if(m_prop.begin(), m_prop.end(), compareP<content>(s));
    if(ifi != m_prop.end()) {
        res=&((*ifi).second);
    }
    return res;
}

template <class T>
int16_t cparam::getproperty(std::string s, T &rOut)
{
    int16_t res=EXIT_FAILURE;
    settings::iterator ifi = find_if(m_prop.begin(), m_prop.end(), compareP<content>(s));
    if(ifi != m_prop.end()) {
        (*ifi).second.getvalue(rOut);
        res=EXIT_SUCCESS;
    }
    return res;
}

int16_t cparam::setproperty(int16_t n, std::string na, std::string v)
{
    int16_t res=EXIT_FAILURE;
    if(n<m_prop.size()) {
        m_prop[n].first = na;
        m_prop[n].second._s   = v;
        res=EXIT_SUCCESS;
    }
    return res;
}
/*
int16_t cparam::setproperty(std::string s, std::string &sIn)
{
    int16_t res=EXIT_FAILURE;
    settings::iterator ifi = find_if(m_prop.begin(), m_prop.end(), compareP<content>(s));
    if(ifi != m_prop.end()) {
        (*ifi).second._s = sIn;
        res=EXIT_SUCCESS;
    }
    return res;
}
*/
template <class T> 
int16_t cparam::setproperty(std::string s, T nIn)
{
    int16_t res=EXIT_FAILURE;
    settings::iterator ifi = find_if(m_prop.begin(), m_prop.end(), compareP<content>(s));
    if(ifi != m_prop.end()) {
        (*ifi).second.setvalue(nIn);
        res=EXIT_SUCCESS;
    }
    return res;
}

int16_t parseBuff(std::fstream &fstr, int8_t type, void *obj=NULL)
{    
    std::string line;
    std::string lineL;                  // line in low register
    int16_t     nTmp;
    settings    prop;
    cmbxchg     *mb = (cmbxchg *)obj;
    upcon       *up = (upcon *)obj;
    std::string::size_type found;

//    cout << "parsebuff = " << fstr << " type = " << (int)type << " obj = " << obj << endl<< endl;
    while( std::getline( fstr, line ) ) {
//        cout << line.c_str() << endl;
        removeCharsFromString(line, (char *)" \t\r");
        lineL = line;
        std::transform(lineL.begin(), lineL.end(), lineL.begin(), easytolower);
//      cout << "|" << lineL.c_str() << endl;
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
//              cout << "found = " << found << endl;
                if (obj && found != std::string::npos) {
//                  cout << " commands \n";
//                    line.erase(found);
//                    line.erase(0, strlen("modbusport")+1);
//                    if(atoi(line.c_str()) == 
//                            ((cmbxchg *)obj)->m_id) {
                    if(getnumfromstr(lineL, "modbusport", "commands") == ((cmbxchg *)obj)->m_id) {
                        parseBuff(fstr, _parse_mbcmd, obj);
                    }
                }
                else 
                if (obj && (found=lineL.find("ai")) != std::string::npos){
//                  cout << " tags \n";
//                    line.erase(found);
//                    line.erase(0, strlen("modbusport")+1);
//                    if(atoi(line.c_str()) == ((cmbxchg *)obj)->m_id) {
                    if(getnumfromstr(lineL, "modbusport", "ai")== ((cmbxchg *)obj)->m_id) {
                        parseBuff(fstr, _parse_ai, obj);
                    }
                }
                else {
//                  cout << " ports \n";
                    mb = new cmbxchg();
//                    cout << "new conn " << mb << endl;
                    conn.push_back(mb);
//                    line.erase(0, strlen("modbusport")+1);
//                    line.erase(line.length()-1);
//                    mb->m_id = atoi(line.c_str());
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
        }
        else
        if(type==_parse_upcon && obj) {
            std::istringstream iss( line );
            std::string sTag;
            std::string sVal;
            if( std::getline( iss, sTag, '=') ) {
                std::getline( iss, sVal );
                up->setproperty( sTag, sVal );
            }
        }
        else
        if(type==_parse_mbport && obj) {
            std::istringstream iss( line );
            std::string sTag;
            std::string sVal;
            if( std::getline( iss, sTag, '=') ) {
                std::getline( iss, sVal );
                content oVal(sVal);
                mb->portPropertySet( sTag.c_str(), oVal );
            }
        }
        else
        if(type==_parse_mbcmd && obj) {
            std::istringstream iss( line+";" );
            std::vector<int16_t> result;
            std::string sVal;
            while( std::getline( iss, sVal, ';') ) {
                result.push_back(atoi(sVal.c_str()));
            }
            ccmd cmd(result);
            mb->mbCommandAdd(cmd);
        }
        else
        if(type==_parse_ai && obj) {
            std::string sVal;
            std::istringstream iss( line+";" );
            if(lineL.find("topic") == 0) {
                while( std::getline( iss, sVal, ';') ) {
//                  cout << sVal << " | ";
                    prop.push_back(std::make_pair(sVal, content("")));
                }
//                cout << endl;
            }
            else {
                cparam  p;
                int32_t nI=0;
                string s;
                while( std::getline( iss, sVal, ';') ) {
                    p.addproperty( prop[nI].first, sVal );
                    p.setproperty( prop[nI].first, sVal );
                    nI++;
                }
                p.p_conn = obj;
//                cout << "prop size " << p.getpropertysize() << endl;
                if(p.getproperty("name", s)==EXIT_SUCCESS) tags.push_back(make_pair(s, p));
            }
        }
    }
    return EXIT_SUCCESS;
}
//  
//	Чтение и парсинг конфигурационного файла
//	name;mqtt;type;address;
//
int16_t readCfg()
{
	int16_t     res=0;
    int16_t     nI=0, i, j;
    cmbxchg     *mb=NULL;  

//    cout << "conn size = " << conn.size() << endl;
//    cout << "readcfg" << endl;
	std::fstream filestr("map.cfg");
    parseBuff(filestr, _parse_root, (void *)mb);

//    cout << "conn size = " << conn.size() << endl;
    for(i=0; i<conn.size(); i++) {
        mb = conn[i];
//        cout << mb << " | port settings " << mb->portPropertyCount() << endl;
        for(j=0; j < mb->portPropertyCount(); ++j) {
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
    return res;
}

//
// task for writing value on parameter name
//
int16_t taskparam(std::string na, std::string va)
{
    int16_t res=EXIT_FAILURE;
    int16_t ival;
    paramlist::iterator ifi = find_if(tags.begin(), tags.end(), compareP<cparam>(na));
    if( ifi != tags.end() && isdigit(va[0]) ) {
        std::istringstream(va) >> ival;
        (*ifi).second.setvalue(ival);
        res=EXIT_SUCCESS;
    }
    return res;
}

//
// поток обработки параметров 
//
void* paramProcessing(void *args) 
{
    paramlist::iterator ih, iend;
    fieldconnections::iterator icn;    
    int16_t nRes, nRes1, nVal;
    uint8_t nQ;
    double  rVal;
    int32_t nCnt=0;
//    struct tm * ptm;
    cout << "start parameters processing " << args << endl;

    while (fParamThreadInitialized) {
        
        pthread_mutex_lock( &mutex_param );
        pthread_cond_wait( &data_ready, &mutex_param );// start processing after data receive     
        ih   = tags.begin();
        iend = tags.end();
        while ( ih != iend ) {
            if(!((*ih).second.getproperty("readdata")->_s.empty())) {
                int16_t nu;
                nRes = (*ih).second.getraw( nVal );
                nRes1= (*ih).second.getvalue( rVal, nQ );
                
                if( (*ih).second.hasnewvalue() && (nu=(*ih).second.getupcon())>=0 && nu<upc.size()) {
                    upc[nu]->publish((*ih).second);                        
                }
/*
                ptm = localtime( (*ih).second.getTS() );
                if((nCnt%int(1000000l/_param_prc_delay))==0) {
                    std::stringstream out;
                    string s; int16_t r=(*ih).second.getproperty("name", s);
                    out<<setfill(' ')<<setw(8)<<s<<" = "<< \
                    setfill(' ') << setw(7) << nVal << " | " << setfill(' ') << setw(8) << \
                    fixed << setprecision(3) << rVal << " | 0x" << hex << int16_t(nQ);
                    outtext(out.str());
                }
*/
            }
            else if( (*ih).second.taskset() ) (*ih).second.taskprocess();
            ++ih;
        }
//        if((nCnt++%int(1000000l/_param_prc_delay))==0) cout << endl;
        
        pthread_mutex_unlock( &mutex_param );
        usleep(_param_prc_delay);
    }     
    
    cout << "end parameters processing" << endl;
    
    return EXIT_SUCCESS;
}


