//========================================================================
//
// QtPDFCore.h
//
// Copyright 2009-2014 Glyph & Cog, LLC
//
//========================================================================

#ifndef QTPDFCORE_H
#define QTPDFCORE_H

#include <aconf.h>

#ifdef USE_GCC_PRAGMAS
#pragma interface
#endif

#include <QDateTime>
#include "gtypes.h"
#include "SplashTypes.h"
#include "PDFCore.h"

class GString;
class BaseStream;
class PDFDoc;
class LinkAction;

class QWidget;
class QScrollBar;

//------------------------------------------------------------------------
// callbacks
//------------------------------------------------------------------------

typedef void (*QtPDFUpdateCbk)(void *data, GString *fileName,
			       int pageNum, int numPages,
			       const char *linkLabel);

typedef void (*QtPDFMidPageChangedCbk)(void *data, int pageNum);

typedef void (*QtPDFLoadCbk)(void *data);

typedef void (*QtPDFActionCbk)(void *data, char *action);

typedef void (*QtPDFLinkCbk)(void *data, const char *type,
			     const char *dest, int page);

typedef void (*QtPDFSelectDoneCbk)(void *data);

typedef void (*QtPDFPaintDoneCbk)(void *data, bool finished);


//------------------------------------------------------------------------
// QtPDFCore
//------------------------------------------------------------------------

class QtPDFCore: public PDFCore {
public:

  // Create viewer core in <viewportA>.
  QtPDFCore(QWidget *viewportA,
	    QScrollBar *hScrollBarA, QScrollBar *vScrollBarA,
	    SplashColorPtr paperColor, SplashColorPtr matteColor,
	    GBool reverseVideo);

  ~QtPDFCore();

  //----- loadFile / displayPage / displayDest

  // Load a new file.  Returns pdfOk or error code.
  virtual int loadFile(GString *fileName, GString *ownerPassword = NULL,
		       GString *userPassword = NULL);

#ifdef _WIN32
  // Load a new file.  Returns pdfOk or error code.
  virtual int loadFile(wchar_t *fileName, int fileNameLen,
		       GString *ownerPassword = NULL,
		       GString *userPassword = NULL);
#endif

  // Load a new file, via a Stream instead of a file name.  Returns
  // pdfOk or error code.
  virtual int loadFile(BaseStream *stream, GString *ownerPassword = NULL,
		       GString *userPassword = NULL);

  // Load an already-created PDFDoc object.
  virtual void loadDoc(PDFDoc *docA);

  // Reload the current file.  This only works if the PDF was loaded
  // via a file.  Returns pdfOk or error code.
  virtual int reload();

  // Called after any update is complete.
  virtual void finishUpdate(GBool addToHist, GBool checkForChangedFile);

  //----- panning and selection

  void startPan(int wx, int wy);
  void endPan(int wx, int wy);
  void startSelection(int wx, int wy);
  void endSelection(int wx, int wy);
  void mouseMove(int wx, int wy);
  QString getSelectedTextQString();
  void copySelection(GBool toClipboard);

  //----- hyperlinks

  GBool doAction(LinkAction *action);
  LinkAction *getLinkAction() { return linkAction; }
  QString getLinkInfo(LinkAction *action);
  GString *mungeURL(GString *url);

  //----- find

  virtual GBool find(char *s, GBool caseSensitive, GBool next,
		     GBool backward, GBool wholeWord, GBool onePageOnly);
  virtual GBool findU(Unicode *u, int len, GBool caseSensitive,
		      GBool next, GBool backward,
		      GBool wholeWord, GBool onePageOnly);

  //----- password dialog

  virtual GString *getPassword();

  //----- misc access

  virtual void setBusyCursor(GBool busy);
  void doSetCursor(const QCursor &cursor);
  void doUnsetCursor();
  void takeFocus();
  QSize getBestSize();
  int getDisplayDpi() { return displayDpi; }
  void enableHyperlinks(GBool on) { hyperlinksEnabled = on; }
  GBool getHyperlinksEnabled() { return hyperlinksEnabled; }
  void enableSelect(GBool on) { selectEnabled = on; }
  void enablePan(GBool on) { panEnabled = on; }
  void setShowPasswordDialog(GBool show) { showPasswordDialog = show; }
  void setUpdateCbk(QtPDFUpdateCbk cbk, void *data)
    { updateCbk = cbk; updateCbkData = data; }
  void setMidPageChangedCbk(QtPDFMidPageChangedCbk cbk, void *data)
    { midPageChangedCbk = cbk; midPageChangedCbkData = data; }
  void setPreLoadCbk(QtPDFLoadCbk cbk, void *data)
    { preLoadCbk = cbk; preLoadCbkData = data; }
  void setPostLoadCbk(QtPDFLoadCbk cbk, void *data)
    { postLoadCbk = cbk; postLoadCbkData = data; }
  void setActionCbk(QtPDFActionCbk cbk, void *data)
    { actionCbk = cbk; actionCbkData = data; }
  void setLinkCbk(QtPDFLinkCbk cbk, void *data)
    { linkCbk = cbk; linkCbkData = data; }
  void setSelectDoneCbk(QtPDFSelectDoneCbk cbk, void *data)
    { selectDoneCbk = cbk; selectDoneCbkData = data; }
  void setPaintDoneCbk(QtPDFPaintDoneCbk cbk, void *data)
    { paintDoneCbk = cbk; paintDoneCbkData = data; }

  //----- GUI events
  void resizeEvent();
  void paintEvent(int x, int y, int w, int h);
  void scrollEvent();
  virtual void tick();

private:

  //----- hyperlinks
  void doLinkCbk(LinkAction *action);
  void runCommand(GString *cmdFmt, GString *arg);

  //----- PDFCore callbacks
  virtual void invalidate(int x, int y, int w, int h);
  virtual void updateScrollbars();
  virtual GBool checkForNewFile();
  virtual void preLoad();
  virtual void postLoad();

  QWidget *viewport;
  QScrollBar *hScrollBar;
  QScrollBar *vScrollBar;

  int displayDpi;
  double scaleFactor;

  GBool dragging;

  GBool panning;
  int panMX, panMY;

  GBool inUpdateScrollbars;

  int oldFirstPage;
  int oldMidPage;

  LinkAction *linkAction;	// mouse cursor is over this link

  LinkAction *lastLinkAction;	// getLinkInfo() caches an action
  QString lastLinkActionInfo;

  QDateTime modTime;		// last modification time of PDF file

  QtPDFUpdateCbk updateCbk;
  void *updateCbkData;
  QtPDFMidPageChangedCbk midPageChangedCbk;
  void *midPageChangedCbkData;
  QtPDFLoadCbk preLoadCbk;
  void *preLoadCbkData;
  QtPDFLoadCbk postLoadCbk;
  void *postLoadCbkData;
  QtPDFActionCbk actionCbk;
  void *actionCbkData;
  QtPDFLinkCbk linkCbk;
  void *linkCbkData;
  QtPDFSelectDoneCbk selectDoneCbk;
  void *selectDoneCbkData;
  QtPDFPaintDoneCbk paintDoneCbk;
  void *paintDoneCbkData;

  GBool hyperlinksEnabled;
  GBool selectEnabled;
  GBool panEnabled;
  GBool showPasswordDialog;
};

#endif
