#include <pthread.h>

typedef pthread_mutex_t CRITICAL_SECTION;
struct Semaph
{ pthread_cond_t  cond;
  pthread_mutex_t mutex;
  int sem_count;
  int pulsed;
  Semaph(int zero){assert(zero==0);}
  Semaph(){}
};
typedef Semaph LocEvent;

extern const pthread_mutex_t initial_mutex;

static inline void InitializeCriticalSection(CRITICAL_SECTION * cs)
{
	*cs=initial_mutex;
}

#ifdef SINGLE_THREAD_CLIENT
static inline void DeleteCriticalSection    (CRITICAL_SECTION * cs){};
static inline void EnterCriticalSection     (CRITICAL_SECTION * cs){};
static inline void LeaveCriticalSection     (CRITICAL_SECTION * cs){};
static inline BOOL CheckCriticalSection     (CRITICAL_SECTION * cs){return TRUE;};
#else

inline void DeleteCriticalSection(CRITICAL_SECTION * cs)
{
pthread_mutex_destroy(cs);
}

inline void EnterCriticalSection(CRITICAL_SECTION * cs)
{
pthread_mutex_lock(cs);
}

inline void LeaveCriticalSection(CRITICAL_SECTION * cs)
{
pthread_mutex_unlock(cs);
}

extern const pthread_mutex_t initial_mutex;

BOOL CheckCriticalSection(CRITICAL_SECTION * cs);
#endif
