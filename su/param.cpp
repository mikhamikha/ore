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
//
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
        cout << line.c_str() << endl;
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
                CParam  p;
                int32_t nI=0;
                while( std::getline( iss, sVal, ';') ) {
                    p.addProperty( prop[nI]._n, sVal );
                    nI++;
                }
                p.m_conn_id = ((cmbxchg *)obj)->m_id;
                cout << "prop size " << p.getPropertySize() << endl;
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
        for(j=0; j<tags[i].getPropertySize(); j++) {
            cout << tags[i].getProperty(j)->ToText() << endl;
        }    
    }
    return res;
}
//
// поток обработки параметров 
//
void* paramProcessing(void *args) 
{
    cout << "start parameters processing " << args << endl;
    paramlist::iterator ih, ie;
    fieldconnections::iterator icn;    
    ie = tags.end();
    int16_t nRes, nVal;

    while (fParamThreadInitialized) {
        ih = tags.begin();
        while ( ih != ie) {
            icn = find_if(conn.begin(), conn.end(), equalID((*ih).m_conn_id));
            nRes = (*ih).getProperty("readdata", nVal);
            (*ih).addProperty("adc");
            if(nRes==EXIT_SUCCESS) (*ih).setProperty("adc", (*icn)->m_pReadData[nVal]);
            cout << (*ih).getProperty("name") << " = " <<  (*ih).getProperty("adc")<< endl;
            ++ih;
        }
        cout << endl;
        sleep(5);
    }     
    
    cout << "end parameters processing" << endl;
    
    return EXIT_SUCCESS;
}


