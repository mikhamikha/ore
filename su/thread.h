#ifndef _THREAD_HPP_
#define _THREAD_HPP_

//extern "C" {   
#include <pthread.h>
//}
#include <string.h>
#include <string>

using namespace std;
 
/*   Signals a problem with the thread handling.
 */
class cthreadException: public std::exception
{
public:
   /*  Construct a SocketException with a explanatory message.
   *   @param message explanatory message
   *   @param bSysMsg true if system message (from strerror(errno))
   *   should be postfixed to the user provided message
   */
    cthreadException(const string &message, bool bSysMsg = false) throw();
 
    /* Destructor.
     * Virtual to allow for subclassing.
     */
    virtual ~cthreadException() throw ();
 
    /*  Returns a pointer to the (constant) error description.
     *  @return A pointer to a \c const \c char*. The underlying memory
     *          is in posession of the \c Exception object. Callers \a must
     *          not attempt to free the memory.
     */
    virtual const char* what() const throw (){  return m_sMsg.c_str(); }
 
protected:
    /*  Error message.  */
    std::string m_sMsg;
};
 
 
/* 
*  Abstract class for Thread management 
*/
class cthread
{
 public:
    /*   Default Constructor for thread   */
    cthread();
    /*   virtual destructor     */
    virtual ~cthread();
    /*   Thread functionality Pure virtual function, 
     *   it will be re implemented in derived classes */
    virtual void run() = 0;
    /*   Function to start thread.   */
    void start() throw(cthreadException);
    /*   Function to join thread.   */
    void join() throw(cthreadException);
 private:
     /*  private Function to create thread.   */
     void createThread() throw(cthreadException);
     /*  Call back Function Passing to pthread create API   */
     static void* threadFunc(void* pTr);
     /*  Internal pthread ID..   */
     pthread_t m_Tid;
};
#endif
