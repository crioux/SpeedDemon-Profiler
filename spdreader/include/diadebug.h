#ifndef __INC_DIADEBUG_H
#define __INC_DIADEBUG_H

#include"spdreaderdoc.h"

bool LoadDIASymbols(const wxString &strPDBFile, 
					wxInt32 modulenum,
					std::list<CFunctionDescription> & llFuncs,
					stdext::hash_map<BasedAddress,int> & addressmap
					);

#endif