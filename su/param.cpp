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

using namespace std;

pthread_mutex_t mutex_param     = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  start_param     = PTHREAD_COND_INITIALIZER;
paramlist tags;
bool fParamThreadInitialized;

string to_string(int16_t i)
{
    std::string s;
    std::stringstream out;
    out << i;
    s = out.str();
    return s;
}

char easytolower(char in){
    if(in<='Z' && in>='A') return in-('Z'-'z');
    return in;
} 

char easytoupper(char in){
    if(in<='z' && in>='a') return in+('Z'-'z');
    return in;
} 

// example of usage:
// removeCharsFromString( str, "()-" );
void removeCharsFromString( string &str, char* charsToRemove ) {
    for ( unsigned int i = 0; i < strlen(charsToRemove); ++i ) {
        str.erase( remove(str.begin(), str.end(), charsToRemove[i]), str.end() );
    }
    std::istringstream iss( str );
    std::getline( iss, str , '#');    // remove comment
//    std::transform(str.begin(), str.end(), str.begin(), easytolower);
}

enum {
    _parse_root,
    _parse_mbport,
    _parse_mbcmd,
    _parse_ai
};

cparam::cparam()
{
    addproperty(std::string("raw"), std::string(""));
}

void cparam::addproperty(std::string na, std::string v="")
{
    cfield fld;
    fields::iterator ifi = find_if(m_prop.begin(), m_prop.end(), compProp(na));
    if(ifi == m_prop.end()) {
        fld._n = na;
        fld._v = v;
        m_prop.push_back(fld);
    }
    return;
}

int16_t cparam::getraw(int16_t &nOut)
{
    int16_t res=EXIT_FAILURE;
    cmbxchg *mb;
    int16_t nOff;

    fields::iterator ifi = find_if(m_prop.begin(), m_prop.end(), compProp("readdata"));
    if(ifi != m_prop.end()) {
        mb = (cmbxchg *)p_conn;
        nOff=(*ifi).ToInt();
        if(nOff<mb->m_maxReadData) {
            nOut = mb->m_pReadData[nOff];
            res=EXIT_SUCCESS;
        }
    }
    return res;
}

int16_t cparam::getvalue(double &rOut)
{
    int16_t res=EXIT_FAILURE;
    cmbxchg     *mb;
    int16_t     nOff, nVal;
    double      rVal;
    timespec    tv;
    int32_t     nTime;
    int64_t     nctt;
    int64_t     nD;
    int64_t     nodt;             // time on previous step

    clock_gettime(CLOCK_MONOTONIC,&tv);
    nctt = tv.tv_sec*1000000l + tv.tv_nsec*1000;
    nodt = m_ts.tv_sec*1000000l + m_ts.tv_nsec*1000;
    nD = abs(nctt-nodt);
    
    if((res = getraw(nVal))==EXIT_SUCCESS) {
        double lraw, hraw, leng, heng;
        res = getproperty("minraw", lraw)   \
            | getproperty("maxraw", hraw)   \
            | getproperty("mineng", leng)   \
            | getproperty("maxeng", heng)   \
            | getproperty("flttime", nTime);
        if(res == EXIT_SUCCESS && hraw!=lraw && heng!=leng) {
            rVal = (heng-leng)/(hraw-lraw)*(nVal-lraw)+leng;
            nTime = nTime*1000;
            if(nD) rVal = (m_dvalue*nTime+rVal*nD)/(nTime+nD); 
        }
    }
    m_dvalue = rVal;
    memcpy(&m_ts, &tv, sizeof(tv));
    rOut = rVal;
    return res;
}


cfield* cparam::getproperty(int16_t n)
{
    cfield *res=NULL;
    if(n<m_prop.size()) {
        res=&m_prop[n];
    }
    return res;
}

std::string cparam::getproperty(std::string s)
{
    std::string sOut = "no value";
    fields::iterator ifi = find_if(m_prop.begin(), m_prop.end(), compProp(s));
    if(ifi != m_prop.end()) {
        sOut=(*ifi)._v;
    }
    return sOut;
}

int16_t cparam::getproperty(std::string s, int32_t &nOut)
{
    int16_t res=EXIT_FAILURE;
    fields::iterator ifi = find_if(m_prop.begin(), m_prop.end(), compProp(s));
    if(ifi != m_prop.end()) {
        nOut=(*ifi).ToInt();
        res=EXIT_SUCCESS;
    }
    return res;
}

int16_t cparam::getproperty(std::string s, int16_t &nOut)
{
    int16_t res=EXIT_FAILURE;
    fields::iterator ifi = find_if(m_prop.begin(), m_prop.end(), compProp(s));
    if(ifi != m_prop.end()) {
        nOut=(*ifi).ToInt();
        res=EXIT_SUCCESS;
    }
    return res;
}

int16_t cparam::getproperty(std::string s, double &rOut)
{
    int16_t res=EXIT_FAILURE;
    fields::iterator ifi = find_if(m_prop.begin(), m_prop.end(), compProp(s));
    if(ifi != m_prop.end()) {
        rOut=(*ifi).ToReal();
        res=EXIT_SUCCESS;
    }
    return res;
}

int16_t cparam::setproperty(int16_t n, std::string na, std::string v)
{
    int16_t res=EXIT_FAILURE;
    if(n<m_prop.size()) {
        m_prop[n]._n = na;
        m_prop[n]._v = v;
        res=EXIT_SUCCESS;
    }
    return res;
}

int16_t cparam::setproperty(std::string s, std::string &sIn)
{
    int16_t res=EXIT_FAILURE;
    fields::iterator ifi = find_if(m_prop.begin(), m_prop.end(), compProp(s));
   if(ifi != m_prop.end()) {
        (*ifi)._v = sIn;
        res=EXIT_SUCCESS;
    }
    return res;
}

int16_t cparam::setproperty(std::string s, int16_t &nIn)
{
    int16_t res=EXIT_FAILURE;
    fields::iterator ifi = find_if(m_prop.begin(), m_prop.end(), compProp(s));
    if(ifi != m_prop.end()) {
        (*ifi)._v = to_string(nIn);
        res=EXIT_SUCCESS;
    }
    return res;
}

int16_t parseBuff(std::fstream &fstr, int8_t type, void *obj=NULL)
{    
    std::string line;
    std::string lineL;                  // line in low register
    int16_t     nTmp;
    fields      prop;
    cmbxchg     *mb = (cmbxchg *)obj;
    std::string::size_type found;

//    cout << "parsebuff = " << fstr << " type = " << (int)type << " obj = " << obj << endl;
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
            if(lineL.find("modbusport") > 0) {
                found = lineL.find("commands");
//              cout << "found = " << found << endl;
                if (obj && found != std::string::npos) {
//                  cout << " commands \n";
                    line.erase(found);
                    line.erase(0, strlen("modbusport")+1);
                    if(atoi(line.c_str()) == ((cmbxchg *)obj)->m_id) {
                        parseBuff(fstr, _parse_mbcmd, obj);
                    }
                }
                else 
                if (obj && (found=lineL.find("ai")) != std::string::npos){
//                  cout << " tags \n";
                    line.erase(found);
                    line.erase(0, strlen("modbusport")+1);
                    if(atoi(line.c_str()) == ((cmbxchg *)obj)->m_id) {
                        parseBuff(fstr, _parse_ai, obj);
                    }
                }
                else {
//                  cout << " ports \n";
                    mb = new cmbxchg();
                    cout << "new conn " << mb << endl;
                    conn.push_back(mb);
                    line.erase(0, strlen("modbusport")+1);
                    line.erase(line.length()-1);
                    mb->m_id = atoi(line.c_str());
                    parseBuff(fstr, _parse_mbport, (void *)mb);
                }
            }
        }
        else
        if(type==_parse_mbport && obj) {
            std::istringstream iss( line );
            std::string sTag;
            std::string sVal;
            if( std::getline( iss, sTag, ';') ) {
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
//            cout << line << endl;
//            cout << lineL << endl;
            if(lineL.find("topic") == 0) {
                while( std::getline( iss, sVal, ';') ) {
//                  cout << sVal << " | ";
                    cfield fld;
                    fld._n = sVal;
                    prop.push_back(fld);
                }
                cout << endl;
            }
            else {
                cparam  p;
                int32_t nI=0;
                while( std::getline( iss, sVal, ';') ) {
                    p.addproperty( prop[nI]._n, sVal );
                    nI++;
                }
                p.p_conn = obj;
                cout << "prop size " << p.getpropertysize() << endl;
                tags.push_back(p);
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
    cout << "readcfg" << endl;
	std::fstream filestr("map.cfg");
    parseBuff(filestr, _parse_root, (void *)mb);

    cout << "conn size = " << conn.size() << endl;
    for(i=0; i<conn.size(); i++) {
        mb = conn[i];
        cout << mb << " | port settings " << mb->portPropertyCount() << endl;
        for(j=0; j < mb->portPropertyCount(); ++j) {
            cout << setfill(' ') << setw(2) << j << " | " <<  mb->portProperty2Text(j) << endl;
        }
        cout << "mb commands " << mb->mbCommandsCount() << endl;
        for(j=0; j < mb->mbCommandsCount(); ++j) {
            cout << mb->mbCommand(j)->ToString() << endl;
        }
    }
//    cout << tags.size() << endl;
    for(i=0; i<tags.size(); i++) {
//        cout << tags[i].getPropertySize() << endl;
        for(j=0; j<tags[i].getpropertysize(); j++) {
            cout << tags[i].getproperty(j)->ToText() << endl;
        }    
    }
    return res;
}
//
// поток обработки параметров 
//
void* paramProcessing(void *args) 
{
    paramlist::iterator ih;
    fieldconnections::iterator icn;    
    int16_t nRes, nRes1, nVal;
    double  rVal;
    int32_t nCnt=0;
    struct tm * ptm;
    cout << "start parameters processing " << args << endl;

    while (fParamThreadInitialized) {
        
        ih = tags.begin();
        pthread_mutex_lock( &mutex_param );
        pthread_cond_wait( &start_param, &mutex_param );        
        while ( ih != tags.end()) {
            nRes = (*ih).getraw( nVal );
            nRes1= (*ih).getvalue( rVal );

            ptm = localtime( (*ih).getTS() );
            if((nCnt%10)==0) {
                    std::string s;
                    std::stringstream out;
                    out <<setfill(' ')<<setw(8)<<(*ih).getproperty("name")<<" = "<< \
                    setfill(' ') << setw(7) << nVal << " | " << \
                    setfill(' ') << setw(8) << fixed << setprecision(3) << rVal << " | " << \
                    ((nRes==0 && nRes1==0)?"OK":"FAULT");
                    outtext(out.str());
            }
            ++ih;
        }
        if((nCnt++%10)==0) cout << endl;
        
        pthread_mutex_unlock( &mutex_param );
        usleep(100000);
    }     
    
    cout << "end parameters processing" << endl;
    
    return EXIT_SUCCESS;
}


