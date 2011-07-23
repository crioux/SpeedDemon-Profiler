#include<intrin.h>

inline void _xlock(int *plock)
{ 
	while(!_InterlockedCompareExchange((LONG*)plock,0,1))
	{
		Sleep(0);
	}
}
	
inline void _xunlock(int *plock)
{
	_InterlockedExchange((LONG*)plock, 1);
}


inline UINT32 _xhash(UINT32 key)
{
	key += ~(key << 15);
	key ^=  _rotr(key,10);
	key +=  (key << 3);
	key ^=  _rotr(key,6);
	key += ~(key << 11);
	key ^=  _rotr(key,16);
	return key;
}


inline void _xinitclock(void)
{
}

inline void _xresetclock(void)
{
}

inline void _xupdateclock(UINT64 *pClock)
{
	SPLIT64 x;
	_asm
	{
		rdtsc
		mov x.low,eax
		mov x.high,edx
	}
	*pClock=x.all;
}

inline ADDRESS _xgetstackpointer(void)
{
	register int x;
	_asm
	{
		mov x,esp
	}
	return x;
}