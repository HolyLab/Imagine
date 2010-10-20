#ifndef lockguard_h
#define lockguard_h

#include <QMutex>

class CLockGuard {
public:
   CLockGuard(QMutex* apLock):mpLock(apLock),mOwned(false){
      mpLock->lock();
      mOwned=true;
   }
   ~CLockGuard(){
      if(mOwned)mpLock->unlock();
   }

private:
   QMutex * mpLock;
   bool mOwned;

   //forbid copy assignment and initialization
   CLockGuard& operator= (const CLockGuard &);
   CLockGuard(const CLockGuard &);

};//class


#endif //#ifndef
 
