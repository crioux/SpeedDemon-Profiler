#include<windows.h>
#include<stdio.h>

int fib3(int x);
int fib2(int x);
int fib1(int x);


int fib1(int x)
{
	if(x<0)
	{
		return 0;
	}
	if(x==1)
	{
		return 1;
	}
	if(x==2)
	{
		throw(x);
	}
	return x+fib2(x-1);
}


int fib2(int x)
{
	if(x<0)
	{
		return 0;
	}
	if(x==1)
	{
		return 1;
	}
	if(x==2)
	{
		throw(x);
	}
	return x+fib3(x-1);
}

int fib3(int x)
{
	if(x<0)
	{
		return 0;
	}
	if(x==1)
	{
		return 1;
	}
	if(x==2)
	{
		throw(x);
	}

	return x+fib1(x-1);
}


int main(void)
{
	int x;
	try
	{
		x=fib1(100);
	}
	catch(int)
	{
		x=0; 
	}

	return x;
}