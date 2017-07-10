#include "main.h"

#define _tag_prc_delay    100000
#define _ten_thou         10000

ctagdirector tagdir;


//
<<<<<<< 65512ba9996f76309fff3db429f77239cb6c5d13
// ask tageter addr by it name
=======
// get tag addr by it name
>>>>>>> 25. a)pthread_create into class method run() b)PID algoritm complete
//
ctag* ctagdirector::gettag( const char*  na ) {
    ctag*     rc=NULL;

//    pthread_mutex_lock( &mutex_tag );
    taglist::iterator ifi = find_if( tags.begin(), tags.end(), compareP<ctag>(na) );
    if( ifi != tags.end() ) {
        rc = &(ifi->second);
    }
//    pthread_mutex_unlock( &mutex_tag );
<<<<<<< 65512ba9996f76309fff3db429f77239cb6c5d13
=======
/*
>>>>>>> 25. a)pthread_create into class method run() b)PID algoritm complete
    if(!rc) {
        cout<<"gettag() не нашел параметр "<<na<<". Завершаю работу..."<<endl;
        exit(0);
    }
<<<<<<< 65512ba9996f76309fff3db429f77239cb6c5d13
=======
*/
>>>>>>> 25. a)pthread_create into class method run() b)PID algoritm complete
    return rc;
}

//
<<<<<<< 65512ba9996f76309fff3db429f77239cb6c5d13
// ask value tageter by it name
=======
// ask value tag by it name
>>>>>>> 25. a)pthread_create into class method run() b)PID algoritm complete
//
int16_t ctagdirector::gettag( const char* na, double& va, int16_t& qual, timespec* ts, int16_t trigger=0 ) {
    int16_t     rc=EXIT_FAILURE;

    pthread_mutex_lock( &mutex_tag );
    ctag* pp = gettag( na );
    if( pp!=NULL ) {
        qual= pp->getquality();
        va = (!trigger) ? pp->getvalue() : pp->gettrigger();
        ts  = pp->getTS();
        rc=EXIT_SUCCESS;
    }
    pthread_mutex_unlock( &mutex_tag );

    return rc;
}

//
<<<<<<< 65512ba9996f76309fff3db429f77239cb6c5d13
// ask value tageter by it name
=======
// ask value tag by it name
>>>>>>> 25. a)pthread_create into class method run() b)PID algoritm complete
//
int16_t ctagdirector::gettag(const char* na, std::string& va) {
    int16_t     rc=EXIT_FAILURE;
    
    pthread_mutex_lock( &mutex_tag );
    ctag* pp = gettag( na );
    if( pp!=NULL ) {
        va = to_string( pp->getvalue() );
        if( pp->getquality() != OPC_QUALITY_GOOD ) va += " bad";
        rc=EXIT_SUCCESS;
    }
    pthread_mutex_unlock( &mutex_tag );

    return rc;
}

//
// возврат разницы между новым значением параметра и предыдущим считанным 
//
int16_t ctagdirector::gettagcount( const char* na, int16_t& val ) {
    int16_t     rc=EXIT_FAILURE;
    double      rvc, rvo;

    pthread_mutex_lock( &mutex_tag );
    ctag* pp = gettag( na );
    if( pp!=NULL && pp->getquality()==OPC_QUALITY_GOOD ) {
        rvc =  pp->getvalue();
        rvo =  pp->getoldvalue();
        pp->setoldvalue(rvc);
        val = rvc-rvo;
        rc=EXIT_SUCCESS;
    }
    pthread_mutex_unlock( &mutex_tag );

    return rc;
}

int16_t ctagdirector::gettaglimits( const char* na, double& emin, double& emax) {
    int16_t     rc=EXIT_FAILURE;
    double      rvc, rvo;

    pthread_mutex_lock( &mutex_tag );
    ctag* pp = gettag( na );
    if( pp!=NULL && pp->getquality()==OPC_QUALITY_GOOD ) {
        pp->getlimits( emin, emax );
        rc=EXIT_SUCCESS;
    }
    pthread_mutex_unlock( &mutex_tag );

    return rc;
}

//
// task for writing value on tag name
//
int16_t ctagdirector::tasktag( std::string& na, std::string& fi, std::string& va ) {
    int16_t rc=EXIT_FAILURE;
    double  rval;
    string  val(va);
    
    reduce(val, (char *)" \t\n");
    cout<<"tasktag "<<na<<" field "<<fi<<" value "<<val<<endl;
    if( (val.length() && isdigit(val[0])) || (val.length()>1 && val[0]=='-' && isdigit(val[1])) ) {
        
        ctag* pp = gettag(na.c_str());
        if( pp ) {
            pthread_mutex_lock( &mutex_tag );
            std::istringstream(val) >> rval;
            if( fi.find("task") != std::string::npos ) pp->settask( rval );
            else
            if( fi=="value" ) pp->setvalue( rval );
            else pp->setproperty(fi, rval);
            rc=EXIT_SUCCESS;
            pthread_mutex_unlock( &mutex_tag );
        }
    }
    return rc;
}

//
// task for writing value on tag full name 
//
int16_t ctagdirector::tasktag( std::string& na, std::string& va ) {
    int16_t rc=EXIT_FAILURE;
    double  rval;
    string  val(va);
    
    reduce(val, (char *)" \t\n");
    cout<<"tasktag "<<na<<" value "<<val;
            
    std::vector<string> vc;
    std::string stag, sf;
    strsplit( na, '/', vc);
    if( (val.length() && isdigit(val[0])) || (val.length()>1 && val[0]=='-' && isdigit(val[1]))  
            && vc.size() > 3 ) {
        sf   = vc.back(); vc.pop_back();
        stag = vc.back(); vc.pop_back();
        cout<<" name "<<stag<<" field "<<sf<< " value "<<val<<" size "<<vc.size()<<endl;
            
        ctag* pp = gettag(stag.c_str());
        if( pp ) {
            pthread_mutex_lock( &mutex_tag );
            std::istringstream(val) >> rval;
            if( sf.find("task") != std::string::npos ) pp->settask( rval );
            else
            if( sf=="value" ) pp->setvalue( rval );
            else pp->setproperty(sf, rval);
            rc=EXIT_SUCCESS;
            pthread_mutex_unlock( &mutex_tag );
        }
    }
    return rc;
}

//
// поток обработки параметров 
//
void ctagdirector::run() {
    taglist::iterator ih, iend;
    alglist::iterator iah, iaend;   
    int16_t nRes, nRes1, nVal;
    uint8_t nQ;
    double  rVal;
    int32_t nCnt=0;

    cout << "start tags processing " << endl;
    sleep(1);
    cton tt;
    while ( ftagThreadInitialized ) {
<<<<<<< 65512ba9996f76309fff3db429f77239cb6c5d13
/*        cout<<"T1 = "<<tt.getTT();
        pthread_mutex_lock( &mutex_tag );
        cout<<" T2 = "<<tt.getTT();
        pthread_cond_wait( &data_ready, &mutex_tag );// start processing after data receive     
*/
=======
>>>>>>> 25. a)pthread_create into class method run() b)PID algoritm complete
        ih   = tags.begin();
        iend = tags.end();
        while ( ih != iend ) {
            string  sOff;
            int16_t nu;
<<<<<<< 65512ba9996f76309fff3db429f77239cb6c5d13
            ctag  &pp = ih->second;
            
            nRes1= pp.getvalue( rVal );
=======
            ctag&   pp = ih->second;
            
            nRes1 = pp.getvalue( rVal );                        // вычислим значение параметра
>>>>>>> 25. a)pthread_create into class method run() b)PID algoritm complete

            // публикация свежих данных
            if( pp.hasnewvalue() && (nu=pp.getpubcon())>=0 && nu<upc.size() ) {
                publish(pp);                        
            }

            // выдача задания на модули вв
            pthread_mutex_lock( &mutex_tag );
            if( pp.getquality()==OPC_QUALITY_GOOD ) {
                if( pp.isbool() && pp.m_tasktimer.isDone() ) {  // если было импульсное задание и вышел таймер
                    pp.settask( !pp.gettask() );                // инверитруем выход
                    pp.m_tasktimer.reset();
                }
            }
            pthread_mutex_unlock( &mutex_tag );
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
<<<<<<< 65512ba9996f76309fff3db429f77239cb6c5d13
//        cout<<" T4 = "<<tt.getTT();
//        pthread_mutex_unlock( &mutex_tag );
//        cout<<" T5 = "<<tt.getTT()<<endl;
=======
>>>>>>> 25. a)pthread_create into class method run() b)PID algoritm complete
        tt.start(1000);
        usleep(_tag_prc_delay);
    }     
    
    cout << "end tags processing" << endl;
    
//    return EXIT_SUCCESS;
}


