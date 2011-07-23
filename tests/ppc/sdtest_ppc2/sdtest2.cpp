#include<windows.h>

void (*pwang)(void);
void (*pdong)(void);
void (*peit)(void);

void wang_i(void) { int a; a=0; }
void dong_i(void) { int a; a=0; }
void eit_i(void) { int a; a=0; }

int WINAPI WinMain(HINSTANCE hInst,HINSTANCE hPrev,LPWSTR lpCmdLine, int nShowCmd)
{

	for(int i=0;i<100;i++)
	{
		HMODULE lib=LoadLibrary(L"\\sdtest_ppc2dll.dll");
		pwang=(void (*)(void))GetProcAddress(lib,L"wang");
		pdong=(void (*)(void))GetProcAddress(lib,L"dong");
		peit=(void (*)(void))GetProcAddress(lib,L"eit");

		wang_i();
		pwang();

		dong_i();
		pdong();

		eit_i();
		peit();
				
		FreeLibrary(lib);
	}
	return 0;
}
