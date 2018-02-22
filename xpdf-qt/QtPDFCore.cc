//========================================================================
//
// QtPDFCore.cc
//
// Copyright 2009-2014 Glyph & Cog, LLC
//
//========================================================================

#include <aconf.h>

#ifdef USE_GCC_PRAGMAS
#pragma implementation
#endif

#include <math.h>
#include <string.h>
#include <QApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QFileInfo>
#include <QImage>
#include <QInputDialog>
#include <QMessageBox>
#include <QPainter>
#include <QProcess>
#include <QScrollBar>
#include <QStyle>
#include <QUrl>
#include <QWidget>
#include "gmem.h"
#include "gmempp.h"
#include "gfile.h"
#include "GString.h"
#include "GList.h"
#include "Error.h"
#include "GlobalParams.h"
#include "PDFDoc.h"
#include "Link.h"
#include "ErrorCodes.h"
#include "GfxState.h"
#include "PSOutputDev.h"
#include "TextOutputDev.h"
#include "SplashBitmap.h"
#include "DisplayState.h"
#include "TileMap.h"
#include "QtPDFCore.h"

//------------------------------------------------------------------------
// QtPDFCore
//------------------------------------------------------------------------

QtPDFCore::QtPDFCore(QWidget *viewportA,
		     QScrollBar *hScrollBarA, QScrollBar *vScrollBarA,
		     SplashColorPtr paperColor, SplashColorPtr matteColor,
		     GBool reverseVideo):
  PDFCore(splashModeRGB8, 4, reverseVideo, paperColor)
{
  int dpiX, dpiY;

  viewport = viewportA;
  hScrollBar = hScrollBarA;
  vScrollBar = vScrollBarA;
  hScrollBar->setRange(0, 0);
  hScrollBar->setSingleStep(16);
  vScrollBar->setRange(0, 0);
  vScrollBar->setSingleStep(16);
  viewport->setMouseTracking(true);

  state->setMatteColor(matteColor);

  oldFirstPage = -1;
  oldMidPage = -1;

  linkAction = NULL;
  lastLinkAction = NULL;

  dragging = gFalse;

  panning = gFalse;

  inUpdateScrollbars = gFalse;

  updateCbk = NULL;
  midPageChangedCbk = NULL;
  preLoadCbk = NULL;
  postLoadCbk = NULL;
  actionCbk = NULL;
  linkCbk = NULL;
  selectDoneCbk = NULL;

  // optional features default to on
  hyperlinksEnabled = gTrue;
  selectEnabled = gTrue;
  panEnabled = gTrue;
  showPasswordDialog = gTrue;

  // get Qt's HiDPI scale factor
#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
  scaleFactor = viewport->devicePixelRatioF();
#elif QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
  scaleFactor = viewport->devicePixelRatio();
#else
  scaleFactor = 1;
#endif

  // get the display resolution (used for HiDPI scaling)
  dpiX = viewport->logicalDpiX();
  dpiY = viewport->logicalDpiY();
  displayDpi = dpiX < dpiY ? dpiX : dpiY;
  displayDpi = (int)(displayDpi * scaleFactor);
}

QtPDFCore::~QtPDFCore() {
}

//------------------------------------------------------------------------
// loadFile / displayPage / displayDest
//------------------------------------------------------------------------

int QtPDFCore::loadFile(GString *fileName, GString *ownerPassword,
			GString *userPassword) {
  int err;

  err = PDFCore::loadFile(fileName, ownerPassword, userPassword);
  if (err == errNone) {
    // save the modification time
    modTime = QFileInfo(doc->getFileName()->getCString()).lastModified();

    // update the parent window
    if (updateCbk) {
      (*updateCbk)(updateCbkData, doc->getFileName(), -1,
		   doc->getNumPages(), NULL);
    }
    oldFirstPage = oldMidPage = -1;
  }
  return err;
}

#ifdef _WIN32
int QtPDFCore::loadFile(wchar_t *fileName, int fileNameLen,
			GString *ownerPassword,
			GString *userPassword) {
  int err;

  err = PDFCore::loadFile(fileName, fileNameLen, ownerPassword, userPassword);
  if (err == errNone) {
    // save the modification time
    modTime = QFileInfo(doc->getFileName()->getCString()).lastModified();

    // update the parent window
    if (updateCbk) {
      (*updateCbk)(updateCbkData, doc->getFileName(), -1,
		   doc->getNumPages(), NULL);
    }
    oldFirstPage = oldMidPage = -1;
  }
  return err;
}
#endif

int QtPDFCore::loadFile(BaseStream *stream, GString *ownerPassword,
			GString *userPassword) {
  int err;

  err = PDFCore::loadFile(stream, ownerPassword, userPassword);
  if (err == errNone) {
    // no file
    modTime = QDateTime();

    // update the parent window
    if (updateCbk) {
      (*updateCbk)(updateCbkData, doc->getFileName(), -1,
		   doc->getNumPages(), NULL);
    }
    oldFirstPage = oldMidPage = -1;
  }
  return err;
}

void QtPDFCore::loadDoc(PDFDoc *docA) {
  PDFCore::loadDoc(docA);

  // save the modification time
  if (doc->getFileName()) {
    modTime = QFileInfo(doc->getFileName()->getCString()).lastModified();
  } else {
    modTime = QDateTime();
  }

  // update the parent window
  if (updateCbk) {
    (*updateCbk)(updateCbkData, doc->getFileName(), -1,
		 doc->getNumPages(), NULL);
  }
  oldFirstPage = oldMidPage = -1;
}

int QtPDFCore::reload() {
  int err;

  err = PDFCore::reload();
  if (err == errNone) {
    // save the modification time
    modTime = QFileInfo(doc->getFileName()->getCString()).lastModified();

    // update the parent window
    if (updateCbk) {
      (*updateCbk)(updateCbkData, doc->getFileName(), -1,
		   doc->getNumPages(), NULL);
    }
    oldFirstPage = oldMidPage = -1;
  }
  return err;
}

void QtPDFCore::finishUpdate(GBool addToHist, GBool checkForChangedFile) {
  int firstPage, midPage;

  PDFCore::finishUpdate(addToHist, checkForChangedFile);
  firstPage = getPageNum();
  if (doc && firstPage != oldFirstPage && updateCbk) {
    (*updateCbk)(updateCbkData, NULL, firstPage, -1, "");
  }
  oldFirstPage = firstPage;
  midPage = getMidPageNum();
  if (doc && midPage != oldMidPage && midPageChangedCbk) {
    (*midPageChangedCbk)(midPageChangedCbkData, midPage);
  }
  oldMidPage = midPage;

  linkAction = NULL;
  lastLinkAction = NULL;
}

//------------------------------------------------------------------------
// panning and selection
//------------------------------------------------------------------------

void QtPDFCore::startPan(int wx, int wy) {
  if (!panEnabled) {
    return;
  }
  panning = gTrue;
  panMX = wx;
  panMY = wy;
}

void QtPDFCore::endPan(int wx, int wy) {
  panning = gFalse;
}

void QtPDFCore::startSelection(int wx, int wy) {
  int pg, x, y;

  takeFocus();
  if (!doc || doc->getNumPages() == 0 || !selectEnabled) {
    return;
  }
  if (!cvtWindowToDev(wx, wy, &pg, &x, &y)) {
    return;
  }
    startSelectionDrag(pg, x, y);
    doSetCursor(Qt::CrossCursor);
    dragging = gTrue;
}

void QtPDFCore::endSelection(int wx, int wy) {
  LinkAction *action;
  int pg, x, y;
  double xu, yu;
  GBool ok;

  if (!doc || doc->getNumPages() == 0) {
    return;
  }
  ok = cvtWindowToDev(wx, wy, &pg, &x, &y);
  if (dragging) {
    dragging = gFalse;
    doUnsetCursor();
    if (ok) {
      moveSelectionDrag(pg, x, y);
    }
    finishSelectionDrag();
    if (selectDoneCbk) {
      (*selectDoneCbk)(selectDoneCbkData);
    }
#ifndef NO_TEXT_SELECT
    if (hasSelection()) {
      copySelection(gFalse);
    }
#endif
  }
  if (ok) {
    if (hasSelection()) {
      action = NULL;
    } else {
      cvtDevToUser(pg, x, y, &xu, &yu);
      action = findLink(pg, xu, yu);
    }
    if (linkCbk && action) {
      doLinkCbk(action);
    }
    if (hyperlinksEnabled && action) {
      doAction(action);
    }
  }
}

void QtPDFCore::mouseMove(int wx, int wy) {
  LinkAction *action;
  int pg, x, y;
  double xu, yu;
  const char *s;
  GBool ok;

  if (!doc || doc->getNumPages() == 0) {
    return;
  }
  ok = cvtWindowToDev(wx, wy, &pg, &x, &y);
  if (dragging) {
    if (ok) {
      moveSelectionDrag(pg, x, y);
    }
  } else if (hyperlinksEnabled) {
    cvtDevToUser(pg, x, y, &xu, &yu);
    if (ok && (action = findLink(pg, xu, yu))) {
      doSetCursor(Qt::PointingHandCursor);
      if (action != linkAction) {
	linkAction = action;
	if (updateCbk) {
	  //~ should pass a QString to updateCbk()
	  s = getLinkInfo(action).toLocal8Bit().constData();
	  (*updateCbk)(updateCbkData, NULL, -1, -1, s);
	}
      }
    } else {
      doUnsetCursor();
      if (linkAction) {
	linkAction = NULL;
	if (updateCbk) {
	  (*updateCbk)(updateCbkData, NULL, -1, -1, "");
	}
      }
    }
  }

  if (panning) {
    scrollTo(getScrollX() - (wx - panMX),
	     getScrollY() - (wy - panMY));
    panMX = wx;
    panMY = wy;
  }
}

void QtPDFCore::doLinkCbk(LinkAction *action) {
  LinkDest *dest;
  GString *namedDest;
  Ref pageRef;
  int pg;
  GString *cmd, *params;
  char *s;

  if (!linkCbk) {
    return;
  }

  switch (action->getKind()) {

  case actionGoTo:
    dest = NULL;
    if ((dest = ((LinkGoTo *)action)->getDest())) {
      dest = dest->copy();
    } else if ((namedDest = ((LinkGoTo *)action)->getNamedDest())) {
      dest = doc->findDest(namedDest);
    }
    pg = 0;
    if (dest) {
      if (dest->isPageRef()) {
	pageRef = dest->getPageRef();
	pg = doc->findPage(pageRef.num, pageRef.gen);
      } else {
	pg = dest->getPageNum();
      }
      delete dest;
    }
    (*linkCbk)(linkCbkData, "goto", NULL, pg);
    break;

  case actionGoToR:
    (*linkCbk)(linkCbkData, "pdf",
	       ((LinkGoToR *)action)->getFileName()->getCString(), 0);
    break;

  case actionLaunch:
    cmd = ((LinkLaunch *)action)->getFileName()->copy();
    s = cmd->getCString();
    if (strcmp(s + cmd->getLength() - 4, ".pdf") &&
	strcmp(s + cmd->getLength() - 4, ".PDF") &&
	(params = ((LinkLaunch *)action)->getParams())) {
      cmd->append(' ')->append(params);
    }
    (*linkCbk)(linkCbkData, "launch", cmd->getCString(), 0);
    delete cmd;
    break;

  case actionURI:
    (*linkCbk)(linkCbkData, "url",
	       ((LinkURI *)action)->getURI()->getCString(), 0);
    break;

  case actionNamed:
    (*linkCbk)(linkCbkData, "named",
	       ((LinkNamed *)action)->getName()->getCString(), 0);
    break;

  case actionMovie:
  case actionJavaScript:
  case actionSubmitForm:
  case actionHide:
  case actionUnknown:
    (*linkCbk)(linkCbkData, "unknown", NULL, 0);
    break;
  }
}

QString QtPDFCore::getSelectedTextQString() {
  GString *s, *enc;
  QString qs;
  QChar c;
  int i;

  if (!doc->okToCopy()) {
    return "";
  }
  if (!(s = getSelectedText())) {
    return "";
  }
  enc = globalParams->getTextEncodingName();
  if (!s->cmp("UTF-8")) {
    qs = QString::fromUtf8(s->getCString());
  } else if (!s->cmp("UCS-2")) {
    for (i = 0; i+1 < s->getLength(); i += 2) {
      qs.append((QChar)(((s->getChar(i) & 0xff) << 8) +
			(s->getChar(i+1) & 0xff)));
    }
  } else {
    qs = QString(s->getCString());
  }
  delete s;
  delete enc;
  return qs;
}

void QtPDFCore::copySelection(GBool toClipboard) {
  QString qs;

  // only X11 has the copy-on-select behavior
  if (!toClipboard && !QApplication::clipboard()->supportsSelection()) {
    return;
  }
  if (!doc->okToCopy()) {
    return;
  }
  if (hasSelection()) {
    QApplication::clipboard()->setText(getSelectedTextQString(),
				       toClipboard ? QClipboard::Clipboard
					           : QClipboard::Selection);
  }
}

//------------------------------------------------------------------------
// hyperlinks
//------------------------------------------------------------------------

GBool QtPDFCore::doAction(LinkAction *action) {
  LinkActionKind kind;
  LinkDest *dest;
  GString *namedDest;
  char *s;
  GString *fileName, *fileName2, *params;
  GString *cmd;
  GString *actionName;
  Object movieAnnot, obj1, obj2;
  GString *msg;
  int i;

  switch (kind = action->getKind()) {

  // GoTo / GoToR action
  case actionGoTo:
  case actionGoToR:
    if (kind == actionGoTo) {
      dest = NULL;
      namedDest = NULL;
      if ((dest = ((LinkGoTo *)action)->getDest())) {
	dest = dest->copy();
      } else if ((namedDest = ((LinkGoTo *)action)->getNamedDest())) {
	namedDest = namedDest->copy();
      }
    } else {
      dest = NULL;
      namedDest = NULL;
      if ((dest = ((LinkGoToR *)action)->getDest())) {
	dest = dest->copy();
      } else if ((namedDest = ((LinkGoToR *)action)->getNamedDest())) {
	namedDest = namedDest->copy();
      }
      s = ((LinkGoToR *)action)->getFileName()->getCString();
      if (isAbsolutePath(s)) {
	fileName = new GString(s);
      } else {
	fileName = appendToPath(grabPath(doc->getFileName()->getCString()), s);
      }
      if (loadFile(fileName) != errNone) {
	if (dest) {
	  delete dest;
	}
	if (namedDest) {
	  delete namedDest;
	}
	delete fileName;
	return gFalse;
      }
      delete fileName;
    }
    if (namedDest) {
      dest = doc->findDest(namedDest);
      delete namedDest;
    }
    if (dest) {
      displayDest(dest);
      delete dest;
    } else {
      if (kind == actionGoToR) {
	displayPage(1, gFalse, gFalse, gTrue);
      }
    }
    break;

  // Launch action
  case actionLaunch:
    fileName = ((LinkLaunch *)action)->getFileName();
    s = fileName->getCString();
    if (fileName->getLength() >= 4 &&
	(!strcmp(s + fileName->getLength() - 4, ".pdf") ||
	 !strcmp(s + fileName->getLength() - 4, ".PDF"))) {
      if (isAbsolutePath(s)) {
	fileName = fileName->copy();
      } else {
	fileName = appendToPath(grabPath(doc->getFileName()->getCString()), s);
      }
      if (loadFile(fileName) != errNone) {
	delete fileName;
	return gFalse;
      }
      delete fileName;
      displayPage(1, gFalse, gFalse, gTrue);
    } else {
      cmd = fileName->copy();
      if ((params = ((LinkLaunch *)action)->getParams())) {
	cmd->append(' ')->append(params);
      }
      if (globalParams->getLaunchCommand()) {
	cmd->insert(0, ' ');
	cmd->insert(0, globalParams->getLaunchCommand());
	QProcess::startDetached(cmd->getCString());
      } else {
	msg = new GString("About to execute the command:\n");
	msg->append(cmd);
	if (QMessageBox::question(viewport, "PDF Launch Link",
				  msg->getCString(),
				  QMessageBox::Ok | QMessageBox::Cancel,
				  QMessageBox::Ok)
	    == QMessageBox::Ok) {
	  QProcess::startDetached(cmd->getCString());
	}
	delete msg;
      }
      delete cmd;
    }
    break;

  // URI action
  case actionURI:
    QDesktopServices::openUrl(QUrl(((LinkURI *)action)->getURI()->getCString(),
				   QUrl::TolerantMode));
    break;

  // Named action
  case actionNamed:
    actionName = ((LinkNamed *)action)->getName();
    if (!actionName->cmp("NextPage")) {
      gotoNextPage(1, gTrue);
    } else if (!actionName->cmp("PrevPage")) {
      gotoPrevPage(1, gTrue, gFalse);
    } else if (!actionName->cmp("FirstPage")) {
      displayPage(1, gTrue, gFalse, gTrue);
    } else if (!actionName->cmp("LastPage")) {
      displayPage(doc->getNumPages(), gTrue, gFalse, gTrue);
    } else if (!actionName->cmp("GoBack")) {
      goBackward();
    } else if (!actionName->cmp("GoForward")) {
      goForward();
    } else if (!actionName->cmp("Quit")) {
      if (actionCbk) {
	(*actionCbk)(actionCbkData, actionName->getCString());
      }
    } else {
      error(errSyntaxError, -1,
	    "Unknown named action: '{0:t}'", actionName);
      return gFalse;
    }
    break;

  // Movie action
  case actionMovie:
    if (!(cmd = globalParams->getMovieCommand())) {
      error(errConfig, -1, "No movieCommand defined in config file");
      return gFalse;
    }
    if (((LinkMovie *)action)->hasAnnotRef()) {
      doc->getXRef()->fetch(((LinkMovie *)action)->getAnnotRef()->num,
			    ((LinkMovie *)action)->getAnnotRef()->gen,
			    &movieAnnot);
    } else {
      //~ need to use the correct page num here
      doc->getCatalog()->getPage(tileMap->getFirstPage())->getAnnots(&obj1);
      if (obj1.isArray()) {
	for (i = 0; i < obj1.arrayGetLength(); ++i) {
	  if (obj1.arrayGet(i, &movieAnnot)->isDict()) {
	    if (movieAnnot.dictLookup("Subtype", &obj2)->isName("Movie")) {
	      obj2.free();
	      break;
	    }
	    obj2.free();
	  }
	  movieAnnot.free();
	}
	obj1.free();
      }
    }
    if (movieAnnot.isDict()) {
      if (movieAnnot.dictLookup("Movie", &obj1)->isDict()) {
	if (obj1.dictLookup("F", &obj2)) {
	  if ((fileName = LinkAction::getFileSpecName(&obj2))) {
	    if (!isAbsolutePath(fileName->getCString())) {
	      fileName2 = appendToPath(
			      grabPath(doc->getFileName()->getCString()),
			      fileName->getCString());
	      delete fileName;
	      fileName = fileName2;
	    }
	    runCommand(cmd, fileName);
	    delete fileName;
	  }
	  obj2.free();
	}
	obj1.free();
      }
    }
    movieAnnot.free();
    break;

  // unimplemented actions
  case actionJavaScript:
  case actionSubmitForm:
  case actionHide:
    return gFalse;

  // unknown action type
  case actionUnknown:
    error(errSyntaxError, -1, "Unknown link action type: '{0:t}'",
	  ((LinkUnknown *)action)->getAction());
    return gFalse;
  }

  return gTrue;
}

QString QtPDFCore::getLinkInfo(LinkAction *action) {
  LinkDest *dest;
  GString *namedDest;
  Ref pageRef;
  int pg;
  QString info;

  if (action == lastLinkAction && !lastLinkActionInfo.isEmpty()) {
    return lastLinkActionInfo;
  }

  switch (action->getKind()) {
  case actionGoTo:
    dest = NULL;
    if ((dest = ((LinkGoTo *)action)->getDest())) {
      dest = dest->copy();
    } else if ((namedDest = ((LinkGoTo *)action)->getNamedDest())) {
      dest = doc->findDest(namedDest);
    }
    pg = 0;
    if (dest) {
      if (dest->isPageRef()) {
	pageRef = dest->getPageRef();
	pg = doc->findPage(pageRef.num, pageRef.gen);
      } else {
	pg = dest->getPageNum();
      }
      delete dest;
    }
    if (pg) {
      info = QString("[page ") + QString::number(pg) + QString("]");
    } else {
      info = "[internal]";
    }
    break;
  case actionGoToR:
    info = QString(((LinkGoToR *)action)->getFileName()->getCString());
    break;
  case actionLaunch:
    info = QString(((LinkLaunch *)action)->getFileName()->getCString());
    break;
  case actionURI:
    info = QString(((LinkURI *)action)->getURI()->getCString());
    break;
  case actionNamed:
    info = QString(((LinkNamed *)action)->getName()->getCString());
    break;
  case actionMovie:
    info = "[movie]";
    break;
  case actionJavaScript:
  case actionSubmitForm:
  case actionHide:
  case actionUnknown:
  default:
    info = "[unknown]";
    break;
  }

  lastLinkAction = action;
  lastLinkActionInfo = info;

  return info;
}

// Run a command, given a <cmdFmt> string with one '%s' in it, and an
// <arg> string to insert in place of the '%s'.
void QtPDFCore::runCommand(GString *cmdFmt, GString *arg) {
  GString *cmd;
  char *s;

  if ((s = strstr(cmdFmt->getCString(), "%s"))) {
    cmd = mungeURL(arg);
    cmd->insert(0, cmdFmt->getCString(),
		s - cmdFmt->getCString());
    cmd->append(s + 2);
  } else {
    cmd = cmdFmt->copy();
  }
  QProcess::startDetached(cmd->getCString());
  delete cmd;
}

// Escape any characters in a URL which might cause problems when
// calling system().
GString *QtPDFCore::mungeURL(GString *url) {
  static const char *allowed = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                               "abcdefghijklmnopqrstuvwxyz"
                               "0123456789"
                               "-_.~/?:@&=+,#%";
  GString *newURL;
  char c;
  char buf[4];
  int i;

  newURL = new GString();
  for (i = 0; i < url->getLength(); ++i) {
    c = url->getChar(i);
    if (strchr(allowed, c)) {
      newURL->append(c);
    } else {
      sprintf(buf, "%%%02x", c & 0xff);
      newURL->append(buf);
    }
  }
  return newURL;
}

//------------------------------------------------------------------------
// find
//------------------------------------------------------------------------

GBool QtPDFCore::find(char *s, GBool caseSensitive, GBool next,
		      GBool backward, GBool wholeWord, GBool onePageOnly) {
  if (!PDFCore::find(s, caseSensitive, next,
		     backward, wholeWord, onePageOnly)) {
    return gFalse;
  }
#ifndef NO_TEXT_SELECT
  copySelection(gFalse);
#endif
  return gTrue;
}

GBool QtPDFCore::findU(Unicode *u, int len, GBool caseSensitive,
		       GBool next, GBool backward,
		       GBool wholeWord, GBool onePageOnly) {
  if (!PDFCore::findU(u, len, caseSensitive, next,
		      backward, wholeWord, onePageOnly)) {
    return gFalse;
  }
#ifndef NO_TEXT_SELECT
  copySelection(gFalse);
#endif
  return gTrue;
}

//------------------------------------------------------------------------
// misc access
//------------------------------------------------------------------------

void QtPDFCore::setBusyCursor(GBool busy) {
  if (busy) {
    doSetCursor(Qt::WaitCursor);
  } else {
    doUnsetCursor();
  }
}

void QtPDFCore::doSetCursor(const QCursor &cursor) {
#ifndef QT_NO_CURSOR
  viewport->setCursor(cursor);
#endif
}

void QtPDFCore::doUnsetCursor() {
#ifndef QT_NO_CURSOR
  viewport->unsetCursor();
#endif
}

void QtPDFCore::takeFocus() {
  viewport->setFocus(Qt::OtherFocusReason);
}

QSize QtPDFCore::getBestSize() {
  DisplayMode mode;
  double zoomPercent;
  int w, h, pg, rot;

  if (!doc) {
    //~ what should this return?
    return QSize(612, 792);
  }
  mode = state->getDisplayMode();
  pg = tileMap->getFirstPage();
  rot = (state->getRotate() + doc->getPageRotate(pg)) % 360;
  zoomPercent = state->getZoom();
  if (zoomPercent < 0) {
    zoomPercent = globalParams->getDefaultFitZoom();
    if (zoomPercent <= 0) {
      zoomPercent = (int)((100 * displayDpi) / 72.0 + 0.5);
      if (zoomPercent < 100) {
	zoomPercent = 100;
      }
    }
  }
  if (rot == 90 || rot == 270) {
    w = (int)(doc->getPageCropHeight(pg) * 0.01 * zoomPercent + 0.5);
    h = (int)(doc->getPageCropWidth(pg) * 0.01 * zoomPercent + 0.5);
  } else {
    w = (int)(doc->getPageCropWidth(pg) * 0.01 * zoomPercent + 0.5);
    h = (int)(doc->getPageCropHeight(pg) * 0.01 * zoomPercent + 0.5);
  }
  if (mode == displayContinuous) {
    w += QApplication::style()->pixelMetric(QStyle::PM_ScrollBarExtent);
    h += tileMap->getContinuousPageSpacing();
  } else if (mode == displaySideBySideContinuous) {
    w = w * 2
        + tileMap->getHorizContinuousPageSpacing()
        + QApplication::style()->pixelMetric(QStyle::PM_ScrollBarExtent);
    h += tileMap->getContinuousPageSpacing();
  } else if (mode == displayHorizontalContinuous) {
    w += tileMap->getHorizContinuousPageSpacing();
    h += QApplication::style()->pixelMetric(QStyle::PM_ScrollBarExtent);
  }  else if (mode == displaySideBySideSingle) {
    w = w * 2 + tileMap->getHorizContinuousPageSpacing();
  }
  //~ these additions are a kludge to make this work -- 2 pixels are
  //~   padding in the QAbstractScrollArea; not sure where the rest go
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
  w += 6;
  h += 2;
#else
  w += 10;
  h += 4;
#endif
  return QSize((int)(w / scaleFactor), (int)(h / scaleFactor));
}

//------------------------------------------------------------------------
// GUI code
//------------------------------------------------------------------------

void QtPDFCore::resizeEvent() {
  setWindowSize((int)(viewport->width() * scaleFactor),
		(int)(viewport->height() * scaleFactor));
}

void QtPDFCore::paintEvent(int x, int y, int w, int h) {
  SplashBitmap *bitmap;
  GBool wholeWindow;

  QPainter painter(viewport);
  wholeWindow = x == 0 && y == 0 &&
                w == viewport->width() && h == viewport->height();
  bitmap = getWindowBitmap(wholeWindow);
  QImage image(bitmap->getDataPtr(), bitmap->getWidth(),
	       bitmap->getHeight(), QImage::Format_RGB888);
  painter.drawImage(QRect(x, y, w, h), image,
		    QRect((int)(x * scaleFactor), (int)(y * scaleFactor),
			  (int)(w * scaleFactor), (int)(h * scaleFactor)));
  if (paintDoneCbk) {
    (*paintDoneCbk)(paintDoneCbkData, (bool)isBitmapFinished());
  }
}

void QtPDFCore::scrollEvent() {
  // avoid loops, e.g., scrollTo -> finishUpdate -> updateScrollbars ->
  // hScrollbar.setValue -> scrollContentsBy -> scrollEvent -> scrollTo
  if (inUpdateScrollbars) {
    return;
  }
  scrollTo(hScrollBar->value(), vScrollBar->value());
}

void QtPDFCore::tick() {
  PDFCore::tick();
}

void QtPDFCore::invalidate(int x, int y, int w, int h) {
  viewport->update((int)(x / scaleFactor), (int)(y / scaleFactor),
		   (int)(w / scaleFactor), (int)(h / scaleFactor));
}

void QtPDFCore::updateScrollbars() {
  int winW, winH, horizLimit, vertLimit, horizMax, vertMax;
  bool vScrollBarVisible, hScrollBarVisible;

  inUpdateScrollbars = gTrue;

  winW = state->getWinW();
  winH = state->getWinH();
  tileMap->getScrollLimits(&horizLimit, &vertLimit);

  if (horizLimit > winW) {
    horizMax = horizLimit - winW;
  } else {
    horizMax = 0;
  }
  if (vertLimit > winH) {
    vertMax = vertLimit - winH;
  } else {
    vertMax = 0;
  }

  // Problem case A: in fixed zoom, there is a case where the page
  // just barely fits in the window; if the scrollbars are visible,
  // they reduce the available window size enough that they are
  // necessary, i.e., the scrollbars are only necessary if they're
  // visible -- so check for that situation and force the scrollbars
  // to be hidden.
  // NB: {h,v}ScrollBar->isVisible() are unreliable at startup, so
  //     we compare the viewport size to the ScrollArea size (with
  //     some slack for margins)
  vScrollBarVisible =
      viewport->parentWidget()->width() - viewport->width() > 8;
  hScrollBarVisible =
      viewport->parentWidget()->height() - viewport->height() > 8;
  if (state->getZoom() >= 0 &&
      vScrollBarVisible &&
      hScrollBarVisible &&
      horizMax <= vScrollBar->width() &&
      vertMax <= hScrollBar->height()) {
    horizMax = 0;
    vertMax = 0;
  }

  // Problem case B: in fit-to-width mode, with the vertical scrollbar
  // visible, if the window is just tall enough to fit the page, then
  // the vertical scrollbar will be hidden, resulting in a wider
  // window, resulting in a taller page (because of fit-to-width),
  // resulting in the scrollbar being unhidden, in an infinite loop --
  // so always force the vertical scroll bar to be visible in
  // fit-to-width mode (and in fit-to-page cases where the vertical
  // scrollbar is potentially visible).
  if (state->getZoom() == zoomWidth ||
      (state->getZoom() == zoomPage &&
       (state->getDisplayMode() == displayContinuous ||
	state->getDisplayMode() == displaySideBySideContinuous))) {
    if (vertMax == 0) {
      vertMax = 1;
    }

  // Problem case C: same as case B, but for fit-to-height mode and
  // the horizontal scrollbar.
  } else if (state->getZoom() == zoomHeight ||
	     (state->getZoom() == zoomPage &&
	      state->getDisplayMode() == displayHorizontalContinuous)) {
    if (horizMax == 0) {
      horizMax = 1;
    }
  }

  hScrollBar->setMaximum(horizMax);
  hScrollBar->setPageStep(winW);
  hScrollBar->setValue(state->getScrollX());

  vScrollBar->setMaximum(vertMax);
  vScrollBar->setPageStep(winH);
  vScrollBar->setValue(state->getScrollY());

  inUpdateScrollbars = gFalse;
}

GBool QtPDFCore::checkForNewFile() {
  QDateTime newModTime;

  if (doc->getFileName()) {
    newModTime = QFileInfo(doc->getFileName()->getCString()).lastModified();
    if (newModTime != modTime) {
      modTime = newModTime;
      return gTrue;
    }
  }
  return gFalse;
}

void QtPDFCore::preLoad() {
  if (preLoadCbk) {
    (*preLoadCbk)(preLoadCbkData);
  }
}

void QtPDFCore::postLoad() {
  if (postLoadCbk) {
    (*postLoadCbk)(postLoadCbkData);
  }
}

//------------------------------------------------------------------------
// password dialog
//------------------------------------------------------------------------

GString *QtPDFCore::getPassword() {
  QString s;
  bool ok;

  if (!showPasswordDialog) {
    return NULL;
  }
  s = QInputDialog::getText(viewport, "PDF Password",
			    "This document requires a password",
			    QLineEdit::Password, "", &ok, Qt::Dialog);
  if (ok) {
    return new GString(s.toLocal8Bit().constData());
  } else {
    return NULL;
  }
}
