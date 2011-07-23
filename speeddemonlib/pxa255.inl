extern "C"
{
	UINT32 arreadpmnc255();
	void arwritepmnc255(UINT32 pmnc);
	UINT32 arreadccnt255();
	void _xlock(int *plock);
	void _xunlock(int *plock);
	ADDRESS _xgetstackpointer(void);
};

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

static SPLIT64 s_pxaclock;
int s_nClockLock=1;

inline void _xinitclock(void)
{
	_xlock(&s_nClockLock);
	arwritepmnc255(7);
	s_pxaclock.high=0;
	s_pxaclock.low=arreadccnt255();
	_xunlock(&s_nClockLock);
}

inline void _xresetclock(void)
{
	_xlock(&s_nClockLock);
	arwritepmnc255(0); 
	s_pxaclock.all=0;
	_xunlock(&s_nClockLock);
}

inline void _xupdateclock(UINT64 *pClock)
{
	UINT32 lowerclock=arreadccnt255();
	if(lowerclock<s_pxaclock.low)
	{
		s_pxaclock.high++;
	}
	s_pxaclock.low=lowerclock;
	*pClock=s_pxaclock.all;
}
