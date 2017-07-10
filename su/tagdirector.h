#ifndef _tag_director_h
    #define _tag_director_h

#include "main.h"

typedef std::vector<std::pair<std::string, ctag> > taglist;

// объявление класса 
class ctagdirector: public cthread {
    private:
        taglist tags;
       
    public:
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
