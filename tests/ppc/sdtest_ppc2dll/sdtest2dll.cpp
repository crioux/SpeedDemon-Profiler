#include<windows.h>

extern "C" 
{

__declspec(dllexport) void wang(void)
{
	int x=0;
	x=3;
}

__declspec(dllexport) void dong(void)
{
	int x=0;
	x=3;
}

__declspec(dllexport) void eit(void)
{
	int x=0;
	x=3;
}

}