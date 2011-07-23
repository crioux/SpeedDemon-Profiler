/////////////////////////////////////////////////////////////////////////////
// Name:        ogl.h
// Purpose:     OGL main include
// Author:      Julian Smart
// Modified by:
// Created:     12/07/98
// RCS-ID:      $Id: ogl.h,v 1.3 2004/06/09 16:42:21 ABX Exp $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _OGL_OGL_H_
#define _OGL_OGL_H_

#ifdef WXMAKINGDLL_OGL
    #define WXDLLIMPEXP_OGL WXEXPORT
#elif defined(WXUSINGDLL)
    #define WXDLLIMPEXP_OGL WXIMPORT
#else // not making nor using DLL
    #define WXDLLIMPEXP_OGL
#endif


#include "basic.h"      // Basic shapes
#include "basicp.h"
#include "lines.h"      // Lines and splines
#include "linesp.h"
#include "divided.h"    // Vertically-divided rectangle
#include "composit.h"   // Composite images
#include "canvas.h"     // wxShapeCanvas for displaying objects
#include "ogldiag.h"    // wxDiagram

#include "bmpshape.h"
#include "constrnt.h"
#include "drawn.h"
#include "drawnp.h"
#include "mfutils.h"
#include "misc.h"

// TODO: replace with wxModule implementation
extern WXDLLIMPEXP_OGL void wxOGLInitialize();
extern WXDLLIMPEXP_OGL void wxOGLCleanUp();

#endif
 // _OGL_OGL_H_
