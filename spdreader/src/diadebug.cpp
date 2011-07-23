#include "spdreader_pch.h"
#include "dia2.h"
#include "diacreate.h"
#include "diadebug.h"

typedef void* (*MSVCRT_malloc_func)(size_t);
typedef void (*MSVCRT_free_func)(void*);

extern "C"
{
	char* __unDName(char *OutStr, const char* mangled,  int OutStrLen, MSVCRT_malloc_func memget, MSVCRT_free_func memfree, unsigned short int flags);

}

int __cdecl Dia_strFromGUID(GUID const &, wchar_t *, int)
{
	return 0;
}

typedef HRESULT ( STDAPICALLTYPE * TYPEOF_DllGetClassObject )(IN REFCLSID rclsid, IN REFIID riid, OUT LPVOID FAR* ppv);

bool LoadDIASymbols(const wxString &strPDBFile, 
					wxInt32 modulenum,
					std::list<CFunctionDescription> & llFuncs,
					stdext::hash_map<BasedAddress,int> & addressmap
					)
{
	// Create DIA80 Data Source Object
	IDiaDataSource *pDataSource;
	HRESULT hr = NoRegCoCreate( L"msdia80.dll", _uuidof( DiaSource ), _uuidof( IDiaDataSource ), (void **) &(pDataSource));
	if(!SUCCEEDED(hr)) 
	{
		return false;
	}
	/*
	HMODULE mod=LoadLibrary(L"msdia80.dll");
	if(!mod)
	{
		return false;
	}

	TYPEOF_DllGetClassObject *pGetClassObject=(TYPEOF_DllGetClassObject *)GetProcAddress(mod,"DllGetClassObject");
	if(!pGetClassObject)
	{
		FreeLibrary(mod);
		return false;
	}

	IClassFactory *pcf;
	HRESULT hr=(*pGetClassObject)(CLSID_DiaSource,IID_IClassFactory,(void **)&pcf);
	if(!SUCCEEDED(hr))
	{
		FreeLibrary(mod);
		return false;
	}

	IDiaDataSource *pDataSource;
	hr=pcf->CreateInstance(NULL,_uuidof(IDiaDataSource),(void **)&pDataSource);
	if(!SUCCEEDED(hr))
	{
		pcf->Release();
		FreeLibrary(mod);
		return false;
	}
	pcf->Release();
*/
		
	// Load the executable's debug symbols
	hr=pDataSource->loadDataFromPdb(strPDBFile);
	if(!SUCCEEDED(hr))
	{
		pDataSource->Release();
		return false;	
	}
	
	// Open a symbol session
	IDiaSession *pDIASession;
	hr=pDataSource->openSession(&pDIASession);
	if(!SUCCEEDED(hr)) 
	{
		pDataSource->Release();
		return false;
	}

	// Set the UNBASED address on the session, for resolving BasedAddress addrs
	hr=pDIASession->put_loadAddress(0);
	if(!SUCCEEDED(hr)) 
	{
		pDIASession->Release();
		pDataSource->Release();
		return false;
	}

	// Get addresses for this module
	std::list<BasedAddress> addresses;
	for(stdext::hash_map<BasedAddress,int>::iterator iter=addressmap.begin();iter!=addressmap.end();iter++)
	{
		if(iter->first.nModule==modulenum)
		{
			addresses.push_back(iter->first);
		}
	}
	stdext::hash_map<DWORD,wxInt32> functions;

	int curidx=(int)llFuncs.size();

	for(std::list<BasedAddress>::iterator itera=addresses.begin();itera!=addresses.end();itera++)
	{
		// Get the symbol for this thing
		IDiaSymbol* pFunc;
		if(FAILED(pDIASession->findSymbolByVA(itera->nAddr, SymTagFunction, &pFunc)) || pFunc==NULL)
		{
			continue;
		}

		// Get the unique symbol index id
		DWORD indexId;
		if(FAILED(pFunc->get_symIndexId(&indexId)))
		{
			pFunc->Release();
			continue;
		}

		// See if we have this function already
		stdext::hash_map<DWORD,int>::iterator iterf=functions.find(indexId);
		if(iterf!=functions.end())
		{
			addressmap[*itera]=iterf->second;
		}
		else
		{
			CFunctionDescription func;
	
			//////
			ULONGLONG addr;
			if(FAILED(pFunc->get_virtualAddress(&addr)))
			{
				pFunc->Release();
				continue;
			}

			//////
			ULONGLONG length;
			if(FAILED(pFunc->get_length(&length)))
			{
				pFunc->Release();
				continue;
			}

			/////////
			BSTR pName;
			if(FAILED(pFunc->get_name(&pName)))
			{
				pFunc->Release();
				continue;
			}
			func.strMangledName=(const LPWSTR)pName;
			LocalFree(pName);
				
			char mangled[512];
			WideCharToMultiByte(CP_UTF8,0,func.strMangledName,-1,mangled,512,NULL,NULL);
			
			/////////
			char outbase[512];
			__unDName(outbase,mangled,512,&malloc,&free,0x1000);
			wchar_t outbasew[512];
			MultiByteToWideChar(CP_UTF8,0,outbase,-1,outbasew,512);
			
			func.strBaseName=(const wchar_t *)(outbasew);

			/////////
			char outfull[512];
			__unDName(outfull,mangled,512,&malloc,&free,0);
			wchar_t outfullw[512];
			MultiByteToWideChar(CP_UTF8,0,outfull,-1,outfullw,512);
			
			func.strFullName=(const wchar_t *)(outfullw);


			//////////
			func.nAddress=BasedAddress(addr,modulenum);
			func.nByteLength=length;
			
			/////////

			IDiaEnumLineNumbers *pdeln;
			func.strSourceFile=wxT("<unknown>");
			func.nFirstSourceLine=0;
			func.nLastSourceLine=0;
			bool bGotFile=false;
			if(SUCCEEDED(pDIASession->findLinesByVA(addr,length,&pdeln)))
			{
				IDiaLineNumber *pdln;
				int nLineItem=0;
				while(SUCCEEDED(pdeln->Item(nLineItem,&pdln)))
				{
					DWORD dwLineNumber;
					IDiaSourceFile *pdsf;
					if(SUCCEEDED(pdln->get_lineNumber(&dwLineNumber)) &&
						SUCCEEDED(pdln->get_sourceFile(&pdsf)))
					{
						BSTR filename;
						if(SUCCEEDED(pdsf->get_fileName(&filename)))
						{
							if(bGotFile)
							{
								if(filename==func.strSourceFile)
								{
									if(dwLineNumber<func.nFirstSourceLine)
									{
										func.nFirstSourceLine=dwLineNumber;
									}
									if(dwLineNumber>func.nLastSourceLine)
									{
										func.nLastSourceLine=dwLineNumber;
									}
								}
							}
							else
							{
								bGotFile=true;
								func.strSourceFile=filename;
								func.nFirstSourceLine=dwLineNumber;
								func.nLastSourceLine=dwLineNumber;
							}
							LocalFree(filename);
						}
						pdsf->Release();
					}
					pdln->Release();
					nLineItem++;
				}
				pdeln->Release();
			}
           
			llFuncs.push_back(func);

			functions[indexId]=curidx;

			addressmap[*itera]=curidx;

			curidx++;
		}
	}
	
	pDIASession->Release();
	pDataSource->Release();
		
//	FreeLibrary(mod);

	return true;
}