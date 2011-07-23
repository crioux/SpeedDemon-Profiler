/////////////////////////////////////////////////////////////////////////////
// Name:        bmpshape.h
// Purpose:     wxBitmapShape
// Author:      Julian Smart
// Modified by:
// Created:     12/07/98
// RCS-ID:      $Id: bmpshape.h,v 1.1.1.1 2001/11/13 00:25:33 dildog Exp $
// Copyright:   (c) Julian Smart
// Licence:   	wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _OGL_BITMAP_H_
#define _OGL_BITMAP_H_

#ifdef __GNUG__
#pragma interface "bmpshape.h"
#endif

#include "basic.h"

class wxBitmapShape: public wxRectangleShape
{
 DECLARE_DYNAMIC_CLASS(wxBitmapShape)
 public:
  wxBitmapShape();
  ~wxBitmapShape();

  void OnDraw(wxDC& dc);

#ifdef PROLOGIO
  // I/O
  void WriteAttributes(wxExpr *clause);
  void ReadAttributes(wxExpr *clause);
#endif

  // Does the copying for this object
  void Copy(wxShape& copy);

  void SetSize(double w, double h, bool recursive = TRUE);
  inline wxBitmap& GetBitmap() const { return (wxBitmap&) m_bitmap; }
  void SetBitmap(const wxBitmap& bm);
  inline void SetFilename(const wxString& f) { m_filename = f; };
  inline wxString GetFilename() const { return m_filename; }

private:
  wxBitmap      m_bitmap;
  wxString      m_filename;
};

#endif
  // _OGL_BITMAP_H_


