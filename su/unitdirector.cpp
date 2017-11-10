#include "unitdirector.h"

#define _unit_prc_delay   100000
#define _ten_thou         10000

cunitdirector unitdir;
//
// Добавим блок в список
//
int16_t cunitdirector::addunit( string& s, cunit& p ) {
    int16_t rc = _exOK;
    
    if( getunit(s.c_str())==NULL ) units.push_back(make_pair(s, p)); 
    else rc = _exBadParam;
    
    return rc;        
}
//
// get unit addr by it name
//
cunit* cunitdirector::getunit( const char*  na ) {
    cunit*     rc=NULL;

//    pthread_mutex_lock( &mutex_unit );
    unitlist::iterator ifi = find_if( units.begin(), units.end(), compareP<cunit>(na) );
    if( ifi != units.end() ) {
        rc = &(ifi->second);
    }
//    pthread_mutex_unlock( &mutex_unit );

    return rc;
}
/*
//
// ask value unit by it name
//
int16_t cunitdirector::getunit( const char* na, double& va, int16_t& qual, timespec* ts, int16_t trigger=0 ) {
    int16_t     rc=EXIT_FAILURE;

    pthread_mutex_lock( &mutex_unit );
    cunit* pp = getunit( na );
    if( pp!=NULL ) {
        qual= pp->getquality();
        va = (!trigger) ? pp->getvalue() : pp->gettrigger();
        ts  = pp->getTS();
        rc=EXIT_SUCCESS;
    }
    pthread_mutex_unlock( &mutex_unit );

    return rc;
}

//
// ask value unit by it name
//
int16_t cunitdirector::getunit(const char* na, std::string& va) {
    int16_t     rc=EXIT_FAILURE;
    
    pthread_mutex_lock( &mutex_unit );
    cunit* pp = getunit( na );
    if( pp!=NULL ) {
        va = to_string( pp->getvalue() );
        if( pp->getquality() != OPC_QUALITY_GOOD ) va += " bad";
        rc=EXIT_SUCCESS;
    }
    pthread_mutex_unlock( &mutex_unit );

    return rc;
}

//
// возврат разницы между новым значением параметра и предыдущим считанным 
//
int16_t cunitdirector::getunitcount( const char* na, int16_t& val ) {
    int16_t     rc=EXIT_FAILURE;
    double      rvc, rvo;

    pthread_mutex_lock( &mutex_unit );
    cunit* pp = getunit( na );
    if( pp!=NULL && pp->getquality()==OPC_QUALITY_GOOD ) {
        rvc =  pp->getvalue();
        rvo =  pp->getoldvalue();
        pp->setoldvalue(rvc);
        val = rvc-rvo;
        rc=EXIT_SUCCESS;
    }
    pthread_mutex_unlock( &mutex_unit );

    return rc;
}

int16_t cunitdirector::getunitlimits( const char* na, double& emin, double& emax) {
    int16_t     rc=EXIT_FAILURE;
    double      rvc, rvo;

    pthread_mutex_lock( &mutex_unit );
    cunit* pp = getunit( na );
    if( pp!=NULL && pp->getquality()==OPC_QUALITY_GOOD ) {
        pp->getlimits( emin, emax );
        rc=EXIT_SUCCESS;
    }
    pthread_mutex_unlock( &mutex_unit );

    return rc;
}

//
// task for writing value on unit name
//
int16_t cunitdirector::taskunit( std::string& na, std::string& fi, std::string& va ) {
    int16_t rc=EXIT_FAILURE;
    double  rval;
    string  val(va);
    
    reduce(val, (char *)" \t\n");
    cout<<"taskunit "<<na<<" field "<<fi<<" value "<<val<<endl;
    if( (val.length() && isdigit(val[0])) || (val.length()>1 && val[0]=='-' && isdigit(val[1])) ) {
        
        cunit* pp = getunit(na.c_str());
        if( pp ) {
            pthread_mutex_lock( &mutex_unit );
            std::istringstream(val) >> rval;
            if( fi.find("task") != std::string::npos ) pp->settask( rval );
            else
            if( fi=="value" ) pp->setvalue( rval );
            else pp->setproperty(fi, rval);
            rc=EXIT_SUCCESS;
            pthread_mutex_unlock( &mutex_unit );
        }
    }
    return rc;
}

//
// task for writing value on unit full name 
//
int16_t cunitdirector::taskunit( std::string& na, std::string& va ) {
    int16_t rc=EXIT_FAILURE;
    double  rval;
    string  val(va);
    
    reduce(val, (char *)" \t\n");
    cout<<"taskunit "<<na<<" value "<<val;
            
    std::vector<string> vc;
    std::string sunit, sf;
    strsplit( na, '/', vc);
    if( (val.length() && isdigit(val[0])) || (val.length()>1 && val[0]=='-' && isdigit(val[1]))  
            && vc.size() > 3 ) {
        sf   = vc.back(); vc.pop_back();
        sunit = vc.back(); vc.pop_back();
        cout<<" name "<<sunit<<" field "<<sf<< " value "<<val<<" size "<<vc.size()<<endl;
            
        cunit* pp = getunit(sunit.c_str());
        if( pp ) {
            pthread_mutex_lock( &mutex_unit );
            std::istringstream(val) >> rval;
            if( sf.find("task") != std::string::npos ) pp->settask( rval );
            else
            if( sf=="value" ) pp->setvalue( rval );
            else pp->setproperty(sf, rval);
            rc=EXIT_SUCCESS;
            pthread_mutex_unlock( &mutex_unit );
        }
    }
    return rc;
}
*/
//
// поток обработки блоков 
//
void cunitdirector::run() {
    unitlist::iterator ih, iend;
    
    cout << "start units processing " << endl;
    sleep(3);
    
    while ( ftagThreadInitialized ) {
        ih   = units.begin();
        iend = units.end();
        while ( ih != iend ) {
            cunit& pun = ih->second;
            
            ++ih;
        }
        usleep(_unit_prc_delay);
    }     
    
    cout << "end units processing" << endl;
    
//    return EXIT_SUCCESS;
}
//
// получить ссылку на юнит по имени
// 
cunit* getaddrunit(string& str) {
    cunit* p = unitdir.getunit( str.c_str() );
    cout<<"getunit "<<hex<<long(p);
    if(p) cout<<" name="<< p->getname();
    cout<<endl;

    return p;
}

