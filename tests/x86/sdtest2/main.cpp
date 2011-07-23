#include<windows.h>
#include<process.h>
#include<stdio.h>

int foo(int a);
int bar(int a);
int baz(int a);

int foo(int a)
{
	return a*69+bar(a);
}

int bar(int a)
{
	if(a==0)
	{
		return 0;
	}
	return a+baz(a-1);
}

int baz(int a)
{
	if(a==0)
	{
		return 0;
	}
	return a+bar(a-1);
}

void thread1(void *)
{
	foo(1);
}

void thread2(void *)
{
	foo(2);
}

void thread3(void *)
{
	bar(3);
}

void thread4(void *)
{
	bar(4);
}

void thread5(void *)
{
	baz(5);
}



void main(void)
{
	int j;

	for(j=0;j<10;j++)
	{
		_beginthread(thread1,0,NULL);
		_beginthread(thread2,0,NULL);
		_beginthread(thread3,0,NULL);
		_beginthread(thread4,0,NULL);
		_beginthread(thread5,0,NULL);
		_beginthread(thread1,0,NULL);
		_beginthread(thread2,0,NULL);
		_beginthread(thread3,0,NULL);
		_beginthread(thread4,0,NULL);
		_beginthread(thread5,0,NULL);
		Sleep(1000);
		printf("%d\n",j);
	}
}