#ifndef _tag_director_h
    #define _tag_director_h

#include "main.h"

typedef std::vector<std::pair<std::string, ctag> > taglist;

// объявление класса 
class ctagdirector: public cthread {
    private:
        taglist tags;
       
    public:
<<<<<<< 65512ba9996f76309fff3db429f77239cb6c5d13
=======
        void addtag( string& s, ctag& p ) {
            if( gettag(s.c_str())==NULL )
                tags.push_back(make_pair(s, p)); 
        }
>>>>>>> 25. a)pthread_create into class method run() b)PID algoritm complete
        int16_t tasktag( std::string&, std::string&, std::string& );
        int16_t tasktag( std::string&, std::string& );

        int16_t gettag( const char*, double&, int16_t& qual, timespec*, int16_t );
        int16_t gettag( const char*, std::string& );
        ctag*   gettag( const char* );
        int16_t gettagcount( const char*, int16_t& );
        int16_t gettaglimits( const char*, double&, double& );
        void run();
};


extern ctagdirector tagdir;

#endif
