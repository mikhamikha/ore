#include <pugixml.hpp>
#include "utils.h"
#include "upcon.h"
#include "unitdirector.h"
#include "tagdirector.h"
#include "mbxchg.h"
#include "display.h"
#include "algo.h"

using namespace std;

int32_t dT = 0;


int32_t getnumfromstr(std::string in, std::string st, std::string fin) {
    string line = in;
    int32_t res=-1;
    line.erase(0, line.find(st)+st.length());
    line.erase(line.find(fin));
    if(isdigit(line[0])) res = atoi(line.c_str());
    return res;
}

void setDT() {
    timespec    rawtime;
//    struct tm   *ptm1;
    time_t      gt;

    clock_gettime(CLOCK_MONOTONIC, &rawtime);
   
    time(&gt);
    dT = int32_t(gt-rawtime.tv_sec);
    cout << " dT = " << dT << endl;
}

string to_string(int32_t i) {
    string s;
    stringstream out;
    out << i;
    s = out.str();
    return s;
}

string to_string(double i) {
    string s;
    stringstream out;
    out <<  i;
    s = out.str();
//    cout << "tostr "<<i<<" | "<<s<<endl;
    return s;
}

char easytolower(char in) {
    if(in<='Z' && in>='A') return in-('Z'-'z');
    return in;
} 

char easytoupper(char in){
    if(in<='z' && in>='a') return in+('Z'-'z');
    return in;
} 

//
//  Removing leading and trailing spaces from a string
//
std::string trim(const std::string& str,
                         const std::string& whitespace = " \t") {

    std::size_t strBegin = str.find_first_not_of(whitespace);
    if (strBegin == std::string::npos) return ""; // no content

    std::size_t strEnd = str.find_last_not_of(whitespace);
    std::size_t strRange = strEnd - strBegin + 1;

    return str.substr(strBegin, strRange);
}

// Удаление ненужных символов из строки
// example of usage:
// reduce( str, "()-" );
//
void reduce( string &str, char* charsToRemove ) {
    for ( unsigned int i = 0; i < strlen(charsToRemove); ++i ) {
        str.erase( remove(str.begin(), str.end(), charsToRemove[i]), str.end() );
    }
    str = str.substr( 0, str.find('#') );
}

void outtext(std::string tx) {
  time_t      rawtime;
//    timespec    rawtime;
    struct tm   *ptm;
    
      time(&rawtime);
//    clock_gettime(CLOCK_MONOTONIC, &rawtime);
//    ptm = localtime(&(rawtime.tv_sec));
    ptm = localtime(&rawtime);
    cout<<dec<<
        setfill('0')<<setw(2)<<ptm->tm_hour<<":"<<
        setfill('0')<<setw(2)<<ptm->tm_min<<":"<< 
        setfill('0')<<setw(2)<<ptm->tm_sec<<"  "<<tx<<endl;
}

string time2string(time_t rawtime) {
    stringstream s;
    struct tm   *ptm;
    int32_t     dt = int32_t(rawtime);//+dT;

    ptm = localtime((time_t *)&dt);
 
    s <<    setfill('0')<<setw(4)<<ptm->tm_year+1900<<"/"<<  \
            setfill('0')<<setw(2)<<ptm->tm_mon+1<<"/"<<   \
            setfill('0')<<setw(2)<<ptm->tm_mday<<" "<< \
/*
    s <<    setfill('0')<<setw(2)<<ptm->tm_mday<<"."<< \
            setfill('0')<<setw(2)<<ptm->tm_mon+1<<"."<<   \
            setfill('0')<<setw(4)<<ptm->tm_year+1900<<" "<<  \
*/
            setfill('0')<<setw(2)<<ptm->tm_hour<<":"<<  \
            setfill('0')<<setw(2)<<ptm->tm_min<<":"<<   \
            setfill('0')<<setw(2)<<ptm->tm_sec;

    return s.str();
}
//
//  заменяет в строке сабджект все вхождения search на replace
//
int16_t replaceString(string& subject, const string& search, const string& replace) {
    size_t  pos = 0;
    int16_t rc=EXIT_SUCCESS;

    while((pos = subject.find(search, pos)) != std::string::npos) {
        subject.replace(pos, search.length(), replace);
        pos += replace.length();
    }
    return rc;
}
//
//  разбивает строку s на подстроки, используя разделитель delim, результат в вектор vec
int16_t strsplit(string& s, char delim, vector<string>& vec) {
    int16_t count = 0;

    vec.erase( vec.begin(), vec.end() );
    std::istringstream iss( s+delim );
    std::string sval;
    while( std::getline( iss, sval, delim) ) {
        vec.push_back(sval);
        count++;
    }
    return count;
}

//  
//	Чтение и парсинг конфигурационного файла
//
int16_t readCfg() {
	int16_t     rc = _exFail;
    cmbxchg     *mb = NULL;  
    upcon       *up = NULL;
    
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
                        std::vector<int32_t> result;
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
                    string _n = tool.name();
                    string _v = tool.text().get();
                    mb->setproperty( _n, _v );
                }
            }
        }
        string spar, sval;
        // парсим тэги
        tools = doc.select_nodes("//tags/tag");
        for(pugi::xpath_node_set::const_iterator it = tools.begin(); it != tools.end(); ++it) {
            ctag  p;
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
            
            tagdir.addtag( s, p );
        }

        // парсим соединения наверх
        tools = doc.select_nodes("//uplinks/up");
        for(pugi::xpath_node_set::const_iterator it = tools.begin(); it != tools.end(); ++it) {
            up = new upcon();
            upc.push_back(up);
            up->m_id = atoi(it->node().attribute("num").value());   
            
            cout<<"parse upcon num="<<up->m_id<<endl;
            for(pugi::xml_node tool = it->node().first_child(); tool; tool = tool.next_sibling()) {        
                string _n = tool.name();
                string _v = tool.text().get();
                up->setproperty( _n, _v );
                cout<<" "<<tool.name()<<"="<<tool.text().get();   
            }
            cout<<endl;   
        }

        // парсим объекты
        tools = doc.select_nodes("//units/unit");
        for(pugi::xpath_node_set::const_iterator it = tools.begin(); it != tools.end(); ++it) {
            cunit uni;

            cout<<"parse units "<<endl;
           
            for (pugi::xml_attribute attr = it->node().first_attribute(); attr; attr = attr.next_attribute()) {
                spar = attr.name();
                sval = attr.value();
                uni.setproperty( spar, sval );
                cout<<" "<<spar<<"="<<sval;
            } 
            
            for(pugi::xml_node tool = it->node().first_child(); tool; tool = tool.next_sibling()) {        
                string _n = tool.name();
                string _v = tool.text().get();
                uni.setproperty( _n, _v );
                cout<<" "<<tool.name()<<"="<<tool.text().get();   
            }
            string s;
            if( uni.getproperty( "name", s )==_exOK && !s.empty() ) unitdir.addunit( s, uni );
            cout<<"\nuni name="<<s<<" size="<<unitdir.size()<<endl; 
        }
       // парсим алгоритмы
        int16_t cnt=0;
        tools = doc.select_nodes("//algo/alg");
        for(pugi::xpath_node_set::const_iterator it = tools.begin(); it != tools.end(); ++it) {
            calgo* alg = new calgo();

            cout<<"parse alg "<<++cnt<<endl;
           
            for (pugi::xml_attribute attr = it->node().first_attribute(); attr; attr = attr.next_attribute()) {
                spar = attr.name();
                sval = attr.value();
                alg->setproperty( spar, sval );
                cout<<" "<<spar<<"="<<sval<<" size="<<alg->getpropertysize();
            } 
            
            for(pugi::xml_node tool = it->node().first_child(); tool; tool = tool.next_sibling()) {        
                string _n = tool.name();
                string _v = tool.text().get();
                alg->setproperty( _n, _v );
                cout<<" "<<tool.name()<<"="<<tool.text().get()<<" size="<<alg->getpropertysize();   
            }
            cout<<endl;   
            algos.push_back(alg);
        }
        //
        // парсим описания дисплеев
        pugi::xpath_node tool = doc.select_node("//displays[@sleep]");
        cout<<"parse disp settings"<<endl;
        if(tool) {
            int32_t t=atoi(tool.node().attribute("sleep").value());
            if(t) dsp.setSleepWait(t*1000);
            string s=tool.node().attribute("unlock").value();
            if(s.length()>0) dsp.setUnlockCode(s);
            cout<<"sleep = "<<t<<" sec. unlock key = "<<s<<endl;
        }
        
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
        
        // парсим режимы клапана
        tools = doc.select_nodes("//valve/mode");
        for(pugi::xpath_node_set::const_iterator it = tools.begin(); it != tools.end(); ++it) {
            cproperties<content> prp;

            cout<<"parse valve mode "<<endl;
           
            for (pugi::xml_attribute attr = it->node().first_attribute(); attr; attr = attr.next_attribute()) {
                spar = attr.name();
                sval = attr.value();
                prp.setproperty( spar, sval );
                cout<<" "<<spar<<"="<<sval;
            } 
            cout<<endl;   
            vmodes.push_back(prp);
        }
         // парсим статусы клапана
        tools = doc.select_nodes("//valve/status");
        for(pugi::xpath_node_set::const_iterator it = tools.begin(); it != tools.end(); ++it) {
            cproperties<content> prp;

            cout<<"parse status mode "<<endl;
           
            for (pugi::xml_attribute attr = it->node().first_attribute(); attr; attr = attr.next_attribute()) {
                spar = attr.name();
                sval = attr.value();
                prp.setproperty( spar, sval );
                cout<<" "<<spar<<"="<<sval;
            } 
            cout<<endl;   
            vstatuses.push_back(prp);
        }
      
        rc = _exOK;
    }
    else {
        cout << "Cfg load error: " << result.description() << endl;
    }
    
    return rc;
}

// проверка на NULL
bool testaddr(void* x) {
    return (x==NULL);
}



