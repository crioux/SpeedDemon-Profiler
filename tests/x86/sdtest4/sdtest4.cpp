#include<windows.h>

void (*pwang)(void);
void (*pdong)(void);
void (*peit)(void);

void wang_i(void) { int a; a=0; }
void dong_i(void) { int a; a=0; }
void eit_i(void) { int a; a=0; }

void main(void)
{

	for(int i=0;i<100;i++)
	{
#ifdef _WIN64
		HMODULE lib=LoadLibrary("..\\sdtest4dll\\x64\\sdtest4dll.dll");
#else
		HMODULE lib=LoadLibrary("..\\sdtest4dll\\X86\\sdtest4dll.dll");
#endif
		pwang=(void (*)(void))GetProcAddress(lib,"wang");
		pdong=(void (*)(void))GetProcAddress(lib,"dong");
		peit=(void (*)(void))GetProcAddress(lib,"eit");

		wang_i();
		pwang();

		dong_i();
		pdong();

		eit_i();
		peit();
				
		FreeLibrary(lib);
	}
}