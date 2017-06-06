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

#define _param_prc_delay    100000
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
    setproperty( string("raw"),         double(0)  );
    setproperty( string("value"),       double(0)  );
    setproperty( string("task"),        double(0)  );
    setproperty( string("quality"),     int32_t(0) );
    setproperty( string("timestamp"),   string("") );  
    setproperty( string("deadband"),    double(0)  );
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

void cparam::init() {
    string sOff; 
//    cmbxchg     *mb = (cmbxchg *)p_conn;

    if( getproperty("readdata", sOff)==0 ) {
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
    if( getproperty("writedata", sOff)==0 ) {
        if( !sOff.empty() && isdigit(sOff[0]) ) m_writeOff = atoi(sOff.c_str());
        else /*if(sOff[0]=='E')*/ m_writeOff = -2;
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
    getproperty( "mindev",  m_minDev   );
    getproperty( "maxdev",  m_maxDev   );

    int16_t nPortErrOff=0, nErrOff=0;
    
    cout<<"param::init "<<m_name<<" rc="<<rc<<" bool? "<<m_isBool<<" deadband "<<m_deadband<< \
        " maxE "<<m_maxEng<<" minE "<<m_minEng<<" maxR "<<m_maxRaw<<" minR "<<m_minRaw<< \
        " hihi "<<m_hihi<<" hi "<<m_hi<<" lolo "<<m_lolo<<" lo "<<m_lo<<endl;
    
    if( (m_readOff >= 0 || m_writeOff >= 0) && /*mb->getproperty("commanderror", nPortErrOff) == EXIT_SUCCESS &&*/ \
                getproperty("ErrPtr", nErrOff) == EXIT_SUCCESS ) {     // read errors of read modbus operations
         if( (nErrOff + nPortErrOff) < cmbxchg::m_maxReadData ) m_connErr = nErrOff + nPortErrOff;
    }

    if( m_readOff == -2 ) {
        m_quality = OPC_QUALITY_GOOD;
        if( m_name.find("LV") == 0 ) {              // if parameter (valve position mm) must be evaluate from other
            string  s1;
            int16_t num;
            
            num = atoi( m_name.substr(2, 2).c_str() );
            s1 = "FV"+to_string(num);
            m_pos = getparam( s1.c_str() );         // valve position in percent
            cout<<"init "<<m_name<<" pos="<<hex<<long(m_pos)<<dec<<endl;
        }
        if( m_name.find("FV") == 0 ) {
            setproperty( "configured", 1 );
            setproperty( "task_delta", double(1) );            
            setproperty( "max_task_delta", double(10) );           
            if( m_name.size()>3 && isdigit(m_name[2]) ) setproperty( "valve", m_name.substr(2,1).c_str() ); 
        }
    }
    // подписка на команды
    int16_t nu;
    if( (nu=getsubcon())>=0 && nu<upc.size() ) {
        upc[nu]->subscribe( *this );
//      pp.m_sub = nu;
    }
}

int16_t cparam::getraw() {
    int16_t     rc  = EXIT_SUCCESS;
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
/*
        if(m_name.find("FV") == 0 ) {
            rc =rawValveValueEvaluate();
            setproperty("raw", m_raw);    
//            rc=EXIT_SUCCESS;
        }
        else
*/
        if(m_name.find("LV") == 0 ) {
            setrawval( m_pos->getvalue() );
            setrawscale( m_pos->m_minEng, m_pos->m_maxEng );
            m_quality = m_pos->m_quality;
//            rc=EXIT_SUCCESS;
        }
/*
        else if(m_name.find("FT") == 0 ) {
//            rc =flowEvaluate();
//            setproperty("raw", m_raw);    
//            rc=EXIT_SUCCESS;
        }
*/
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

//    if( m_name.substr(0,4)=="PT31")  cout<<to_text()<<endl;
    if( (rc = getproperty("simen", rsim_en) | \
            getproperty("simva", rsim_v)) == EXIT_SUCCESS && rsim_en != 0 ) {   // simulation mode switched ON 
        m_quality_old = m_quality;
        rVal  = rsim_v;
        rOut  = rsim_v;
        m_quality = OPC_QUALITY_GOOD;
        rc = EXIT_SUCCESS;
    }
    else { 
//       if( m_name.find("FV") ==std::string::npos ) 
           rc = getraw();
    }

    if( rc==EXIT_SUCCESS ) {
        if( rsim_en == 0 ) {                                                        // simulation mode switched OFF
            if( m_maxRaw!=m_minRaw && m_maxEng!=m_minEng ) {
                rVal = (m_maxEng-m_minEng)/(m_maxRaw-m_minRaw)*(m_raw-m_minRaw)+m_minEng;
                rVal = min( rVal, m_maxEng );                                       // ограничим значение инженерной шкалой
                rVal = max( rVal, m_minEng );
/*                
                if( m_minDev!=m_maxDev )  {                                         // если задана шкала параметра < шкалы прибора,
                    rVal = min( rVal, m_maxDev );                                   // ограничим значение инженерной шкалой
                    rVal = max( rVal, m_minDev );
                }
*/                
                nTime = m_fltTime*1000;
                if( nD && nTime ) rVal = (m_rvalue*nTime+rVal*nD)/(nTime+nD); 
                if(m_isBool==1) rVal = (rVal >= m_hihi);                            // if it is a discret parameter
                if(m_isBool==2) rVal = (rVal < m_hihi);                             // if it is a discret parameter & inverse
                rOut = rVal;                                                        // current value
            }
        }
      
//        if( m_name.substr(0,4)=="FV11") \
            cout <<"getvalue name="<<m_name<<" TS= "<< nodt << "|" << nctt << " dT= " << nD <<" readOff="<<m_readOff \
                <<" val= "<<dec<<m_rvalue_old<<"|"<<m_rvalue<<"|"<<rVal \
                <<" raw= "<<m_raw_old<<"|"<<m_raw<<" dead= "<<m_deadband<<" engSc= "<<m_minEng<<"|"<<m_maxEng \
                <<" rawSc= "<<m_minRaw<<"|"<<m_maxRaw<<" hihi= "<<m_hihi<<" mConnErrOff= "<<m_connErr \
                <<" q= "<<int(m_quality_old)<<"|"<<int(m_quality)<<endl;

        //      save value if it (or quality) was changes
        if( fabs(rVal-m_rvalue)>=m_deadband || m_quality_old != m_quality || nD>60*_million ) {
            m_ts.tv_sec = tv.tv_sec;
            m_ts.tv_nsec = tv.tv_nsec;
            m_valueupdated = true;
            setproperty("value", rVal);
            setproperty("quality", m_quality);
            setproperty("sec",  int32_t(tv.tv_sec));
            setproperty("msec", int32_t(tv.tv_nsec/_million));
/*          if( m_name.substr(0,3)=="FV1") cout<<endl;
                cout <<" |v "<< rVal<<" |vold "<<m_rvalue<< \
                " |d "<<m_deadband<<" | mConnErrOff "<<m_connErr<<" |q "<<int(nQual)<<" |dt "<<nD/_million<<endl;
*/
            m_rvalue = rVal;
        } 
//        if(m_firstscan) { m_rvalue_old = m_rvalue; m_firstscan = false; }
    }    
    if(m_firstscan) { m_rvalue_old = m_rvalue; m_firstscan = false; }
   
    return rc;
}

int16_t cparam::setvalue( double rin=0 ) {
    int16_t rc = EXIT_FAILURE;
//    cmbxchg *mb;
    string  sOff;
    int16_t nOff;
    
    /*
    if( m_writeOff >= 0 ) {
//      if( m_name.substr(0,3)=="FC1")
//            cout<<"setvalue "<<m_name<<" task "<<m_task<<" off="<<m_writeOff<<" conn="<<int(mb)<<endl;      
        cmbxchg::m_pWriteData[m_writeOff] = m_task;
        cmbxchg::m_pLastWriteData[m_writeOff] = m_task-1;
        m_task_go = false; 
        rc = EXIT_SUCCESS;
    }
    else 
    */    
    if( m_writeOff == -2 ) {
//        if( m_name.substr(0,4)=="FV11") cout << "? setvalue really raw = "<< rin <<endl;   
        m_raw = rin;
        m_quality = OPC_QUALITY_GOOD;
    }
    
    return rc;
}

int16_t cparam::settask(double rin, bool fgo) {
    int16_t rc = EXIT_SUCCESS;
    
    if( m_maxRaw-m_minRaw!=0 && m_maxEng-m_minEng!=0 ) {
        double rVal = rin;
        rVal = (m_maxRaw-m_minRaw)/(m_maxEng-m_minEng)*(rVal-m_minEng)+m_minRaw;
        m_task    = rVal;  
    }
    else m_task = rin;
//        if( m_name.substr(0,3)=="FC1")
        cout<<endl<<"settask "<<m_name<<" task=="<<rin<<" -> "<<m_task<<endl; 
    m_task_go = fgo; 
    
    if( m_writeOff >= 0 ) {
//      if( m_name.substr(0,3)=="FC1")
//            cout<<"setvalue "<<m_name<<" task "<<m_task<<" off="<<m_writeOff<<" conn="<<int(mb)<<endl;   
        pthread_mutex_lock( &mutex_param );
        cmbxchg::m_pWriteData[m_writeOff] = m_task;
        cmbxchg::m_pLastWriteData[m_writeOff] = m_task-1;
        pthread_mutex_unlock( &mutex_param );
        m_task_go = false; 
    }
    setproperty( "task", rin );
    return rc;
}


//  
//	Чтение и парсинг конфигурационного файла
//
int16_t readCfg() {
	int16_t     rc = EXIT_FAILURE;
    int16_t     nI=0, i, j;
    cmbxchg     *mb = NULL;  
    upcon       *up = NULL;
    calgo       *alg= NULL;    
    
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

        // парсим алгоритмы
        int16_t cnt=0;
        tools = doc.select_nodes("//algo/alg");
        for(pugi::xpath_node_set::const_iterator it = tools.begin(); it != tools.end(); ++it) {
            alg = new calgo();
            algos.push_back(alg);

            cout<<"parse alg "<<++cnt<<endl;
           
            for (pugi::xml_attribute attr = it->node().first_attribute(); attr; attr = attr.next_attribute()) {
                spar = attr.name();
                sval = attr.value();
                alg->setproperty( spar, sval );
                cout<<" "<<spar<<"="<<sval;
            } 
            
            for(pugi::xml_node tool = it->node().first_child(); tool; tool = tool.next_sibling()) {        
                alg->setproperty( tool.name(), tool.text().get() );
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
            else
            if( sf=="value" ) ifi->second.setvalue( rval );
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
    alglist::iterator iah, iaend;   
    int16_t nRes, nRes1, nVal;
    uint8_t nQ;
    double  rVal;
    int32_t nCnt=0;

    cout << "start parameters processing " << args << endl;
    sleep(1);
    cton tt;
    while ( fParamThreadInitialized ) {
//        cout<<"T1 = "<<tt.getTT();
//        pthread_mutex_lock( &mutex_param );
//        cout<<" T2 = "<<tt.getTT();
       
//        pthread_cond_wait( &data_ready, &mutex_param );// start processing after data receive     
        ih   = tags.begin();
        iend = tags.end();
        while ( ih != iend ) {
            string  sOff;
            int16_t nu;
            cparam  &pp = ih->second;
            
            nRes1= pp.getvalue( rVal );

            // публикация свежих данных
            if( pp.hasnewvalue() && (nu=pp.getpubcon())>=0 && nu<upc.size() ) {
                publish(pp);                        
            }

            // выдача задания на модули вв
            pthread_mutex_lock( &mutex_param );
            if( pp.getquality()==OPC_QUALITY_GOOD ) {
//                if( pp.taskset() ) pp.setvalue();
                if( pp.isbool() && pp.m_tasktimer.isDone() ) {                  // если было импульсное задание и вышел таймер
                    pp.settask( !pp.gettask() );                                // инверитруем выход
                    pp.m_tasktimer.reset();
                }
            }
            pthread_mutex_unlock( &mutex_param );
            ++ih;
        }
//        cout<<" T3 = "<<tt.getTT();
        iah   = algos.begin();
        iaend = algos.end();
        while ( iah != iaend ) {
            calgo* p = *iah;
            if(p) p->solveIt();
            ++iah;
        }
//        cout<<" T4 = "<<tt.getTT();
//        pthread_mutex_unlock( &mutex_param );
//        cout<<" T5 = "<<tt.getTT()<<endl;
        tt.start(1000);
        usleep(_param_prc_delay);
    }     
    
    cout << "end parameters processing" << endl;
    
    return EXIT_SUCCESS;
}


