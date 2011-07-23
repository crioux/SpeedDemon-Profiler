/////////////////////////////////////////////////////////////////////////////
// Name:        ogl.h
// Purpose:     OGL main include
// Author:      Julian Smart
// Modified by:
// Created:     12/07/98
// RCS-ID:      $Id: ogl.h,v 1.1.1.1 2001/11/13 00:25:45 dildog Exp $
// Copyright:   (c) Julian Smart
// Licence:   	wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _OGL_OGL_H_
#define _OGL_OGL_H_

#include "basic.h"      // Basic shapes
#include "lines.h"      // Lines and splines
#include "divided.h"    // Vertically-divided rectangle
#include "composit.h"   // Composite images
#include "canvas.h"     // wxShapeCanvas for displaying objects
#include "ogldiag.h"    // wxDiagram

// TODO: replace with wxModule implementation
extern void wxOGLInitialize();
extern void wxOGLCleanUp();

#endif
 // _OGL_OGL_H_
