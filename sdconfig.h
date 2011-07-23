#ifndef __SDCONFIG_H
#define __SDCONFIG_H

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	

#define SD_X86_WIN32	1
#define SD_X86_WCE		2
#define SD_PXA270_WCE	3
#define SD_PXA255_WCE	4
#define SD_X64_WIN64	5

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	

#ifndef SD_ARCH
#error "You need to set SD_ARCH."
#endif

#if SD_ARCH==SD_X86_WIN32

#include<windows.h>

	#define LITTLE_ENDIAN 1
	#define HASH_SIZE_BITS (12)
	#define CALLSTACK_DEPTH (1024)
	#define MAX_CONCURRENT_THREADS (1024)
	#define MAX_MODULES (256)
	#define HASH_FULL_PERCENT256 (128)
	#define WINDOWSCODE 1
	#define HAS_PENTER_PEXIT 1
	#define FAST_MODULE_TABLE 1
	#define STACKGROWSDOWN 1
	#define PE32_MODULE_TAGGING 1

	typedef unsigned __int64 UINT64;
	typedef signed __int64 INT64;
	typedef unsigned int UINT32;
	typedef signed int INT32;
	typedef unsigned short UINT16;
	typedef signed short INT16;
	typedef unsigned char UINT8;
	typedef signed char INT8;
	typedef size_t ADDRESS;
	typedef HANDLE FILEMAP;
	typedef HANDLE THREAD;
		
#elif SD_ARCH==SD_X86_WCE

#include<windows.h>
#include<tlhelp32.h>
	
	#define LITTLE_ENDIAN 1
	#define HASH_SIZE_BITS (10)
	#define CALLSTACK_DEPTH (1024)
	#define MAX_CONCURRENT_THREADS (1024)
	#define MAX_MODULES (256)
	#define HASH_FULL_PERCENT256 (128)
	#define WINDOWSCODE 1
	#define WINDOWSCE 1
	#define HAS_CALLCAP 1
	#define FAST_MODULE_TABLE 1
	#define STACKGROWSDOWN 1
	#define PE32_MODULE_TAGGING 1

	typedef unsigned __int64 UINT64;
	typedef signed __int64 INT64;
	typedef unsigned int UINT32;
	typedef signed int INT32;
	typedef unsigned short UINT16;
	typedef signed short INT16;
	typedef unsigned char UINT8;
	typedef signed char INT8;
	typedef size_t ADDRESS;
	typedef HANDLE FILEMAP;
	typedef UINT32  THREAD;
	
#elif SD_ARCH==SD_PXA270_WCE

#include<windows.h>
#include<tlhelp32.h>
	
	#define LITTLE_ENDIAN 1
	#define HASH_SIZE_BITS (10)
	#define CALLSTACK_DEPTH (1024)
	#define MAX_CONCURRENT_THREADS (1024)
	#define MAX_MODULES (256)
	#define HASH_FULL_PERCENT256 (128)
	#define WINDOWSCODE 1
	#define WINDOWSCE 1
	#define HAS_CALLCAP 1
	#define HAS_FASTCAP 1
	#define FAST_MODULE_TABLE 1
	#define STACKGROWSDOWN 1
	#define PE32_MODULE_TAGGING 1
	
	typedef unsigned __int64 UINT64;
	typedef signed __int64 INT64;
	typedef unsigned int UINT32;
	typedef signed int INT32;
	typedef unsigned short UINT16;
	typedef signed short INT16;
	typedef unsigned char UINT8;
	typedef signed char INT8;
	typedef size_t ADDRESS;
	typedef HANDLE FILEMAP;
	typedef UINT32  THREAD;
	
#elif SD_ARCH==SD_PXA255_WCE

#include<windows.h>
#include<tlhelp32.h>
	
	#define LITTLE_ENDIAN 1
	#define HASH_SIZE_BITS (10)
	#define CALLSTACK_DEPTH (1024)
	#define MAX_CONCURRENT_THREADS (1024)
	#define MAX_MODULES (256)
	#define HASH_FULL_PERCENT256 (128)
	#define WINDOWSCODE 1
	#define WINDOWSCE 1
	#define HAS_CALLCAP 1
	#define HAS_FASTCAP 1
	#define FAST_MODULE_TABLE 1
	#define STACKGROWSDOWN 1
	#define PE32_MODULE_TAGGING 1
	
	typedef unsigned __int64 UINT64;
	typedef signed __int64 INT64;
	typedef unsigned int UINT32;
	typedef signed int INT32;
	typedef unsigned short UINT16;
	typedef signed short INT16;
	typedef unsigned char UINT8;
	typedef signed char INT8;
	typedef size_t ADDRESS;
	typedef HANDLE FILEMAP;
	typedef UINT32 THREAD;
	
#elif SD_ARCH==SD_X64_WIN64

#include<windows.h>

	#define LITTLE_ENDIAN 1
	#define HASH_SIZE_BITS (12)
	#define CALLSTACK_DEPTH (1024)
	#define MAX_CONCURRENT_THREADS (1024)
	#define MAX_MODULES (256)
	#define HASH_FULL_PERCENT256 (128)
	#define WINDOWSCODE 1
	#define HAS_PENTER_PEXIT 1
	#define STACKGROWSDOWN 1
	#define PE32_MODULE_TAGGING 1

	typedef unsigned __int64 UINT64;
	typedef signed __int64 INT64;
	typedef unsigned int UINT32;
	typedef signed int INT32;
	typedef unsigned short UINT16;
	typedef signed short INT16;
	typedef unsigned char UINT8;
	typedef signed char INT8;
	typedef size_t ADDRESS;
	typedef HANDLE FILEMAP;
	typedef HANDLE THREAD;
		
#else
	
	#error "Need to choose a valid architecture. Set SD_ARCH."

#endif

union SPLIT64
{
	struct
	{
#ifdef LITTLE_ENDIAN
	UINT32 low;
	UINT32 high;
#else
	UINT32 high;
	UINT32 low;
#endif			
	};
	UINT64 all;
};


// Validation
#if HASH_SIZE_BITS<10
#error "Initial hash size too small."
#endif

#endif