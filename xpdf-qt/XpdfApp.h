//========================================================================
//
// XpdfApp.h
//
// Copyright 2015 Glyph & Cog, LLC
//
//========================================================================

#ifndef XPDFAPP_H
#define XPDFAPP_H

#include <aconf.h>

#include <QApplication>
#include <QColor>
#include "gtypes.h"

class GList;
class XpdfViewer;

//------------------------------------------------------------------------
// XpdfApp
//------------------------------------------------------------------------

class XpdfApp: public QApplication {
  Q_OBJECT

public:

  XpdfApp(int &argc, char **argv);
  virtual ~XpdfApp();

  int getNumViewers();

  void newWindow(GBool fullScreen = gFalse);

  GBool openInNewWindow(QString fileName, int page = 1,
			QString dest = QString(),
			QString password = QString(),
			GBool fullScreen = gFalse);

  void closeWindowOrQuit(XpdfViewer *viewer);

  void quit();

  //--- for use by XpdfViewer

  int getErrorEventType() { return errorEventType; }
  const QColor &getPaperColor() { return paperColor; }
  const QColor &getMatteColor() { return matteColor; }
  const QColor &getFullScreenMatteColor() { return fsMatteColor; }
  GBool getReverseVideo() { return reverseVideo; }

private:

  int errorEventType;
  QColor paperColor;
  QColor matteColor;
  QColor fsMatteColor;
  GBool reverseVideo;

  GList *viewers;		// [XpdfViewer]
};

#endif
