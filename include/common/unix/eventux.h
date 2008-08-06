#ifdef SINGLE_THREAD_CLIENT
BOOL inline CreateSemaph(sig32 lInitialCount, sig32 lMaximumCount, Semaph * semaph)
{
  return TRUE;
}

BOOL inline CloseSemaph(Semaph * semaph)
{
  return TRUE;
}

BOOL inline ReleaseSemaph(Semaph * semaph, sig32 count)
{
  return TRUE;
}

DWORD inline WaitSemaph(Semaph * semaph, DWORD timeout)
{ 
  return WAIT_OBJECT_0;
}
#else
BOOL CreateSemaph(sig32 lInitialCount, sig32 lMaximumCount, Semaph * semaph);
BOOL CloseSemaph(Semaph * semaph);
BOOL ReleaseSemaph(Semaph * semaph, sig32 count);
DWORD WaitSemaph(Semaph * semaph, DWORD timeout);
#endif

inline BOOL  CreateLocalAutoEvent(BOOL bInitialState, LocEvent * event)
{ return CreateSemaph(bInitialState ? 1 : 0, 1, event); }

inline BOOL  WINAPI CloseLocalAutoEvent(LocEvent * event)
{ return CloseSemaph(event); }

inline BOOL  WINAPI SetLocalAutoEvent(LocEvent * event)
{ return ReleaseSemaph(event, 1); }

inline DWORD WINAPI WaitLocalAutoEvent(LocEvent * event, DWORD timeout)
{ return WaitSemaph(event, timeout); }
