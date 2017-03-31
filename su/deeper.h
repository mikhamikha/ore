#ifndef _deeper_h
    #define _deeper_h

class cvalve {
    bool        m_active;
    bool        m_lso;
    bool        m_lsc;
    bool        m_fault;
    int16_t     m_state;
            

    cparam*     plso;
    cparam*     plsc;
    cparam*     pcmdo;
    cparam*     pcmdc;
    cparam*     pfc;
    cparam*     pfv;
   
    public:
    
    cvalue() {}
    cvalue( string& lso, string& lsc, string& cmdo, string& cmdc, string& fc, string& fv );
    
}

class cdeeper {

}

#endif

