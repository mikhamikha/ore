#include "main.h"
//#include "tag.h"
//#include "mbxchg.h"

#include <fstream>	
#include <sstream>	
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <string>
#include <string.h>
#include <stdlib.h>

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

void ctag::init() {
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
    
    cout<<"tag::init "<<m_name<<" rc="<<rc<<" bool? "<<m_isBool<<" deadband "<<m_deadband<< \
        " maxE "<<m_maxEng<<" minE "<<m_minEng<<" maxR "<<m_maxRaw<<" minR "<<m_minRaw<< \
        " hihi "<<m_hihi<<" hi "<<m_hi<<" lolo "<<m_lolo<<" lo "<<m_lo<<endl;
    
    if( (m_readOff >= 0 || m_writeOff >= 0) && /*mb->getproperty("commanderror", nPortErrOff) == EXIT_SUCCESS &&*/ \
                getproperty("ErrPtr", nErrOff) == EXIT_SUCCESS ) {     // read errors of read modbus operations
         if( (nErrOff + nPortErrOff) < cmbxchg::m_maxReadData ) m_connErr = nErrOff + nPortErrOff;
    }

    if( m_readOff == -2 ) {
        m_quality = OPC_QUALITY_GOOD;
        if( m_name.find("LV") == 0 ) {              // if tageter (valve position mm) must be evaluate from other
            string  s1;
            int16_t num;
            
            num = atoi( m_name.substr(2, 2).c_str() );
            s1 = "FV"+to_string(num);
            m_pos = tagdir.gettag( s1.c_str() );         // valve position in percent
            cout<<"init "<<m_name<<" pos="<<hex<<long(m_pos)<<dec<<endl;
        }
        if( m_name.find("FV") == 0 ) {
            setproperty( "configured", 1 );
            setproperty( "task_delta", double(1) );            
            setproperty( "max_task_delta", double(10) );           
            if( m_name.size()>3 && isdigit(m_name[2]) ) setproperty( "valve", m_name.substr(2,1).c_str() ); 
/*            double d;
            getproperty( "kp", d );
            cout << "Init tag "<< m_name<< " kp = " << d << endl; */
        }
    }
    // подписка на команды
    int16_t nu;
    if( (nu=getsubcon())>=0 && nu<upc.size() ) {
        upc[nu]->subscribe( *this );
//      pp.m_sub = nu;
    }
}

int16_t ctag::getraw() {
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
int16_t ctag::getvalue(double &rOut) {
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
                if(m_isBool==1) rVal = (rVal >= m_hihi);                            // if it is a discret tageter
                if(m_isBool==2) rVal = (rVal < m_hihi);                             // if it is a discret tageter & inverse
                rOut = rVal;                                                        // current value
            }
            else {
                rOut = m_raw;
                rVal = m_raw;
            }
        }
      
//        if( m_name.substr(0,4)=="MV11") \
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
/*          if( m_name.substr(0,3)=="MV1") cout<<endl;
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

int16_t ctag::setvalue( double rin=0 ) {
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
        settask( rin, false );
    }
    
    return rc;
}

int16_t ctag::settask(double rin, bool fgo) {
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
    
    if( m_writeOff >= 0 ) {                                     // если это modbus
//      if( m_name.substr(0,3)=="FC1")
//            cout<<"setvalue "<<m_name<<" task "<<m_task<<" off="<<m_writeOff<<" conn="<<int(mb)<<endl;   
        pthread_mutex_lock( &mutex_tag );
        cmbxchg::m_pWriteData[m_writeOff] = int(m_task)&USHRT_MAX;
        cmbxchg::m_pLastWriteData[m_writeOff] = cmbxchg::m_pWriteData[m_writeOff]-1;
        pthread_mutex_unlock( &mutex_tag );
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
            ctag  p;
            string  s = it->node().text().get();
            cout<<"Tag "<<s;
            for (pugi::xml_attribute attr = it->node().first_attribute(); attr; attr = attr.next_attribute()) {
                spar = attr.name();
                sval = attr.value();
                p.setproperty( spar, sval );
//                double d;
//                p.getproperty( spar, d );
                cout<<" "<<spar<<"="<<sval;
            } 
            cout<<endl;
            p.setproperty("name", s);
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
        
        // парсим режимы клапана
        tools = doc.select_nodes("//valve/mode");
        for(pugi::xpath_node_set::const_iterator it = tools.begin(); it != tools.end(); ++it) {
            cproperties prp;

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
       
        rc = EXIT_SUCCESS;
    }
    else {
        cout << "Cfg load error: " << result.description() << endl;
    }
    
    return rc;
}
// 
// получить текстовое описание по значению
//
void ctag::getdescvalue( string& name, string& sval )
{
    int16_t ival;
    getproperty( name, ival );
    if( m_name.substr(0,2)=="MV" && ival < vmodes.size() ) vmodes[ival].getproperty( "display", sval );
}


