#include "thread.h"
#include "errno.h"
#include <stdlib.h>
 
cthreadException::cthreadException( const string &sMessage, bool blSysMsg /*= false*/ ) throw() :m_sMsg(sMessage)
{
    if (blSysMsg) {
        m_sMsg.append(": ");
        m_sMsg.append(strerror(errno));
    }
}
 
cthreadException::~cthreadException() throw ()
{
 
}
 
cthread::cthread()
{
 
}
 
cthread::~cthread()
{
 
}
 
void cthread::start() throw(cthreadException)
{
   createThread();
}
 
void cthread::join() throw(cthreadException)
{
    int rc = pthread_join(m_Tid, NULL);
    if ( rc != 0 )
    {        
        throw cthreadException("Error in thread join.... (pthread_join())", true);
    }  
}
 
void* cthread::threadFunc( void* pTr )
{
    cthread* pThis = static_cast<cthread*>(pTr);
    pThis->run();
    pthread_exit(0); 
}
 
void cthread::createThread() throw(cthreadException)
{
    int rc = pthread_create (&m_Tid, NULL, threadFunc,this);
    if ( rc != 0 )
    {        
        throw cthreadException("Error in thread creation... (pthread_create())", true);
    }  
}
