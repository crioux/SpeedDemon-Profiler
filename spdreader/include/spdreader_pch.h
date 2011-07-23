#ifndef __INC_SPDREADER_PCH_H
#define __INC_SPDREADER_PCH_H

#include<vector>
#include<list>
#include<deque>
#include<map>
#include<hash_map>
#include<hash_set>
#include<algorithm>
#include<functional>   

#include"wx/wx.h"
#include"wx/docview.h"
#include"wx/settings.h"
#include"wx/config.h"
#include"wx/filedlg.h"
#include"wx/filename.h"
#include"wx/stream.h"
#include"wx/grid.h"
#include"wx/listctrl.h"
#include"wx/treectrl.h"
#include"wx/mstream.h"
#include"wx/wfstream.h"
#include"treemultiitemwindow.h"
#include"treemultiitemroot.h"
#include"treemultiitemnode.h"
#include"treemultiitembase.h"
#include"wxtreemultictrl.h"
#include"graphview.h"
#include"noflickerlistctrl.h"
#include"piechart.h"


#if defined(_WIN32) || defined(_WIN32_WCE)
#define LLFMT wxT("%I64u")
#else
#define LLFMT wxT("%llu")
#endif
#define FLFMT wxT("%.5f");


#include"../res/spdreader_wdr.h"

#endif