//========================================================================
//
// XpdfWidgetPrint.h
//
// Copyright 2012 Glyph & Cog, LLC
//
//========================================================================

#ifndef XPDFWIDGETPRINT_H
#define XPDFWIDGETPRINT_H

#if XPDFWIDGET_PRINTING

#include <aconf.h>

class XpdfWidget;

extern XpdfWidget::ErrorCode printPDF(PDFDoc *doc, QPrinter *prt,
				      int hDPI, int vDPI,
				      XpdfWidget *widget);

#endif // XPDFWIDGET_PRINTING

#endif // XPDFWIDGETPRINT_H
